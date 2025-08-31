#include "Encryption.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cctype>

const std::string Encryption::DEFAULT_KEY = "BankingSystem2024SecureKey!@#";
const std::string Encryption::BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// XOR encryption
std::string Encryption::xorEncrypt(const std::string& data, const std::string& key) {
    std::string result = data;
    size_t key_len = key.length();
    
    for (size_t i = 0; i < data.length(); ++i) {
        result[i] = data[i] ^ key[i % key_len];
    }
    
    return result;
}

// XOR decryption (same as encryption for XOR)
std::string Encryption::xorDecrypt(const std::string& data, const std::string& key) {
    return xorEncrypt(data, key);
}

// Base64 encoding
std::string Encryption::base64Encode(const std::string& data) {
    std::string encoded;
    int val = 0, valb = -6;
    
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        encoded.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    
    return encoded;
}

// Base64 decoding
std::string Encryption::base64Decode(const std::string& data) {
    std::string decoded;
    int val = 0, valb = -8;
    
    for (unsigned char c : data) {
        if (!isBase64(c)) break;
        
        val = (val << 6) + BASE64_CHARS.find(c);
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return decoded;
}

// Combined encrypt and encode
std::string Encryption::encryptAndEncode(const std::string& data, const std::string& key) {
    std::string encrypted = xorEncrypt(data, key);
    return base64Encode(encrypted);
}

// Combined decode and decrypt
std::string Encryption::decodeAndDecrypt(const std::string& data, const std::string& key) {
    std::string decoded = base64Decode(data);
    return xorDecrypt(decoded, key);
}

// Generate session token
std::string Encryption::generateSessionToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::string token;
    for (int i = 0; i < 32; ++i) {
        token += static_cast<char>(dis(gen));
    }
    
    return base64Encode(token);
}

// Validate session token
bool Encryption::isValidSessionToken(const std::string& token) {
    if (token.empty() || token.length() < 16) {
        return false;
    }
    
    // Check if it's valid base64
    for (char c : token) {
        if (!isBase64(c) && c != '=') {
            return false;
        }
    }
    
    return true;
}

// Check if character is valid base64
bool Encryption::isBase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}
