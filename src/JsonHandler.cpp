#include "JsonHandler.h"
#include "NetworkProtocol.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iostream>

// Serialize login request
std::string JsonHandler::serializeLoginRequest(const LoginRequest& request) {
    std::stringstream ss;
    ss << "{";
    ss << createJsonString("email", request.email) << ",";
    ss << createJsonString("password", request.password) << ",";
    ss << createJsonString("atm_id", request.atm_id);
    ss << "}";
    return ss.str();
}

// Serialize balance request
std::string JsonHandler::serializeBalanceRequest(const BalanceRequest& request) {
    std::stringstream ss;
    ss << "{";
    ss << createJsonString("session_token", request.session_token) << ",";
    ss << createJsonInt("account_id", request.account_id);
    ss << "}";
    return ss.str();
}

// Serialize withdraw request
std::string JsonHandler::serializeWithdrawRequest(const WithdrawRequest& request) {
    std::stringstream ss;
    ss << "{";
    ss << createJsonString("session_token", request.session_token) << ",";
    ss << createJsonInt("account_id", request.account_id) << ",";
    ss << createJsonDouble("amount", request.amount);
    ss << "}";
    return ss.str();
}

// Serialize logout request
std::string JsonHandler::serializeLogoutRequest(const LogoutRequest& request) {
    std::stringstream ss;
    ss << "{";
    ss << createJsonString("session_token", request.session_token);
    ss << "}";
    return ss.str();
}

// Serialize login response
std::string JsonHandler::serializeLoginResponse(const LoginResponse& response) {
    std::stringstream ss;
    ss << "{";
    ss << createJsonBool("success", response.success) << ",";
    ss << createJsonString("message", response.message) << ",";
    ss << createJsonString("user_name", response.user_name) << ",";
    ss << createJsonInt("user_id", response.user_id) << ",";
    ss << createJsonString("session_token", response.session_token);
    ss << "}";
    return ss.str();
}

// Serialize balance response
std::string JsonHandler::serializeBalanceResponse(const BalanceResponse& response) {
    std::stringstream ss;
    ss << "{";
    ss << createJsonBool("success", response.success) << ",";
    ss << createJsonString("message", response.message) << ",";
    ss << createJsonDouble("balance", response.balance) << ",";
    ss << createJsonString("account_type", response.account_type);
    ss << "}";
    return ss.str();
}

// Serialize withdraw response
std::string JsonHandler::serializeWithdrawResponse(const WithdrawResponse& response) {
    std::stringstream ss;
    ss << "{";
    ss << createJsonBool("success", response.success) << ",";
    ss << createJsonString("message", response.message) << ",";
    ss << createJsonDouble("new_balance", response.new_balance) << ",";
    ss << createJsonString("transaction_id", response.transaction_id);
    ss << "}";
    return ss.str();
}

// Serialize logout response
std::string JsonHandler::serializeLogoutResponse(const LogoutResponse& response) {
    std::stringstream ss;
    ss << "{";
    ss << createJsonBool("success", response.success) << ",";
    ss << createJsonString("message", response.message);
    ss << "}";
    return ss.str();
}

// Serialize error response
std::string JsonHandler::serializeErrorResponse(const ErrorResponse& response) {
    std::stringstream ss;
    ss << "{";
    ss << createJsonString("error_code", response.error_code) << ",";
    ss << createJsonString("error_message", response.error_message);
    ss << "}";
    return ss.str();
}

// Deserialize login request
LoginRequest JsonHandler::deserializeLoginRequest(const std::string& json) {
    LoginRequest request;
    request.email = extractJsonValue(json, "email");
    request.password = extractJsonValue(json, "password");
    request.atm_id = extractJsonValue(json, "atm_id");
    return request;
}

// Deserialize balance request
BalanceRequest JsonHandler::deserializeBalanceRequest(const std::string& json) {
    BalanceRequest request;
    request.session_token = extractJsonValue(json, "session_token");
    request.account_id = extractJsonInt(json, "account_id");
    return request;
}

// Deserialize withdraw request
WithdrawRequest JsonHandler::deserializeWithdrawRequest(const std::string& json) {
    WithdrawRequest request;
    request.session_token = extractJsonValue(json, "session_token");
    request.account_id = extractJsonInt(json, "account_id");
    request.amount = extractJsonDouble(json, "amount");
    return request;
}

// Deserialize logout request
LogoutRequest JsonHandler::deserializeLogoutRequest(const std::string& json) {
    LogoutRequest request;
    request.session_token = extractJsonValue(json, "session_token");
    return request;
}

// Deserialize login response
LoginResponse JsonHandler::deserializeLoginResponse(const std::string& json) {
    LoginResponse response;
    response.success = extractJsonBool(json, "success");
    response.message = extractJsonValue(json, "message");
    response.user_name = extractJsonValue(json, "user_name");
    response.user_id = extractJsonInt(json, "user_id");
    response.session_token = extractJsonValue(json, "session_token");
    return response;
}

