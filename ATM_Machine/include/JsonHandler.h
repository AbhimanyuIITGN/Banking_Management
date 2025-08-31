#ifndef JSON_HANDLER_H
#define JSON_HANDLER_H

#include "NetworkProtocol.h"
#include <string>
#include <unordered_map>

class JsonHandler {
public:
    // Serialize requests to JSON
    static std::string serializeLoginRequest(const LoginRequest& request);
    static std::string serializeBalanceRequest(const BalanceRequest& request);
    static std::string serializeWithdrawRequest(const WithdrawRequest& request);
    static std::string serializeLogoutRequest(const LogoutRequest& request);
    
    // Serialize responses to JSON
    static std::string serializeLoginResponse(const LoginResponse& response);
    static std::string serializeBalanceResponse(const BalanceResponse& response);
    static std::string serializeWithdrawResponse(const WithdrawResponse& response);
    static std::string serializeLogoutResponse(const LogoutResponse& response);
    static std::string serializeErrorResponse(const ErrorResponse& response);
    
    // Deserialize requests from JSON
    static LoginRequest deserializeLoginRequest(const std::string& json);
    static BalanceRequest deserializeBalanceRequest(const std::string& json);
    static WithdrawRequest deserializeWithdrawRequest(const std::string& json);
    static LogoutRequest deserializeLogoutRequest(const std::string& json);
    
    // Deserialize responses from JSON
    static LoginResponse deserializeLoginResponse(const std::string& json);
    static BalanceResponse deserializeBalanceResponse(const std::string& json);
    static WithdrawResponse deserializeWithdrawResponse(const std::string& json);
    static LogoutResponse deserializeLogoutResponse(const std::string& json);
    static ErrorResponse deserializeErrorResponse(const std::string& json);
    
    // Create network message
    static std::string createNetworkMessage(MessageType type, const std::string& payload);
    static NetworkMessage parseNetworkMessage(const std::string& message);
    
private:
    // Simple JSON parsing helpers
    static std::string extractJsonValue(const std::string& json, const std::string& key);
    static bool extractJsonBool(const std::string& json, const std::string& key);
    static double extractJsonDouble(const std::string& json, const std::string& key);
    static int extractJsonInt(const std::string& json, const std::string& key);
    
    // JSON creation helpers
    static std::string createJsonString(const std::string& key, const std::string& value);
    static std::string createJsonBool(const std::string& key, bool value);
    static std::string createJsonDouble(const std::string& key, double value);
    static std::string createJsonInt(const std::string& key, int value);
    
    // Escape JSON strings
    static std::string escapeJsonString(const std::string& str);
};

#endif // JSON_HANDLER_H
