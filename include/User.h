#ifndef USER_H
#define USER_H

#include <string>
#include <memory>

class User {
private:
    int user_id;
    std::string name;
    std::string email;
    std::string password_hash;

public:
    // Constructors
    User();
    User(int id, const std::string& name, const std::string& email, const std::string& password_hash);
    
    // Destructor
    ~User();

    // Getters
    int getUserId() const;
    std::string getName() const;
    std::string getEmail() const;
    std::string getPasswordHash() const;

    // Setters
    void setUserId(int id);
    void setName(const std::string& name);
    void setEmail(const std::string& email);
    void setPasswordHash(const std::string& hash);

    // Authentication methods
    static std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password) const;
    
    // User operations
    bool login(const std::string& email, const std::string& password);
    static std::shared_ptr<User> registerUser(const std::string& name, 
                                            const std::string& email, 
                                            const std::string& password);
    
    // Validation methods
    static bool isValidEmail(const std::string& email);
    static bool isValidPassword(const std::string& password);
    
    // Display methods
    void displayUserInfo() const;
};

#endif // USER_H
