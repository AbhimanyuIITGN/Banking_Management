#include "User.h"
#include "Security.h"
#include "DatabaseHandler.h"
#include <iostream>
#include <regex>

// Default constructor
User::User() : user_id(0), name(""), email(""), password_hash("") {}

// Parameterized constructor
User::User(int id, const std::string& name, const std::string& email, const std::string& password_hash)
    : user_id(id), name(name), email(email), password_hash(password_hash) {}

// Destructor
User::~User() {}

// Getters
int User::getUserId() const {
    return user_id;
}

std::string User::getName() const {
    return name;
}

std::string User::getEmail() const {
    return email;
}

std::string User::getPasswordHash() const {
    return password_hash;
}

// Setters
void User::setUserId(int id) {
    user_id = id;
}

void User::setName(const std::string& name) {
    this->name = name;
}

void User::setEmail(const std::string& email) {
    this->email = email;
}

void User::setPasswordHash(const std::string& hash) {
    this->password_hash = hash;
}

// Hash password using Security class
std::string User::hashPassword(const std::string& password) {
    return Security::hashPassword(password);
}

// Verify password
bool User::verifyPassword(const std::string& password) const {
    // Use the same salt as during registration
    std::string computed_hash = Security::hashPassword(password, "default_salt_2024");
    return computed_hash == password_hash;
}

// Login method
bool User::login(const std::string& email, const std::string& password) {
    try {
        DatabaseHandler& db = DatabaseHandler::getInstance();
        auto user = db.getUserByEmail(email);
        
        if (!user) {
            Security::logFailedLogin(email);
            return false;
        }
        
        if (!user->verifyPassword(password)) {
            Security::logFailedLogin(email);
            return false;
        }
        
        // Update this user's data
        this->user_id = user->getUserId();
        this->name = user->getName();
        this->email = user->getEmail();
        this->password_hash = user->getPasswordHash();
        
        Security::logSuccessfulLogin(email);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Login error: " << e.what() << std::endl;
        return false;
    }
}

// Register new user
std::shared_ptr<User> User::registerUser(const std::string& name, 
                                       const std::string& email, 
                                       const std::string& password) {
    try {
        // Validate input
        if (!isValidEmail(email)) {
            std::cerr << "Invalid email format" << std::endl;
            return nullptr;
        }
        
        if (!isValidPassword(password)) {
            std::cerr << "Password does not meet requirements" << std::endl;
            return nullptr;
        }
        
        if (name.empty() || name.length() > 100) {
            std::cerr << "Invalid name" << std::endl;
            return nullptr;
        }
        
        DatabaseHandler& db = DatabaseHandler::getInstance();
        
        // Check if email already exists
        auto existing_user = db.getUserByEmail(email);
        if (existing_user) {
            std::cerr << "Email already registered" << std::endl;
            return nullptr;
        }
        
        // Create new user
        std::string hashed_password = hashPassword(password);
        int new_user_id = db.getNextUserId();
        
        auto new_user = std::make_shared<User>(new_user_id, name, email, hashed_password);
        
        if (db.insertUser(*new_user)) {
            Security::logSecurityEvent("User registered: " + email);
            return new_user;
        }
        
        return nullptr;
    }
    catch (const std::exception& e) {
        std::cerr << "Registration error: " << e.what() << std::endl;
        return nullptr;
    }
}

// Email validation
bool User::isValidEmail(const std::string& email) {
    return Security::isValidEmail(email);
}

// Password validation
bool User::isValidPassword(const std::string& password) {
    return Security::isValidPassword(password);
}

// Display user information
void User::displayUserInfo() const {
    std::cout << "=== User Information ===" << std::endl;
    std::cout << "User ID: " << user_id << std::endl;
    std::cout << "Name: " << name << std::endl;
    std::cout << "Email: " << email << std::endl;
    std::cout << "========================" << std::endl;
}
