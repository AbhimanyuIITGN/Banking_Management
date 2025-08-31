#ifndef BANK_SYSTEM_H
#define BANK_SYSTEM_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include "User.h"
#include "Account.h"
#include "Transaction.h"
#include "DatabaseHandler.h"
#include "DeadlockPrevention.h"

class BankSystem {
private:
    static std::unique_ptr<BankSystem> instance;
    static std::mutex instance_mutex;
    
    DatabaseHandler& db_handler;
    DeadlockPrevention deadlock_manager;
    
    // In-memory caches for performance
    std::unordered_map<int, std::shared_ptr<User>> user_cache;
    std::unordered_map<int, std::shared_ptr<Account>> account_cache;

    // Transaction cache for immediate history access
    std::unordered_map<int, std::vector<std::shared_ptr<Transaction>>> transaction_cache;
    
    mutable std::mutex system_mutex;
    mutable std::mutex user_cache_mutex;
    mutable std::mutex account_cache_mutex;
    
    // Current logged-in user
    std::shared_ptr<User> current_user;
    
    // System statistics
    int total_users;
    int total_accounts;
    int total_transactions;
    double total_system_balance;

    // Private constructor for singleton
    BankSystem();

public:
    // Destructor
    ~BankSystem();

    // Singleton pattern
    static BankSystem& getInstance();
    
    // Delete copy constructor and assignment operator
    BankSystem(const BankSystem&) = delete;
    BankSystem& operator=(const BankSystem&) = delete;

    // System initialization
    bool initialize();
    void shutdown();

    // User management
    bool registerUser(const std::string& name, const std::string& email, const std::string& password);
    bool loginUser(const std::string& email, const std::string& password);
    bool authenticateUser(const std::string& email, const std::string& password);
    void logoutUser();
    bool logout(); // Alias for logoutUser for compatibility
    std::shared_ptr<User> getCurrentUser() const;
    bool isUserLoggedIn() const;

    // Account management
    int createAccount(AccountType type, double initial_balance = 0.0);
    std::shared_ptr<Account> getAccount(int account_id);
    std::vector<std::shared_ptr<Account>> getUserAccounts();
    std::vector<std::shared_ptr<Account>> getUserAccounts(int user_id);
    bool deleteAccount(int account_id);

    // Banking operations
    bool deposit(int account_id, double amount);
    bool withdraw(int account_id, double amount);
    bool transfer(int from_account_id, int to_account_id, double amount);
    
    // Transaction history
    std::vector<std::shared_ptr<Transaction>> getAccountTransactions(int account_id);
    std::vector<std::shared_ptr<Transaction>> getUserTransactions();
    std::vector<std::shared_ptr<Transaction>> getUserTransactions(int user_id);

    // System operations
    void displaySystemStats() const;
    void displayAllAccounts() const;
    void displayAllUsers() const;
    
    // Interest operations
    void applyInterestToAllSavingsAccounts(double annual_rate);
    void runMonthlyInterestJob();

    // Cache management
    void refreshUserCache();
    void refreshAccountCache();
    void clearCaches();

    // Validation methods
    bool validateAccountOwnership(int account_id, int user_id) const;
    bool accountExists(int account_id) const;
    bool userExists(int user_id) const;

    // Concurrent operation support
    void processTransactionsConcurrently(const std::vector<std::shared_ptr<Transaction>>& transactions);

    // Deadlock management
    DeadlockPrevention& getDeadlockManager() { return deadlock_manager; }

    // Admin operations
    bool isAdmin() const;
    void generateSystemReport() const;
    void backupSystem(const std::string& backup_path) const;

private:
    // Helper methods
    void updateSystemStats();
    void logSystemEvent(const std::string& event) const;
    bool validateTransactionLimits(double amount, AccountType type) const;
    
    // Cache helper methods
    void addToUserCache(std::shared_ptr<User> user);
    void addToAccountCache(std::shared_ptr<Account> account);
    void removeFromUserCache(int user_id);
    void removeFromAccountCache(int account_id);
};

#endif // BANK_SYSTEM_H
