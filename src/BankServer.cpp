#include "BankServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <ctime>

BankServer::BankServer(int port)
    : bank_system(BankSystem::getInstance()), server_socket(-1), port(port), running(false) {
}

BankServer::~BankServer() {
    stop();
}

// Start the server
bool BankServer::start() {
    if (running) {
        std::cout << "Server is already running" << std::endl;
        return false;
    }
    
    if (!setupSocket()) {
        return false;
    }
    
    running = true;
    std::cout << "Bank Server started on port " << port << std::endl;
    std::cout << "Waiting for ATM connections..." << std::endl;
    
    // Accept client connections
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }
        
        std::cout << "New ATM connected from " << inet_ntoa(client_addr.sin_addr) << std::endl;
        
        // Store client socket
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            client_sockets.push_back(client_socket);
        }
        
        // Handle client in separate thread
        client_threads.emplace_back(&BankServer::handleClient, this, client_socket);
    }
    
    return true;
}

// Stop the server
void BankServer::stop() {
    if (!running) return;
    
    running = false;
    std::cout << "Stopping Bank Server..." << std::endl;
    
    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        for (int socket : client_sockets) {
            close(socket);
        }
        client_sockets.clear();
    }
    
    // Wait for all client threads to finish
    for (auto& thread : client_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    client_threads.clear();
    
    cleanupSocket();
    std::cout << "Bank Server stopped" << std::endl;
}

// Handle individual client
void BankServer::handleClient(int client_socket) {
    std::cout << "Handling ATM client on socket " << client_socket << std::endl;
    
    while (running) {
        std::string encrypted_message = receiveMessage(client_socket);
        if (encrypted_message.empty()) {
            break; // Client disconnected
        }
        
        processMessage(client_socket, encrypted_message);
    }
    
    // Remove client socket from tracking
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        client_sockets.erase(
            std::remove(client_sockets.begin(), client_sockets.end(), client_socket),
            client_sockets.end()
        );
    }
    
    close(client_socket);
    std::cout << "ATM client disconnected from socket " << client_socket << std::endl;
}

// Process encrypted message from client
void BankServer::processMessage(int client_socket, const std::string& encrypted_message) {
    try {
        // Decrypt the message
        std::string decrypted = Encryption::decodeAndDecrypt(encrypted_message);
        std::cout << "Received message: " << decrypted << std::endl;
        
        // Parse network message
        NetworkMessage net_msg = JsonHandler::parseNetworkMessage(decrypted);
        
        std::string response_json;
        
        // Handle different message types
        switch (net_msg.type) {
            case MessageType::LOGIN_REQUEST:
                response_json = handleLoginRequest(net_msg.payload);
                break;
            case MessageType::BALANCE_REQUEST:
                response_json = handleBalanceRequest(net_msg.payload);
                break;
            case MessageType::WITHDRAW_REQUEST:
                response_json = handleWithdrawRequest(net_msg.payload);
                break;
            case MessageType::LOGOUT_REQUEST:
                response_json = handleLogoutRequest(net_msg.payload);
                break;
            default:
                response_json = createErrorResponse("INVALID_REQUEST", "Unknown message type");
                break;
        }
        
        // Encrypt and send response
        std::string encrypted_response = Encryption::encryptAndEncode(response_json);
        sendMessage(client_socket, encrypted_response);
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
        std::string error_response = createErrorResponse("PROCESSING_ERROR", e.what());
        std::string encrypted_error = Encryption::encryptAndEncode(error_response);
        sendMessage(client_socket, encrypted_error);
    }
}

// Handle login request
std::string BankServer::handleLoginRequest(const std::string& json_payload) {
    try {
        LoginRequest request = JsonHandler::deserializeLoginRequest(json_payload);
        std::cout << "Login attempt from ATM " << request.atm_id << " for user: " << request.email << std::endl;

        // Authenticate user
        if (bank_system.authenticateUser(request.email, request.password)) {
            auto user = bank_system.getCurrentUser();
            if (user) {
                // Create session
                std::string session_token = createSession(user->getUserId(), request.atm_id);

                LoginResponse response;
                response.success = true;
                response.message = "Login successful";
                response.user_name = user->getName();
                response.user_id = user->getUserId();
                response.session_token = session_token;

                std::cout << "Login successful for user: " << user->getName() << std::endl;
                return JsonHandler::createNetworkMessage(MessageType::LOGIN_RESPONSE,
                                                       JsonHandler::serializeLoginResponse(response));
            }
        }

        LoginResponse response;
        response.success = false;
        response.message = "Invalid credentials";
        response.user_name = "";
        response.user_id = 0;
        response.session_token = "";

        std::cout << "Login failed for user: " << request.email << std::endl;
        return JsonHandler::createNetworkMessage(MessageType::LOGIN_RESPONSE,
                                               JsonHandler::serializeLoginResponse(response));

    } catch (const std::exception& e) {
        return createErrorResponse("LOGIN_ERROR", e.what());
    }
}

