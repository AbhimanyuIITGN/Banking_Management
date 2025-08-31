#include "ATMClient.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <random>
#include <iomanip>

ATMClient::ATMClient(const std::string& host, int port) 
    : server_host(host), server_port(port), connected(false), client_socket(-1), user_id(0) {
    generateATMId();
}

ATMClient::~ATMClient() {
    disconnect();
}

// Connect to bank server
bool ATMClient::connectToBank() {
    if (connected) {
        std::cout << "Already connected to bank server" << std::endl;
        return true;
    }
    
    if (!setupSocket()) {
        return false;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_host.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid server address: " << server_host << std::endl;
        cleanupSocket();
        return false;
    }
    
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to bank server at " << server_host << ":" << server_port << std::endl;
        cleanupSocket();
        return false;
    }
    
    connected = true;
    std::cout << "Connected to bank server at " << server_host << ":" << server_port << std::endl;
    return true;
}

// Disconnect from server
void ATMClient::disconnect() {
    if (connected) {
        if (!session_token.empty()) {
            logout(); // Logout before disconnecting
        }
        cleanupSocket();
        connected = false;
        std::cout << "Disconnected from bank server" << std::endl;
    }
}

// Login to bank
bool ATMClient::login(const std::string& email, const std::string& password) {
    if (!connected) {
        std::cerr << "Not connected to bank server" << std::endl;
        return false;
    }
    
    try {
        LoginRequest request;
        request.email = email;
        request.password = password;
        request.atm_id = atm_id;
        
        std::string json_payload = JsonHandler::serializeLoginRequest(request);
        std::string network_message = JsonHandler::createNetworkMessage(MessageType::LOGIN_REQUEST, json_payload);
        
        if (!sendEncryptedMessage(network_message)) {
            std::cerr << "Failed to send login request" << std::endl;
            return false;
        }
        
        std::string encrypted_response = receiveEncryptedMessage();
        if (encrypted_response.empty()) {
            std::cerr << "No response from server" << std::endl;
            return false;
        }
        
        NetworkMessage net_msg = JsonHandler::parseNetworkMessage(encrypted_response);
        LoginResponse response = JsonHandler::deserializeLoginResponse(net_msg.payload);
        
        if (response.success) {
            session_token = response.session_token;
            user_name = response.user_name;
            user_id = response.user_id;
            
            std::cout << "Login successful! Welcome, " << user_name << std::endl;
            loadUserAccounts();
            return true;
        } else {
            std::cerr << "Login failed: " << response.message << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Login error: " << e.what() << std::endl;
        return false;
    }
}

// Check account balance
bool ATMClient::checkBalance(int account_id, double& balance, std::string& account_type) {
    if (!connected || session_token.empty()) {
        std::cerr << "Not logged in" << std::endl;
        return false;
    }
    
    try {
        BalanceRequest request;
        request.session_token = session_token;
        request.account_id = account_id;
        
        std::string json_payload = JsonHandler::serializeBalanceRequest(request);
        std::string network_message = JsonHandler::createNetworkMessage(MessageType::BALANCE_REQUEST, json_payload);
        
        if (!sendEncryptedMessage(network_message)) {
            std::cerr << "Failed to send balance request" << std::endl;
            return false;
        }
        
        std::string encrypted_response = receiveEncryptedMessage();
        if (encrypted_response.empty()) {
            std::cerr << "No response from server" << std::endl;
            return false;
        }
        
        NetworkMessage net_msg = JsonHandler::parseNetworkMessage(encrypted_response);
        BalanceResponse response = JsonHandler::deserializeBalanceResponse(net_msg.payload);
        
        if (response.success) {
            balance = response.balance;
            account_type = response.account_type;
            return true;
        } else {
            std::cerr << "Balance check failed: " << response.message << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Balance check error: " << e.what() << std::endl;
        return false;
    }
}

// Withdraw money
bool ATMClient::withdraw(int account_id, double amount, double& new_balance, std::string& transaction_id) {
    if (!connected || session_token.empty()) {
        std::cerr << "Not logged in" << std::endl;
        return false;
    }

    try {
        WithdrawRequest request;
        request.session_token = session_token;
        request.account_id = account_id;
        request.amount = amount;

        std::string json_payload = JsonHandler::serializeWithdrawRequest(request);
        std::string network_message = JsonHandler::createNetworkMessage(MessageType::WITHDRAW_REQUEST, json_payload);

        if (!sendEncryptedMessage(network_message)) {
            std::cerr << "Failed to send withdraw request" << std::endl;
            return false;
        }

        std::string encrypted_response = receiveEncryptedMessage();
        if (encrypted_response.empty()) {
            std::cerr << "No response from server" << std::endl;
            return false;
        }

        NetworkMessage net_msg = JsonHandler::parseNetworkMessage(encrypted_response);
        WithdrawResponse response = JsonHandler::deserializeWithdrawResponse(net_msg.payload);

        if (response.success) {
            new_balance = response.new_balance;
            transaction_id = response.transaction_id;
            return true;
        } else {
            std::cerr << "Withdrawal failed: " << response.message << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "Withdrawal error: " << e.what() << std::endl;
        return false;
    }
}

// Logout
bool ATMClient::logout() {
    if (!connected || session_token.empty()) {
        return true; // Already logged out
    }

    try {
        LogoutRequest request;
        request.session_token = session_token;

        std::string json_payload = JsonHandler::serializeLogoutRequest(request);
        std::string network_message = JsonHandler::createNetworkMessage(MessageType::LOGOUT_REQUEST, json_payload);

        if (!sendEncryptedMessage(network_message)) {
            std::cerr << "Failed to send logout request" << std::endl;
            return false;
        }

        std::string encrypted_response = receiveEncryptedMessage();
        if (!encrypted_response.empty()) {
            NetworkMessage net_msg = JsonHandler::parseNetworkMessage(encrypted_response);
            LogoutResponse response = JsonHandler::deserializeLogoutResponse(net_msg.payload);

            if (response.success) {
                session_token.clear();
                user_name.clear();
                user_id = 0;
                user_accounts.clear();
                std::cout << "Logout successful" << std::endl;
                return true;
            }
        }

        // Clear session even if server response failed
        session_token.clear();
        user_name.clear();
        user_id = 0;
        user_accounts.clear();
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Logout error: " << e.what() << std::endl;
        return false;
    }
}

// Main ATM interface
void ATMClient::run() {
    std::cout << "\n=== ATM Machine Started ===" << std::endl;
    std::cout << "ATM ID: " << atm_id << std::endl;

    if (!connectToBank()) {
        std::cerr << "Failed to connect to bank server. Exiting..." << std::endl;
        return;
    }

    while (connected) {
        if (!isLoggedIn()) {
            handleLogin();
        } else {
            showATMMenu();

            std::cout << "Select option: ";
            int choice = getIntInput();

            switch (choice) {
                case 1:
                    handleBalanceCheck();
                    break;
                case 2:
                    handleWithdraw();
                    break;
                case 3:
                    handleLogout();
                    break;
                case 4:
                    std::cout << "Thank you for using our ATM service!" << std::endl;
                    disconnect();
                    return;
                default:
                    std::cout << "Invalid option. Please try again." << std::endl;
                    break;
            }
        }
    }
}

// Show ATM menu
void ATMClient::showATMMenu() {
    std::cout << "\n=== ATM Menu ===" << std::endl;
    std::cout << "Welcome, " << user_name << "!" << std::endl;
    std::cout << "1. Check Balance" << std::endl;
    std::cout << "2. Withdraw Money" << std::endl;
    std::cout << "3. Logout" << std::endl;
    std::cout << "4. Exit" << std::endl;
    std::cout << "==================" << std::endl;
}

// Handle login
void ATMClient::handleLogin() {
    std::cout << "\n=== ATM Login ===" << std::endl;
    std::cout << "Enter email: ";
    std::string email;
    std::getline(std::cin, email);

    std::cout << "Enter password: ";
    std::string password = getSecureInput();

    std::cout << "Authenticating..." << std::endl;
    if (login(email, password)) {
        std::cout << "Login successful!" << std::endl;
    } else {
        std::cout << "Login failed. Please try again." << std::endl;
    }
}

// Handle balance check
void ATMClient::handleBalanceCheck() {
    displayAccounts();

    std::cout << "Enter account number to check balance: ";
    int account_id = getIntInput();

    double balance;
    std::string account_type;

    std::cout << "Checking balance..." << std::endl;
    if (checkBalance(account_id, balance, account_type)) {
        std::cout << "\n=== Balance Information ===" << std::endl;
        std::cout << "Account ID: " << account_id << std::endl;
        std::cout << "Account Type: " << account_type << std::endl;
        std::cout << "Current Balance: $" << std::fixed << std::setprecision(2) << balance << std::endl;
        std::cout << "=========================" << std::endl;
    } else {
        std::cout << "Failed to retrieve balance. Please try again." << std::endl;
    }
}

// Handle withdrawal
void ATMClient::handleWithdraw() {
    displayAccounts();

    std::cout << "Enter account number for withdrawal: ";
    int account_id = getIntInput();

    std::cout << "Enter withdrawal amount: $";
    double amount = getDoubleInput();

    if (amount <= 0) {
        std::cout << "Invalid amount. Please enter a positive value." << std::endl;
        return;
    }

    double new_balance;
    std::string transaction_id;

    std::cout << "Processing withdrawal..." << std::endl;
    if (withdraw(account_id, amount, new_balance, transaction_id)) {
        std::cout << "\n=== Withdrawal Successful ===" << std::endl;
        std::cout << "Amount Withdrawn: $" << std::fixed << std::setprecision(2) << amount << std::endl;
        std::cout << "New Balance: $" << std::fixed << std::setprecision(2) << new_balance << std::endl;
        std::cout << "Transaction ID: " << transaction_id << std::endl;
        std::cout << "Please take your cash." << std::endl;
        std::cout << "============================" << std::endl;
    } else {
        std::cout << "Withdrawal failed. Please check your balance and try again." << std::endl;
    }
}

// Handle logout
void ATMClient::handleLogout() {
    std::cout << "Logging out..." << std::endl;
    if (logout()) {
        std::cout << "Logout successful. Thank you!" << std::endl;
    } else {
        std::cout << "Logout completed." << std::endl;
    }
}

// Load user accounts (simplified for ATM)
void ATMClient::loadUserAccounts() {
    // For ATM, we'll use a simplified approach
    // In a real system, this would query the server for user accounts
    user_accounts.clear();

    // For demo purposes, assume accounts 1-5 might exist
    for (int i = 1; i <= 5; i++) {
        user_accounts.push_back(i);
    }
}

// Display available accounts
void ATMClient::displayAccounts() const {
    std::cout << "\nAvailable accounts: ";
    for (int account_id : user_accounts) {
        std::cout << account_id << " ";
    }
    std::cout << std::endl;
}

// Socket operations
bool ATMClient::setupSocket() {
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Failed to create client socket" << std::endl;
        return false;
    }
    return true;
}

void ATMClient::cleanupSocket() {
    if (client_socket >= 0) {
        close(client_socket);
        client_socket = -1;
    }
}

// Network communication
bool ATMClient::sendEncryptedMessage(const std::string& message) {
    std::string encrypted = Encryption::encryptAndEncode(message);
    ssize_t bytes_sent = send(client_socket, encrypted.c_str(), encrypted.length(), 0);
    return bytes_sent == static_cast<ssize_t>(encrypted.length());
}

std::string ATMClient::receiveEncryptedMessage() {
    char buffer[MAX_MESSAGE_SIZE];
    memset(buffer, 0, sizeof(buffer));

    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        return "";
    }

    std::string encrypted(buffer, bytes_received);
    return Encryption::decodeAndDecrypt(encrypted);
}

// Input helpers
std::string ATMClient::getSecureInput() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

int ATMClient::getIntInput() {
    int value;
    std::cin >> value;
    std::cin.ignore(); // Clear newline
    return value;
}

double ATMClient::getDoubleInput() {
    double value;
    std::cin >> value;
    std::cin.ignore(); // Clear newline
    return value;
}

// Generate ATM ID
void ATMClient::generateATMId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    atm_id = "ATM-" + std::to_string(dis(gen));
}
