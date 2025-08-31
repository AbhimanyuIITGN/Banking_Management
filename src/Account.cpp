#include "Account.h"
#include "DatabaseHandler.h"
#include "Security.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <fstream>
#include <mutex>

// Default constructor
Account::Account() : account_id(0), user_id(0), balance(0.0), account_type(AccountType::SAVINGS) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    created_at = ss.str();
}

// Parameterized constructor
Account::Account(int account_id, int user_id, double initial_balance, AccountType type)
    : account_id(account_id), user_id(user_id), balance(initial_balance), account_type(type) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    created_at = ss.str();
}

// Destructor
Account::~Account() {}

// Move constructor
Account::Account(Account&& other) noexcept
    : account_id(other.account_id), user_id(other.user_id), 
      balance(other.balance), account_type(other.account_type),
      created_at(std::move(other.created_at)) {
    other.account_id = 0;
    other.user_id = 0;
    other.balance = 0.0;
}

// Move assignment operator
Account& Account::operator=(Account&& other) noexcept {
    if (this != &other) {
        account_id = other.account_id;
        user_id = other.user_id;
        balance = other.balance;
        account_type = other.account_type;
        created_at = std::move(other.created_at);
        
        other.account_id = 0;
        other.user_id = 0;
        other.balance = 0.0;
    }
    return *this;
}

// Getters
int Account::getAccountId() const {
    return account_id;
}

int Account::getUserId() const {
    return user_id;
}

double Account::getBalance() const {
    std::lock_guard<std::mutex> lock(account_mutex);
    return balance;
}

AccountType Account::getAccountType() const {
    return account_type;
}

std::string Account::getCreatedAt() const {
    return created_at;
}

// Setters
void Account::setAccountId(int id) {
    account_id = id;
}

void Account::setUserId(int user_id) {
    this->user_id = user_id;
}

void Account::setAccountType(AccountType type) {
    account_type = type;
}

void Account::setBalance(double new_balance) {
    std::lock_guard<std::mutex> lock(account_mutex);
    balance = new_balance;
}

// Deposit operation
TransactionStatus Account::deposit(double amount) {
    if (!isValidAmount(amount)) {
        std::cerr << "Invalid deposit amount: $" << amount << std::endl;
        return TransactionStatus::FAILED;
    }

    std::lock_guard<std::mutex> lock(account_mutex);

    try {
        double old_balance = balance;
        balance += amount;
        std::cout << "Deposit processed: $" << amount << " added to account " << account_id << std::endl;
        std::cout << "New balance: $" << std::fixed << std::setprecision(2) << balance << std::endl;

        // Sync balance using file-based synchronization (reliable across terminals)
        std::cout << "Syncing account balance..." << std::endl;

        // Use file-based sync to avoid database hanging issues
        // This ensures synchronization across multiple terminal instances
        std::ofstream sync_file("account_" + std::to_string(account_id) + "_balance.sync");
        if (sync_file.is_open()) {
            sync_file << std::fixed << std::setprecision(2) << balance;
            sync_file.close();
            std::cout << "Balance synchronized successfully" << std::endl;
        } else {
            std::cout << "Balance sync warning (continuing with operation)" << std::endl;
        }

        return TransactionStatus::SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Deposit error: " << e.what() << std::endl;
        return TransactionStatus::FAILED;
    }
}

// Withdraw operation
TransactionStatus Account::withdraw(double amount) {
    if (!isValidAmount(amount)) {
        std::cerr << "Invalid withdrawal amount: $" << amount << std::endl;
        return TransactionStatus::FAILED;
    }

    std::lock_guard<std::mutex> lock(account_mutex);

    if (!hasSufficientBalance(amount)) {
        std::cerr << "Insufficient balance. Current balance: $" << balance << std::endl;
        return TransactionStatus::FAILED;
    }

    try {
        double old_balance = balance;
        balance -= amount;
        std::cout << "Withdrawal processed: $" << amount << " from account " << account_id << std::endl;
        std::cout << "New balance: $" << std::fixed << std::setprecision(2) << balance << std::endl;

        // Sync balance using file-based synchronization (reliable across terminals)
        std::cout << "Syncing account balance..." << std::endl;

        // Use file-based sync to avoid database hanging issues
        std::ofstream sync_file("account_" + std::to_string(account_id) + "_balance.sync");
        if (sync_file.is_open()) {
            sync_file << std::fixed << std::setprecision(2) << balance;
            sync_file.close();
            std::cout << "Balance synchronized successfully" << std::endl;
        } else {
            std::cout << "Balance sync warning (continuing with operation)" << std::endl;
        }

        return TransactionStatus::SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Withdrawal error: " << e.what() << std::endl;
        return TransactionStatus::FAILED;
    }
}

