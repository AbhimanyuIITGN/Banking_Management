#ifndef ATM_CLIENT_H
#define ATM_CLIENT_H

#include "NetworkProtocol.h"
#include "JsonHandler.h"
#include "Encryption.h"
#include <string>
#include <vector>
#include <atomic>

class ATMClient {
private:
    int client_socket;
    std::string server_host;
    int server_port;
    std::atomic<bool> connected;
    
    // Session data
    std::string session_token;
    std::string user_name;
    int user_id;
    std::string atm_id;
    
    // User accounts cache
    std::vector<int> user_accounts;

public:
    ATMClient(const std::string& host = "localhost", int port = DEFAULT_BANK_PORT);
    ~ATMClient();
    
    // Connection management
    bool connectToBank();
    void disconnect();
    bool isConnected() const { return connected; }
    
    // ATM operations
    bool login(const std::string& email, const std::string& password);
    bool checkBalance(int account_id, double& balance, std::string& account_type);
    bool withdraw(int account_id, double amount, double& new_balance, std::string& transaction_id);
    bool logout();
    
    // User interface
    void run();
    void showATMMenu();
    void handleLogin();
    void handleBalanceCheck();
    void handleWithdraw();
    void handleLogout();
    
    // Account management
    void loadUserAccounts();
    void displayAccounts() const;
    int selectAccount() const;
    
    // Utility methods
    std::string getATMId() const { return atm_id; }
    std::string getUserName() const { return user_name; }
    bool isLoggedIn() const { return !session_token.empty(); }
    
private:
    // Network communication
    bool sendEncryptedMessage(const std::string& message);
    std::string receiveEncryptedMessage();
    
    // Socket operations
    bool setupSocket();
    void cleanupSocket();
    
    // Input helpers
    std::string getSecureInput();
    int getIntInput();
    double getDoubleInput();
    
    // Error handling
    void handleNetworkError(const std::string& operation);
    void displayError(const std::string& message);
    
    // ATM ID generation
    void generateATMId();
};

#endif // ATM_CLIENT_H
