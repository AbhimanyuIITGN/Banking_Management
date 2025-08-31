#ifndef BANK_SERVER_H
#define BANK_SERVER_H

#include "BankSystem.h"
#include "NetworkProtocol.h"
#include "JsonHandler.h"
#include "Encryption.h"
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

class BankServer {
private:
    BankSystem& bank_system;
    int server_socket;
    int port;
    std::atomic<bool> running;
    std::vector<std::thread> client_threads;
    
    // Session management (using vectors to avoid linker issues)
    std::vector<std::pair<std::string, int>> active_sessions; // token -> user_id
    std::vector<std::pair<std::string, std::string>> session_atm_map; // token -> atm_id
    std::mutex session_mutex;
    
    // Client connection tracking
    std::vector<int> client_sockets;
    std::mutex client_mutex;

public:
    explicit BankServer(int port = DEFAULT_BANK_PORT);
    ~BankServer();
    
    // Server lifecycle
    bool start();
    void stop();
    bool isRunning() const { return running; }
    
    // Client handling
    void handleClient(int client_socket);
    void processMessage(int client_socket, const std::string& encrypted_message);
    
    // Message handlers
    std::string handleLoginRequest(const std::string& json_payload);
    std::string handleBalanceRequest(const std::string& json_payload);
    std::string handleWithdrawRequest(const std::string& json_payload);
    std::string handleLogoutRequest(const std::string& json_payload);
    
    // Session management
    std::string createSession(int user_id, const std::string& atm_id);
    bool validateSession(const std::string& token);
    int getUserIdFromSession(const std::string& token);
    void removeSession(const std::string& token);
    
    // Utility methods
    void broadcastMessage(const std::string& message);
    void displayServerStats() const;
    int getActiveClientCount() const;
    
private:
    // Socket operations
    bool setupSocket();
    void cleanupSocket();
    std::string receiveMessage(int client_socket);
    bool sendMessage(int client_socket, const std::string& message);
    
    // Error handling
    std::string createErrorResponse(const std::string& error_code, const std::string& error_message);
};

#endif // BANK_SERVER_H