// Handle balance request
std::string BankServer::handleBalanceRequest(const std::string& json_payload) {
    try {
        BalanceRequest request = JsonHandler::deserializeBalanceRequest(json_payload);

        if (!validateSession(request.session_token)) {
            BalanceResponse response;
            response.success = false;
            response.message = "Invalid session";
            response.balance = 0.0;
            response.account_type = "";

            return JsonHandler::createNetworkMessage(MessageType::BALANCE_RESPONSE,
                                                   JsonHandler::serializeBalanceResponse(response));
        }

        int user_id = getUserIdFromSession(request.session_token);

        // Validate account ownership
        if (!bank_system.validateAccountOwnership(request.account_id, user_id)) {
            BalanceResponse response;
            response.success = false;
            response.message = "Account access denied";
            response.balance = 0.0;
            response.account_type = "";

            return JsonHandler::createNetworkMessage(MessageType::BALANCE_RESPONSE,
                                                   JsonHandler::serializeBalanceResponse(response));
        }

        // Get account
        auto account = bank_system.getAccount(request.account_id);
        if (account) {
            BalanceResponse response;
            response.success = true;
            response.message = "Balance retrieved successfully";
            response.balance = account->getBalance();
            response.account_type = account->getAccountTypeString();

            std::cout << "Balance check for account " << request.account_id << ": $" << response.balance << std::endl;
            return JsonHandler::createNetworkMessage(MessageType::BALANCE_RESPONSE,
                                                   JsonHandler::serializeBalanceResponse(response));
        }

        BalanceResponse response;
        response.success = false;
        response.message = "Account not found";
        response.balance = 0.0;
        response.account_type = "";

        return JsonHandler::createNetworkMessage(MessageType::BALANCE_RESPONSE,
                                               JsonHandler::serializeBalanceResponse(response));

    } catch (const std::exception& e) {
        return createErrorResponse("BALANCE_ERROR", e.what());
    }
}

// Handle withdraw request
std::string BankServer::handleWithdrawRequest(const std::string& json_payload) {
    try {
        WithdrawRequest request = JsonHandler::deserializeWithdrawRequest(json_payload);

        if (!validateSession(request.session_token)) {
            WithdrawResponse response;
            response.success = false;
            response.message = "Invalid session";
            response.new_balance = 0.0;
            response.transaction_id = "";

            return JsonHandler::createNetworkMessage(MessageType::WITHDRAW_RESPONSE,
                                                   JsonHandler::serializeWithdrawResponse(response));
        }

        int user_id = getUserIdFromSession(request.session_token);

        // Validate account ownership
        if (!bank_system.validateAccountOwnership(request.account_id, user_id)) {
            WithdrawResponse response;
            response.success = false;
            response.message = "Account access denied";
            response.new_balance = 0.0;
            response.transaction_id = "";

            return JsonHandler::createNetworkMessage(MessageType::WITHDRAW_RESPONSE,
                                                   JsonHandler::serializeWithdrawResponse(response));
        }

        // Perform withdrawal
        std::cout << "Processing withdrawal: $" << request.amount << " from account " << request.account_id << std::endl;

        if (bank_system.withdraw(request.account_id, request.amount)) {
            auto account = bank_system.getAccount(request.account_id);

            WithdrawResponse response;
            response.success = true;
            response.message = "Withdrawal successful";
            response.new_balance = account ? account->getBalance() : 0.0;
            response.transaction_id = "TXN-" + std::to_string(std::time(nullptr));

            std::cout << "Withdrawal successful. New balance: $" << response.new_balance << std::endl;
            return JsonHandler::createNetworkMessage(MessageType::WITHDRAW_RESPONSE,
                                                   JsonHandler::serializeWithdrawResponse(response));
        } else {
            WithdrawResponse response;
            response.success = false;
            response.message = "Withdrawal failed - insufficient funds or invalid amount";
            response.new_balance = 0.0;
            response.transaction_id = "";

            std::cout << "Withdrawal failed for account " << request.account_id << std::endl;
            return JsonHandler::createNetworkMessage(MessageType::WITHDRAW_RESPONSE,
                                                   JsonHandler::serializeWithdrawResponse(response));
        }

    } catch (const std::exception& e) {
        return createErrorResponse("WITHDRAW_ERROR", e.what());
    }
}