// Deserialize balance response
BalanceResponse JsonHandler::deserializeBalanceResponse(const std::string& json) {
    BalanceResponse response;
    response.success = extractJsonBool(json, "success");
    response.message = extractJsonValue(json, "message");
    response.balance = extractJsonDouble(json, "balance");
    response.account_type = extractJsonValue(json, "account_type");
    return response;
}

// Deserialize withdraw response
WithdrawResponse JsonHandler::deserializeWithdrawResponse(const std::string& json) {
    WithdrawResponse response;
    response.success = extractJsonBool(json, "success");
    response.message = extractJsonValue(json, "message");
    response.new_balance = extractJsonDouble(json, "new_balance");
    response.transaction_id = extractJsonValue(json, "transaction_id");
    return response;
}

// Deserialize logout response
LogoutResponse JsonHandler::deserializeLogoutResponse(const std::string& json) {
    LogoutResponse response;
    response.success = extractJsonBool(json, "success");
    response.message = extractJsonValue(json, "message");
    return response;
}

// Deserialize error response
ErrorResponse JsonHandler::deserializeErrorResponse(const std::string& json) {
    ErrorResponse response;
    response.error_code = extractJsonValue(json, "error_code");
    response.error_message = extractJsonValue(json, "error_message");
    return response;
}

// Create network message (simplified to avoid double-escaping)
std::string JsonHandler::createNetworkMessage(MessageType type, const std::string& payload) {
    // Instead of nesting JSON, just return the payload directly with type prefix
    return messageTypeToString(type) + "|" + payload;
}

// Parse network message (simplified)
NetworkMessage JsonHandler::parseNetworkMessage(const std::string& message) {
    size_t delimiter = message.find('|');
    if (delimiter == std::string::npos) {
        return NetworkMessage(MessageType::ERROR_RESPONSE, "");
    }

    std::string type_str = message.substr(0, delimiter);
    std::string payload = message.substr(delimiter + 1);

    MessageType type = stringToMessageType(type_str);
    NetworkMessage msg(type, payload);
    msg.timestamp = getCurrentTimestamp();
    return msg;
}

// Helper methods for JSON creation
std::string JsonHandler::createJsonString(const std::string& key, const std::string& value) {
    return "\"" + key + "\":\"" + escapeJsonString(value) + "\"";
}

std::string JsonHandler::createJsonBool(const std::string& key, bool value) {
    return "\"" + key + "\":" + (value ? "true" : "false");
}

std::string JsonHandler::createJsonDouble(const std::string& key, double value) {
    std::stringstream ss;
    ss << "\"" << key << "\":" << std::fixed << std::setprecision(2) << value;
    return ss.str();
}

std::string JsonHandler::createJsonInt(const std::string& key, int value) {
    return "\"" + key + "\":" + std::to_string(value);
}

// Escape JSON strings
std::string JsonHandler::escapeJsonString(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

// Helper methods for JSON parsing
std::string JsonHandler::extractJsonValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\":\"";
    size_t start = json.find(search_key);
    if (start == std::string::npos) {
        return "";
    }

    start += search_key.length();

    // Find the closing quote, handling escaped quotes
    size_t end = start;
    while (end < json.length()) {
        if (json[end] == '"' && (end == start || json[end-1] != '\\')) {
            break;
        }
        end++;
    }

    if (end >= json.length()) return "";

    std::string value = json.substr(start, end - start);

    // Unescape the value
    std::string unescaped;
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '\\' && i + 1 < value.length()) {
            switch (value[i + 1]) {
                case '"': unescaped += '"'; i++; break;
                case '\\': unescaped += '\\'; i++; break;
                case 'n': unescaped += '\n'; i++; break;
                case 'r': unescaped += '\r'; i++; break;
                case 't': unescaped += '\t'; i++; break;
                default: unescaped += value[i]; break;
            }
        } else {
            unescaped += value[i];
        }
    }

    return unescaped;
}

bool JsonHandler::extractJsonBool(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\":";
    size_t start = json.find(search_key);
    if (start == std::string::npos) return false;

    start += search_key.length();
    return json.substr(start, 4) == "true";
}

double JsonHandler::extractJsonDouble(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\":";
    size_t start = json.find(search_key);
    if (start == std::string::npos) return 0.0;

    start += search_key.length();
    size_t end = json.find_first_of(",}", start);
    if (end == std::string::npos) return 0.0;

    try {
        return std::stod(json.substr(start, end - start));
    } catch (...) {
        return 0.0;
    }
}

int JsonHandler::extractJsonInt(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\":";
    size_t start = json.find(search_key);
    if (start == std::string::npos) return 0;

    start += search_key.length();
    size_t end = json.find_first_of(",}", start);
    if (end == std::string::npos) return 0;

    try {
        return std::stoi(json.substr(start, end - start));
    } catch (...) {
        return 0;
    }
}
