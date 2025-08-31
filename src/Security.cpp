#include "Security.h"
#include <iostream>
#include <regex>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <functional>
#include <algorithm>

// Static member initialization (simplified)
std::mutex Security::rate_limit_mutex;

// Hash password with salt (simplified version for demonstration)
std::string Security::hashPassword(const std::string& password, const std::string& salt) {
    std::string salted_password = password + (salt.empty() ? "default_salt_2024" : salt);

    // Simple hash function for demonstration (NOT for production use)
    size_t hash_value = 0;
    for (char c : salted_password) {
        hash_value = hash_value * 31 + static_cast<size_t>(c);
    }

    std::stringstream ss;
    ss << std::hex << hash_value;

    return ss.str();
}

// Generate random salt
std::string Security::generateSalt() {
    return generateRandomString(16);
}

// Verify password against hash
bool Security::verifyPassword(const std::string& password, const std::string& hash, const std::string& salt) {
    std::string computed_hash = hashPassword(password, salt);
    return computed_hash == hash;
}

// Validate email format
bool Security::isValidEmail(const std::string& email) {
    const std::regex email_pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, email_pattern) && email.length() <= 150;
}

// Validate password strength
bool Security::isValidPassword(const std::string& password) {
    if (password.length() < 8 || password.length() > 128) {
        return false;
    }
    
    bool has_upper = false, has_lower = false, has_digit = false, has_special = false;
    
    for (char c : password) {
        if (c >= 'A' && c <= 'Z') has_upper = true;
        else if (c >= 'a' && c <= 'z') has_lower = true;
        else if (c >= '0' && c <= '9') has_digit = true;
        else if (c == '!' || c == '@' || c == '#' || c == '$' || c == '%' || 
                 c == '^' || c == '&' || c == '*' || c == '(' || c == ')' ||
                 c == '-' || c == '_' || c == '+' || c == '=') has_special = true;
    }
    
    return has_upper && has_lower && has_digit && has_special;
}

// Validate name
bool Security::isValidName(const std::string& name) {
    if (name.empty() || name.length() > 100) {
        return false;
    }
    
    const std::regex name_pattern(R"([a-zA-Z\s\-\.]+)");
    return std::regex_match(name, name_pattern);
}

// Validate amount
bool Security::isValidAmount(double amount) {
    return amount > 0 && amount <= 1000000.0; // Max transaction limit
}

// Sanitize input
std::string Security::sanitizeInput(const std::string& input) {
    std::string sanitized = input;
    
    // Remove or escape dangerous characters
    std::regex dangerous_chars(R"([<>\"'&;])");
    sanitized = std::regex_replace(sanitized, dangerous_chars, "");
    
    // Trim whitespace
    sanitized.erase(0, sanitized.find_first_not_of(" \t\n\r\f\v"));
    sanitized.erase(sanitized.find_last_not_of(" \t\n\r\f\v") + 1);
    
    return sanitized;
}

// Escape SQL string
std::string Security::escapeSQLString(const std::string& input) {
    std::string escaped = input;
    
    // Replace single quotes with double single quotes
    size_t pos = 0;
    while ((pos = escaped.find("'", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "''");
        pos += 2;
    }
    
    return escaped;
}

// Check for SQL injection patterns
bool Security::containsSQLInjection(const std::string& input) {
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    std::vector<std::string> sql_keywords = {
        "select", "insert", "update", "delete", "drop", "create", "alter",
        "union", "or", "and", "where", "having", "group by", "order by",
        "exec", "execute", "sp_", "xp_", "--", "/*", "*/"
    };
    
    for (const auto& keyword : sql_keywords) {
        if (lower_input.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

// Generate session token
std::string Security::generateSessionToken() {
    return generateRandomString(64);
}

// Validate session token format
bool Security::isValidSessionToken(const std::string& token) {
    return token.length() == 64 && 
           std::all_of(token.begin(), token.end(), 
                      [](char c) { return std::isalnum(c); });
}

// Rate limiting check (simplified version)
bool Security::checkRateLimit(const std::string& identifier, int max_attempts, int time_window_minutes) {
    (void)identifier; (void)max_attempts; (void)time_window_minutes; // Suppress warnings
    std::lock_guard<std::mutex> lock(rate_limit_mutex);

    // Simplified rate limiting - always allow for demonstration
    // In production, implement proper rate limiting with persistent storage
    return true;
}

// Reset rate limit for identifier (simplified)
void Security::resetRateLimit(const std::string& identifier) {
    (void)identifier; // Suppress warning
    std::lock_guard<std::mutex> lock(rate_limit_mutex);
    // Simplified - no action needed
}

// Log security events
void Security::logSecurityEvent(const std::string& event, const std::string& user_info) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::cout << "[SECURITY] " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
              << " - " << event;
    if (!user_info.empty()) {
        std::cout << " - User: " << user_info;
    }
    std::cout << std::endl;
}

// Log failed login
void Security::logFailedLogin(const std::string& email, const std::string& ip) {
    logSecurityEvent("Failed login attempt", "Email: " + email + ", IP: " + ip);
}

// Log successful login
void Security::logSuccessfulLogin(const std::string& email, const std::string& ip) {
    logSecurityEvent("Successful login", "Email: " + email + ", IP: " + ip);
}

// Simple encryption (for demonstration - use proper encryption in production)
std::string Security::encrypt(const std::string& plaintext, const std::string& key) {
    std::string encrypted = plaintext;
    for (size_t i = 0; i < encrypted.length(); ++i) {
        encrypted[i] ^= key[i % key.length()];
    }
    return base64Encode(encrypted);
}

// Simple decryption
std::string Security::decrypt(const std::string& ciphertext, const std::string& key) {
    std::string decoded = base64Decode(ciphertext);
    for (size_t i = 0; i < decoded.length(); ++i) {
        decoded[i] ^= key[i % key.length()];
    }
    return decoded;
}

// Generate random number
int Security::generateRandomNumber(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

// Generate random string
std::string Security::generateRandomString(int length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(length);
    
    for (int i = 0; i < length; ++i) {
        result += chars[generateRandomNumber(0, chars.length() - 1)];
    }
    
    return result;
}

// Simple hash function (simplified for demonstration)
std::string Security::sha256(const std::string& input) {
    // Simple hash function for demonstration (NOT for production use)
    size_t hash_value = 0;
    for (char c : input) {
        hash_value = hash_value * 31 + static_cast<size_t>(c);
    }

    std::stringstream ss;
    ss << std::hex << hash_value;

    return ss.str();
}

// Base64 encoding (simplified)
std::string Security::base64Encode(const std::string& input) {
    // Simplified base64 encoding for demonstration
    // In production, use a proper base64 library
    return input; // Placeholder
}

// Base64 decoding (simplified)
std::string Security::base64Decode(const std::string& input) {
    // Simplified base64 decoding for demonstration
    // In production, use a proper base64 library
    return input; // Placeholder
}
