#ifndef SECURITY_H
#define SECURITY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>

class Security {
public:
    // Password hashing and verification
    static std::string hashPassword(const std::string& password, const std::string& salt = "");
    static std::string generateSalt();
    static bool verifyPassword(const std::string& password, const std::string& hash, const std::string& salt = "");
    
    // Input validation and sanitization
    static bool isValidEmail(const std::string& email);
    static bool isValidPassword(const std::string& password);
    static bool isValidName(const std::string& name);
    static bool isValidAmount(double amount);
    static std::string sanitizeInput(const std::string& input);
    
    // SQL injection prevention
    static std::string escapeSQLString(const std::string& input);
    static bool containsSQLInjection(const std::string& input);
    
    // Session management
    static std::string generateSessionToken();
    static bool isValidSessionToken(const std::string& token);
    
    // Rate limiting
    static bool checkRateLimit(const std::string& identifier, int max_attempts = 5, int time_window_minutes = 15);
    static void resetRateLimit(const std::string& identifier);
    
    // Audit logging
    static void logSecurityEvent(const std::string& event, const std::string& user_info = "");
    static void logFailedLogin(const std::string& email, const std::string& ip = "localhost");
    static void logSuccessfulLogin(const std::string& email, const std::string& ip = "localhost");
    
    // Encryption/Decryption (for sensitive data)
    static std::string encrypt(const std::string& plaintext, const std::string& key);
    static std::string decrypt(const std::string& ciphertext, const std::string& key);
    
    // Random number generation
    static int generateRandomNumber(int min, int max);
    static std::string generateRandomString(int length);

private:
    // Internal helper methods
    static std::string sha256(const std::string& input);
    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);
    
    // Rate limiting storage (simplified)
    static std::mutex rate_limit_mutex;
};

#endif // SECURITY_H
