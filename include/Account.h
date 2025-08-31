#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>
#include <mutex>
#include <memory>
#include <vector>
#include "Common.h"

class Account {
private:
    int account_id;
    int user_id;
    double balance;
    AccountType account_type;
    mutable std::mutex account_mutex;
    std::string created_at;

public:
    // Constructors
    Account();
    Account(int account_id, int user_id, double initial_balance, AccountType type);
    
    // Destructor
    ~Account();

    // Copy constructor and assignment operator (deleted due to mutex)
    Account(const Account&) = delete;
    Account& operator=(const Account&) = delete;

    // Move constructor and assignment operator
    Account(Account&& other) noexcept;
    Account& operator=(Account&& other) noexcept;

    // Getters
    int getAccountId() const;
    int getUserId() const;
    double getBalance() const;
    AccountType getAccountType() const;
    std::string getCreatedAt() const;

    // Setters
    void setAccountId(int id);
    void setUserId(int user_id);
    void setAccountType(AccountType type);
    void setBalance(double new_balance);

    // Core banking operations
    TransactionStatus deposit(double amount);
    TransactionStatus withdraw(double amount);
    TransactionStatus transfer(std::shared_ptr<Account> to_account, double amount);

    // Balance operations
    bool hasSufficientBalance(double amount) const;
    void updateBalance(double new_balance);

    // Locking mechanisms
    void lock() const;
    void unlock() const;
    bool try_lock() const;
    std::mutex& getMutex() const;

    // Validation methods
    static bool isValidAmount(double amount);
    bool canWithdraw(double amount) const;

    // Display methods
    void displayAccountInfo() const;
    std::string getAccountTypeString() const;

    // Interest calculation (for savings accounts)
    double calculateInterest(double rate, int days) const;
    void applyInterest(double rate);

    // Transaction history
    std::vector<int> getTransactionHistory() const;
};

#endif // ACCOUNT_H
