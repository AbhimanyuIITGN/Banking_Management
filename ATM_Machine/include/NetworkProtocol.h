#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include <string>
#include <vector>
#include <memory>

// Message types for ATM-Bank communication
enum class MessageType {
    LOGIN_REQUEST,
    LOGIN_RESPONSE,
    BALANCE_REQUEST,
    BALANCE_RESPONSE,
    WITHDRAW_REQUEST,
    WITHDRAW_RESPONSE,
    LOGOUT_REQUEST,
    LOGOUT_RESPONSE,
    ERROR_RESPONSE
};

// Request/Response structures
struct LoginRequest {
    std::string email;
    std::string password;
    std::string atm_id;
};

struct LoginResponse {
    bool success;
    std::string message;
    std::string user_name;
    int user_id;
    std::string session_token;
};

struct BalanceRequest {
    std::string session_token;
    int account_id;
};

struct BalanceResponse {
    bool success;
    std::string message;
    double balance;
    std::string account_type;
};

struct WithdrawRequest {
    std::string session_token;
    int account_id;
    double amount;
};

struct WithdrawResponse {
    bool success;
    std::string message;
    double new_balance;
    std::string transaction_id;
};

struct LogoutRequest {
    std::string session_token;
};

struct LogoutResponse {
    bool success;
    std::string message;
};

struct ErrorResponse {
    std::string error_code;
    std::string error_message;
};

// Network message wrapper
struct NetworkMessage {
    MessageType type;
    std::string payload;
    std::string timestamp;
    
    NetworkMessage(MessageType t, const std::string& p) 
        : type(t), payload(p) {}
};

// Protocol constants
const int DEFAULT_BANK_PORT = 8080;
const int MAX_MESSAGE_SIZE = 4096;
const std::string PROTOCOL_VERSION = "1.0";

// Utility functions
std::string messageTypeToString(MessageType type);
MessageType stringToMessageType(const std::string& str);
std::string getCurrentTimestamp();

#endif // NETWORK_PROTOCOL_H
