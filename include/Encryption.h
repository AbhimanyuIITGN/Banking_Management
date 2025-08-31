#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>
#include <vector>

class Encryption {
private:
    static const std::string DEFAULT_KEY;
    
public:
    // XOR-based encryption/decryption
    static std::string xorEncrypt(const std::string& data, const std::string& key = DEFAULT_KEY);
    static std::string xorDecrypt(const std::string& data, const std::string& key = DEFAULT_KEY);
    
    // Base64 encoding/decoding for safe transmission
    static std::string base64Encode(const std::string& data);
    static std::string base64Decode(const std::string& data);
    
    // Combined encrypt and encode for network transmission
    static std::string encryptAndEncode(const std::string& data, const std::string& key = DEFAULT_KEY);
    static std::string decodeAndDecrypt(const std::string& data, const std::string& key = DEFAULT_KEY);
    
    // Generate session tokens
    static std::string generateSessionToken();
    
    // Validate session tokens
    static bool isValidSessionToken(const std::string& token);
    
private:
    // Base64 character set
    static const std::string BASE64_CHARS;
    
    // Helper functions
    static bool isBase64(unsigned char c);
};

#endif // ENCRYPTION_H