// Transfer operation (simplified to avoid hanging)
TransactionStatus Account::transfer(std::shared_ptr<Account> to_account, double amount) {
    if (!to_account || !isValidAmount(amount)) {
        std::cerr << "Invalid transfer parameters" << std::endl;
        return TransactionStatus::FAILED;
    }

    if (account_id == to_account->getAccountId()) {
        std::cerr << "Cannot transfer to the same account" << std::endl;
        return TransactionStatus::FAILED;
    }

    // Lock ordering to prevent deadlock (always lock smaller account_id first)
    // This provides defense-in-depth along with BankSystem-level deadlock prevention
    Account* first_lock = (account_id < to_account->getAccountId()) ? this : to_account.get();
    Account* second_lock = (account_id < to_account->getAccountId()) ? to_account.get() : this;

    std::lock_guard<std::mutex> lock1(first_lock->account_mutex);
    std::lock_guard<std::mutex> lock2(second_lock->account_mutex);

    if (!hasSufficientBalance(amount)) {
        std::cerr << "Insufficient balance for transfer. Current balance: $" << balance << std::endl;
        return TransactionStatus::FAILED;
    }

    try {
        // Store original balances for rollback
        double old_from_balance = balance;
        double old_to_balance = to_account->balance;

        // Perform transfer
        balance -= amount;
        to_account->balance += amount;

        std::cout << "Transfer processed: $" << amount << " from account " << account_id
                  << " to account " << to_account->getAccountId() << std::endl;
        std::cout << "Source account new balance: $" << std::fixed << std::setprecision(2) << balance << std::endl;
        std::cout << "Destination account new balance: $" << std::fixed << std::setprecision(2) << to_account->balance << std::endl;

        // Sync balances using file-based synchronization (reliable across terminals)
        std::cout << "Syncing account balances..." << std::endl;

        // Sync both account balances to files
        std::ofstream from_sync("account_" + std::to_string(account_id) + "_balance.sync");
        std::ofstream to_sync("account_" + std::to_string(to_account->getAccountId()) + "_balance.sync");

        bool sync_success = true;
        if (from_sync.is_open()) {
            from_sync << std::fixed << std::setprecision(2) << balance;
            from_sync.close();
        } else {
            sync_success = false;
        }

        if (to_sync.is_open()) {
            to_sync << std::fixed << std::setprecision(2) << to_account->balance;
            to_sync.close();
        } else {
            sync_success = false;
        }

        if (sync_success) {
            std::cout << "Balances synchronized successfully" << std::endl;
        } else {
            std::cout << "Balance sync warning (continuing with operation)" << std::endl;
        }

        return TransactionStatus::SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Transfer error: " << e.what() << std::endl;
        balance += amount;
        to_account->balance -= amount;
        return TransactionStatus::FAILED;
    }
}

// Check sufficient balance
bool Account::hasSufficientBalance(double amount) const {
    return balance >= amount;
}

// Update balance (internal use)
void Account::updateBalance(double new_balance) {
    std::lock_guard<std::mutex> lock(account_mutex);
    balance = new_balance;
}

// Locking mechanisms
void Account::lock() const {
    account_mutex.lock();
}

void Account::unlock() const {
    account_mutex.unlock();
}

bool Account::try_lock() const {
    return account_mutex.try_lock();
}

std::mutex& Account::getMutex() const {
    return account_mutex;
}

// Validate amount
bool Account::isValidAmount(double amount) {
    return Security::isValidAmount(amount);
}

// Check if withdrawal is allowed
bool Account::canWithdraw(double amount) const {
    std::lock_guard<std::mutex> lock(account_mutex);
    return hasSufficientBalance(amount);
}

// Display account information
void Account::displayAccountInfo() const {
    std::lock_guard<std::mutex> lock(account_mutex);
    std::cout << "=== Account Information ===" << std::endl;
    std::cout << "Account ID: " << account_id << std::endl;
    std::cout << "User ID: " << user_id << std::endl;
    std::cout << "Account Type: " << getAccountTypeString() << std::endl;
    std::cout << "Balance: $" << std::fixed << std::setprecision(2) << balance << std::endl;
    std::cout << "Created: " << created_at << std::endl;
    std::cout << "===========================" << std::endl;
}

// Get account type as string
std::string Account::getAccountTypeString() const {
    switch (account_type) {
        case AccountType::SAVINGS:
            return "SAVINGS";
        case AccountType::CURRENT:
            return "CURRENT";
        default:
            return "UNKNOWN";
    }
}

// Calculate interest for savings accounts
double Account::calculateInterest(double rate, int days) const {
    if (account_type != AccountType::SAVINGS) {
        return 0.0;
    }
    
    std::lock_guard<std::mutex> lock(account_mutex);
    return (balance * rate * days) / 365.0;
}

// Apply interest to account
void Account::applyInterest(double rate) {
    if (account_type != AccountType::SAVINGS) {
        return;
    }
    
    double interest = calculateInterest(rate, 30); // Monthly interest
    deposit(interest);
}

// Get transaction history
std::vector<int> Account::getTransactionHistory() const {
    try {
        DatabaseHandler& db = DatabaseHandler::getInstance();
        auto transactions = db.getTransactionsByAccountId(account_id);
        
        std::vector<int> transaction_ids;
        for (const auto& txn : transactions) {
            transaction_ids.push_back(txn->getTransactionId());
        }
        
        return transaction_ids;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting transaction history: " << e.what() << std::endl;
        return {};
    }
}