// Handle logout request
std::string BankServer::handleLogoutRequest(const std::string& json_payload) {
    try {
        LogoutRequest request = JsonHandler::deserializeLogoutRequest(json_payload);

        if (validateSession(request.session_token)) {
            removeSession(request.session_token);
            bank_system.logout();

            LogoutResponse response;
            response.success = true;
            response.message = "Logout successful";

            std::cout << "User logged out successfully" << std::endl;
            return JsonHandler::createNetworkMessage(MessageType::LOGOUT_RESPONSE,
                                                   JsonHandler::serializeLogoutResponse(response));
        } else {
            LogoutResponse response;
            response.success = false;
            response.message = "Invalid session";

            return JsonHandler::createNetworkMessage(MessageType::LOGOUT_RESPONSE,
                                                   JsonHandler::serializeLogoutResponse(response));
        }

    } catch (const std::exception& e) {
        return createErrorResponse("LOGOUT_ERROR", e.what());
    }
}

// Session management methods
std::string BankServer::createSession(int user_id, const std::string& atm_id) {
    std::lock_guard<std::mutex> lock(session_mutex);

    std::string token = Encryption::generateSessionToken();
    active_sessions.push_back({token, user_id});
    session_atm_map.push_back({token, atm_id});

    std::cout << "Created session for user " << user_id << " from ATM " << atm_id << std::endl;
    return token;
}

bool BankServer::validateSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(session_mutex);
    for (const auto& session : active_sessions) {
        if (session.first == token) {
            return true;
        }
    }
    return false;
}

int BankServer::getUserIdFromSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(session_mutex);
    for (const auto& session : active_sessions) {
        if (session.first == token) {
            return session.second;
        }
    }
    return 0;
}

void BankServer::removeSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(session_mutex);

    // Remove from active_sessions
    active_sessions.erase(
        std::remove_if(active_sessions.begin(), active_sessions.end(),
                      [&token](const auto& session) { return session.first == token; }),
        active_sessions.end()
    );

    // Remove from session_atm_map
    session_atm_map.erase(
        std::remove_if(session_atm_map.begin(), session_atm_map.end(),
                      [&token](const auto& session) { return session.first == token; }),
        session_atm_map.end()
    );
}

// Socket operations
bool BankServer::setupSocket() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create server socket" << std::endl;
        return false;
    }

    // Allow socket reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_socket);
        return false;
    }

    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port << std::endl;
        close(server_socket);
        return false;
    }

    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket);
        return false;
    }

    return true;
}

void BankServer::cleanupSocket() {
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
}

// Receive message from client
std::string BankServer::receiveMessage(int client_socket) {
    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, sizeof(buffer));

    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        return ""; // Client disconnected or error
    }

    return std::string(buffer, bytes_received);
}

// Send message to client
bool BankServer::sendMessage(int client_socket, const std::string& message) {
    ssize_t bytes_sent = send(client_socket, message.c_str(), message.length(), 0);
    return bytes_sent == static_cast<ssize_t>(message.length());
}

// Create error response
std::string BankServer::createErrorResponse(const std::string& error_code, const std::string& error_message) {
    ErrorResponse error;
    error.error_code = error_code;
    error.error_message = error_message;

    return JsonHandler::createNetworkMessage(MessageType::ERROR_RESPONSE,
                                           JsonHandler::serializeErrorResponse(error));
}

// Display server statistics
void BankServer::displayServerStats() const {
    // Create non-const reference for locking
    std::mutex& session_mut = const_cast<std::mutex&>(session_mutex);
    std::mutex& client_mut = const_cast<std::mutex&>(client_mutex);

    std::lock_guard<std::mutex> session_lock(session_mut);
    std::lock_guard<std::mutex> client_lock(client_mut);

    std::cout << "\n=== Bank Server Statistics ===" << std::endl;
    std::cout << "Active ATM connections: " << client_sockets.size() << std::endl;
    std::cout << "Active sessions: " << active_sessions.size() << std::endl;
    std::cout << "Server port: " << port << std::endl;
    std::cout << "Server status: " << (running ? "Running" : "Stopped") << std::endl;
}

// Get active client count
int BankServer::getActiveClientCount() const {
    std::mutex& client_mut = const_cast<std::mutex&>(client_mutex);
    std::lock_guard<std::mutex> lock(client_mut);
    return client_sockets.size();
}
