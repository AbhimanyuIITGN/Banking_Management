#include "NetworkProtocol.h"
#include <chrono>
#include <iomanip>
#include <sstream>

// Convert message type to string
std::string messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::LOGIN_REQUEST: return "LOGIN_REQUEST";
        case MessageType::LOGIN_RESPONSE: return "LOGIN_RESPONSE";
        case MessageType::BALANCE_REQUEST: return "BALANCE_REQUEST";
        case MessageType::BALANCE_RESPONSE: return "BALANCE_RESPONSE";
        case MessageType::WITHDRAW_REQUEST: return "WITHDRAW_REQUEST";
        case MessageType::WITHDRAW_RESPONSE: return "WITHDRAW_RESPONSE";
        case MessageType::LOGOUT_REQUEST: return "LOGOUT_REQUEST";
        case MessageType::LOGOUT_RESPONSE: return "LOGOUT_RESPONSE";
        case MessageType::ERROR_RESPONSE: return "ERROR_RESPONSE";
        default: return "UNKNOWN";
    }
}

// Convert string to message type
MessageType stringToMessageType(const std::string& str) {
    if (str == "LOGIN_REQUEST") return MessageType::LOGIN_REQUEST;
    if (str == "LOGIN_RESPONSE") return MessageType::LOGIN_RESPONSE;
    if (str == "BALANCE_REQUEST") return MessageType::BALANCE_REQUEST;
    if (str == "BALANCE_RESPONSE") return MessageType::BALANCE_RESPONSE;
    if (str == "WITHDRAW_REQUEST") return MessageType::WITHDRAW_REQUEST;
    if (str == "WITHDRAW_RESPONSE") return MessageType::WITHDRAW_RESPONSE;
    if (str == "LOGOUT_REQUEST") return MessageType::LOGOUT_REQUEST;
    if (str == "LOGOUT_RESPONSE") return MessageType::LOGOUT_RESPONSE;
    if (str == "ERROR_RESPONSE") return MessageType::ERROR_RESPONSE;
    return MessageType::ERROR_RESPONSE;
}

// Get current timestamp
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
