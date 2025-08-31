#include "BankSystem.h"
#include "Security.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <fstream>
#include <sstream>
#include <future>

// Static member initialization
std::unique_ptr<BankSystem> BankSystem::instance = nullptr;
std::mutex BankSystem::instance_mutex;

// Private constructor
BankSystem::BankSystem() 
    : db_handler(DatabaseHandler::getInstance()),
      deadlock_manager(DeadlockStrategy::LOCK_ORDERING),
      current_user(nullptr),
      total_users(0), total_accounts(0), total_transactions(0), total_system_balance(0.0) {}

// Destructor
BankSystem::~BankSystem() {
    shutdown();
}

// Singleton getInstance
BankSystem& BankSystem::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (!instance) {
        instance = std::unique_ptr<BankSystem>(new BankSystem());
    }
    return *instance;
}

// Initialize system
bool BankSystem::initialize() {
    std::lock_guard<std::mutex> lock(system_mutex);
    
    try {
        // Connect to database
        if (!db_handler.connect()) {
            std::cerr << "Failed to connect to database" << std::endl;
            return false;
        }
        
        // Initialize caches
        refreshUserCache();
        refreshAccountCache();
        updateSystemStats();
        
        std::cout << "Banking System initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "System initialization error: " << e.what() << std::endl;
        return false;
    }
}

// Shutdown system
void BankSystem::shutdown() {
    std::lock_guard<std::mutex> lock(system_mutex);
    
    logoutUser();
    clearCaches();
    db_handler.disconnect();
    
    std::cout << "Banking System shutdown complete" << std::endl;
}

// Register new user
bool BankSystem::registerUser(const std::string& name, const std::string& email, const std::string& password) {
    try {
        auto new_user = User::registerUser(name, email, password);
        if (new_user) {
            addToUserCache(new_user);
            updateSystemStats();
            return true;
        }
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "User registration error: " << e.what() << std::endl;
        return false;
    }
}

// Login user
bool BankSystem::loginUser(const std::string& email, const std::string& password) {
    try {
        // Check rate limiting
        if (!Security::checkRateLimit(email, 5, 15)) {
            std::cerr << "Too many login attempts. Please try again later." << std::endl;
            return false;
        }

        // Get user from database first
        auto user = db_handler.getUserByEmail(email);
        if (!user) {
            Security::logFailedLogin(email);
            return false;
        }

        // Verify password
        if (user->verifyPassword(password)) {
            std::lock_guard<std::mutex> lock(system_mutex);
            current_user = user;
            Security::resetRateLimit(email);
            Security::logSuccessfulLogin(email);
            std::cout << "Login successful. Welcome, " << user->getName() << "!" << std::endl;
            return true;
        } else {
            Security::logFailedLogin(email);
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Login error: " << e.what() << std::endl;
        return false;
    }
}

// Logout user
void BankSystem::logoutUser() {
    std::lock_guard<std::mutex> lock(system_mutex);
    if (current_user) {
        std::cout << "Goodbye, " << current_user->getName() << "!" << std::endl;
        current_user = nullptr;
    }
}

// Authenticate user (for ATM server)
bool BankSystem::authenticateUser(const std::string& email, const std::string& password) {
    try {
        auto user = db_handler.getUserByEmail(email);
        if (user && Security::verifyPassword(password, user->getPasswordHash())) {
            std::lock_guard<std::mutex> lock(system_mutex);
            current_user = user;
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Authentication error: " << e.what() << std::endl;
        return false;
    }
}

// Logout alias for compatibility
bool BankSystem::logout() {
    logoutUser();
    return true;
}

// Get current user
std::shared_ptr<User> BankSystem::getCurrentUser() const {
    std::lock_guard<std::mutex> lock(system_mutex);
    return current_user;
}

// Check if user is logged in
bool BankSystem::isUserLoggedIn() const {
    std::lock_guard<std::mutex> lock(system_mutex);
    return current_user != nullptr;
}

// Create account
int BankSystem::createAccount(AccountType type, double initial_balance) {
    if (!isUserLoggedIn()) {
        std::cerr << "Please login first" << std::endl;
        return -1;
    }
    
    try {
        int account_id = db_handler.getNextAccountId();
        auto account = std::make_shared<Account>(account_id, current_user->getUserId(), initial_balance, type);
        
        if (db_handler.insertAccount(*account)) {
            addToAccountCache(account);
            updateSystemStats();
            std::cout << "Account created successfully. Account ID: " << account_id << std::endl;
            return account_id;
        }
        
        return -1;
    }
    catch (const std::exception& e) {
        std::cerr << "Account creation error: " << e.what() << std::endl;
        return -1;
    }
}

// Get account with file-based synchronization
std::shared_ptr<Account> BankSystem::getAccount(int account_id) {
    std::lock_guard<std::mutex> lock(account_cache_mutex);

    // First try to get from database
    auto account = db_handler.getAccountById(account_id);
    if (account) {
        // Check for synchronized balance from file
        std::ifstream sync_file("account_" + std::to_string(account_id) + "_balance.sync");
        if (sync_file.is_open()) {
            double synced_balance;
            if (sync_file >> synced_balance) {
                account->setBalance(synced_balance);
                std::cout << "Loaded synchronized balance: $" << synced_balance << " for account " << account_id << std::endl;
            }
            sync_file.close();
        }

        account_cache[account_id] = account;
        return account;
    }

    // Fallback to cache if database fails
    auto it = account_cache.find(account_id);
    if (it != account_cache.end()) {
        return it->second;
    }

    return nullptr;
}

// Get user accounts
std::vector<std::shared_ptr<Account>> BankSystem::getUserAccounts() {
    if (!isUserLoggedIn()) {
        return {};
    }
    
    return getUserAccounts(current_user->getUserId());
}

// Get user accounts by user ID with file synchronization
std::vector<std::shared_ptr<Account>> BankSystem::getUserAccounts(int user_id) {
    try {
        // Always fetch fresh data from database to ensure synchronization
        auto accounts = db_handler.getAccountsByUserId(user_id);

        // Update cache with fresh data and load synchronized balances
        std::lock_guard<std::mutex> lock(account_cache_mutex);
        for (const auto& account : accounts) {
            // Check for synchronized balance from file
            std::ifstream sync_file("account_" + std::to_string(account->getAccountId()) + "_balance.sync");
            if (sync_file.is_open()) {
                double synced_balance;
                if (sync_file >> synced_balance) {
                    account->setBalance(synced_balance);
                    std::cout << "Loaded synchronized balance: $" << synced_balance << " for account " << account->getAccountId() << std::endl;
                }
                sync_file.close();
            }

            account_cache[account->getAccountId()] = account;
        }

        return accounts;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting user accounts: " << e.what() << std::endl;
        return {};
    }
}

// Deposit operation
bool BankSystem::deposit(int account_id, double amount) {
    if (!isUserLoggedIn()) {
        std::cerr << "Please login first" << std::endl;
        return false;
    }

    if (!validateAccountOwnership(account_id, current_user->getUserId())) {
        std::cerr << "Account access denied" << std::endl;
        return false;
    }

    auto account = getAccount(account_id);
    if (!account) {
        std::cerr << "Account not found" << std::endl;
        return false;
    }

    // Perform deposit and record transaction
    if (account->deposit(amount) == TransactionStatus::SUCCESS) {
        // Record transaction in both memory and database
        try {
            auto transaction = Transaction::createDeposit(account_id, amount);
            transaction->setDescription("Deposit to account " + std::to_string(account_id));
            transaction->setStatus(TransactionStatus::SUCCESS);

            // Store in memory transaction cache for immediate access
            transaction_cache[account_id].push_back(transaction);
            total_transactions++;

            // Also sync to file for cross-terminal synchronization
            std::ofstream txn_file("transactions.sync", std::ios::app);
            if (txn_file.is_open()) {
                txn_file << transaction->getTransactionId() << "|"
                         << transaction->getFromAccountId() << "|"
                         << transaction->getToAccountId() << "|"
                         << std::fixed << std::setprecision(2) << transaction->getAmount() << "|"
                         << transaction->getTypeString() << "|"
                         << transaction->getStatusString() << "|"
                         << transaction->getDescription() << "|"
                         << transaction->getTimestamp() << std::endl;
                txn_file.close();
                std::cout << "Transaction recorded in synchronized history" << std::endl;
            } else {
                std::cout << "Transaction recorded in memory only" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to record transaction: " << e.what() << std::endl;
        }

        updateSystemStats();
        std::cout << "Deposit successful. New balance: $" << std::fixed << std::setprecision(2)
                  << account->getBalance() << std::endl;
        return true;
    }

    std::cerr << "Deposit failed" << std::endl;
    return false;
}

// Withdraw operation
bool BankSystem::withdraw(int account_id, double amount) {
    if (!isUserLoggedIn()) {
        std::cerr << "Please login first" << std::endl;
        return false;
    }

    if (!validateAccountOwnership(account_id, current_user->getUserId())) {
        std::cerr << "Account access denied" << std::endl;
        return false;
    }

    auto account = getAccount(account_id);
    if (!account) {
        std::cerr << "Account not found" << std::endl;
        return false;
    }

    // Perform withdrawal and record transaction
    if (account->withdraw(amount) == TransactionStatus::SUCCESS) {
        // Record transaction in both memory and database
        try {
            auto transaction = Transaction::createWithdrawal(account_id, amount);
            transaction->setDescription("Withdrawal from account " + std::to_string(account_id));
            transaction->setStatus(TransactionStatus::SUCCESS);

            // Store in memory transaction cache for immediate access
            transaction_cache[account_id].push_back(transaction);
            total_transactions++;

            // Also sync to file for cross-terminal synchronization
            std::ofstream txn_file("transactions.sync", std::ios::app);
            if (txn_file.is_open()) {
                txn_file << transaction->getTransactionId() << "|"
                         << transaction->getFromAccountId() << "|"
                         << transaction->getToAccountId() << "|"
                         << std::fixed << std::setprecision(2) << transaction->getAmount() << "|"
                         << transaction->getTypeString() << "|"
                         << transaction->getStatusString() << "|"
                         << transaction->getDescription() << "|"
                         << transaction->getTimestamp() << std::endl;
                txn_file.close();
                std::cout << "Transaction recorded in synchronized history" << std::endl;
            } else {
                std::cout << "Transaction recorded in memory only" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to record transaction: " << e.what() << std::endl;
        }

        updateSystemStats();
        std::cout << "Withdrawal successful. New balance: $" << std::fixed << std::setprecision(2)
                  << account->getBalance() << std::endl;
        return true;
    }

    std::cerr << "Withdrawal failed" << std::endl;
    return false;
}

// Transfer operation with deadlock prevention
bool BankSystem::transfer(int from_account_id, int to_account_id, double amount) {
    if (!isUserLoggedIn()) {
        std::cerr << "Please login first" << std::endl;
        return false;
    }

    if (!validateAccountOwnership(from_account_id, current_user->getUserId())) {
        std::cerr << "Source account access denied" << std::endl;
        return false;
    }

    auto from_account = getAccount(from_account_id);
    auto to_account = getAccount(to_account_id);

    if (!from_account || !to_account) {
        std::cerr << "One or both accounts not found" << std::endl;
        return false;
    }

    // Use deadlock prevention for concurrent transfers
    std::vector<int> account_ids = {from_account_id, to_account_id};
    int transaction_id = db_handler.getNextTransactionId();

    std::cout << "Requesting locks for accounts " << from_account_id << " and " << to_account_id
              << " (Transaction ID: " << transaction_id << ")" << std::endl;

    if (!deadlock_manager.requestLocks(account_ids, transaction_id)) {
        std::cerr << "Failed to acquire locks - potential deadlock prevented" << std::endl;
        return false;
    }

    std::cout << "Locks acquired successfully, proceeding with transfer..." << std::endl;

    auto transaction = Transaction::createTransfer(from_account_id, to_account_id, amount);
    transaction->setDescription("Transfer from " + std::to_string(from_account_id) +
                               " to " + std::to_string(to_account_id));

    TransactionStatus result = from_account->transfer(to_account, amount);

    // Release locks after operation
    deadlock_manager.releaseLocks(account_ids);
    std::cout << "Locks released for accounts " << from_account_id << " and " << to_account_id << std::endl;

    if (result == TransactionStatus::SUCCESS) {
        // Record transaction in both memory and database
        try {
            transaction->setStatus(TransactionStatus::SUCCESS);

            // Store in memory transaction cache for both accounts
            transaction_cache[from_account_id].push_back(transaction);
            transaction_cache[to_account_id].push_back(transaction);
            total_transactions++;

            // Also sync to file for cross-terminal synchronization
            std::ofstream txn_file("transactions.sync", std::ios::app);
            if (txn_file.is_open()) {
                txn_file << transaction->getTransactionId() << "|"
                         << transaction->getFromAccountId() << "|"
                         << transaction->getToAccountId() << "|"
                         << std::fixed << std::setprecision(2) << transaction->getAmount() << "|"
                         << transaction->getTypeString() << "|"
                         << transaction->getStatusString() << "|"
                         << transaction->getDescription() << "|"
                         << transaction->getTimestamp() << std::endl;
                txn_file.close();
                std::cout << "Transfer transaction recorded in synchronized history" << std::endl;
            } else {
                std::cout << "Transfer transaction recorded in memory only" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to record transaction: " << e.what() << std::endl;
        }

        updateSystemStats();
        std::cout << "Transfer successful. Amount: $" << std::fixed << std::setprecision(2) << amount << std::endl;
        return true;
    }

    std::cerr << "Transfer failed" << std::endl;
    return false;
}

// Get account transactions with file synchronization
std::vector<std::shared_ptr<Transaction>> BankSystem::getAccountTransactions(int account_id) {
    if (!isUserLoggedIn()) {
        std::cerr << "Please login first" << std::endl;
        return {};
    }

    if (!validateAccountOwnership(account_id, current_user->getUserId())) {
        std::cerr << "Account access denied" << std::endl;
        return {};
    }

    std::vector<std::shared_ptr<Transaction>> account_transactions;

    // Load transactions from synchronized file
    std::ifstream txn_file("transactions.sync");
    if (txn_file.is_open()) {
        std::string line;
        while (std::getline(txn_file, line)) {
            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> tokens;

            while (std::getline(iss, token, '|')) {
                tokens.push_back(token);
            }

            if (tokens.size() >= 7) {
                int txn_id = std::stoi(tokens[0]);
                int from_account = (tokens[1] == "0") ? 0 : std::stoi(tokens[1]);
                int to_account = (tokens[2] == "0") ? 0 : std::stoi(tokens[2]);
                double amount = std::stod(tokens[3]);

                // Check if this transaction involves the requested account
                if (from_account == account_id || to_account == account_id) {
                    TransactionType type = TransactionType::DEPOSIT;
                    if (tokens[4] == "WITHDRAWAL") type = TransactionType::WITHDRAWAL;
                    else if (tokens[4] == "TRANSFER") type = TransactionType::TRANSFER;
                    else if (tokens[4] == "INTEREST") type = TransactionType::INTEREST;

                    TransactionStatus status = TransactionStatus::SUCCESS;
                    if (tokens[5] == "FAILED") status = TransactionStatus::FAILED;
                    else if (tokens[5] == "PENDING") status = TransactionStatus::PENDING;

                    auto transaction = std::make_shared<Transaction>(txn_id, from_account, to_account, amount, type, status);
                    transaction->setDescription(tokens[6]);
                    account_transactions.push_back(transaction);
                }
            }
        }
        txn_file.close();
    }

    return account_transactions;
}

// Update system statistics
void BankSystem::updateSystemStats() {
    // This would typically query the database for current stats
    // For now, we'll implement a simplified version
    total_system_balance = 0.0;
    
    std::lock_guard<std::mutex> lock(account_cache_mutex);
    for (const auto& [id, account] : account_cache) {
        total_system_balance += account->getBalance();
    }
}

// Display system statistics
void BankSystem::displaySystemStats() const {
    std::lock_guard<std::mutex> lock(system_mutex);
    
    std::cout << "=== Banking System Statistics ===" << std::endl;
    std::cout << "Total Users: " << total_users << std::endl;
    std::cout << "Total Accounts: " << total_accounts << std::endl;
    std::cout << "Total Transactions: " << total_transactions << std::endl;
    std::cout << "Total System Balance: $" << std::fixed << std::setprecision(2) << total_system_balance << std::endl;
    std::cout << "=================================" << std::endl;
}

// Validate account ownership
bool BankSystem::validateAccountOwnership(int account_id, int user_id) const {
    auto account = db_handler.getAccountById(account_id);
    return account && account->getUserId() == user_id;
}

// Check if account exists
bool BankSystem::accountExists(int account_id) const {
    return db_handler.getAccountById(account_id) != nullptr;
}

// Add to user cache
void BankSystem::addToUserCache(std::shared_ptr<User> user) {
    std::lock_guard<std::mutex> lock(user_cache_mutex);
    user_cache[user->getUserId()] = user;
}

// Add to account cache
void BankSystem::addToAccountCache(std::shared_ptr<Account> account) {
    std::lock_guard<std::mutex> lock(account_cache_mutex);
    account_cache[account->getAccountId()] = account;
}

// Refresh user cache
void BankSystem::refreshUserCache() {
    std::lock_guard<std::mutex> lock(user_cache_mutex);
    user_cache.clear();
    auto users = db_handler.getAllUsers();
    for (auto user : users) {
        user_cache[user->getUserId()] = user;
    }
    total_users = user_cache.size();
}

// Refresh account cache
void BankSystem::refreshAccountCache() {
    std::lock_guard<std::mutex> lock(account_cache_mutex);
    account_cache.clear();
    auto accounts = db_handler.getAllAccounts();
    for (auto account : accounts) {
        account_cache[account->getAccountId()] = account;
    }
    total_accounts = account_cache.size();
}

// Clear caches
void BankSystem::clearCaches() {
    std::lock_guard<std::mutex> user_lock(user_cache_mutex);
    std::lock_guard<std::mutex> account_lock(account_cache_mutex);

    user_cache.clear();
    account_cache.clear();
}

// Display all users (admin function)
void BankSystem::displayAllUsers() const {
    std::cout << "\n=== All Users ===" << std::endl;
    auto users = db_handler.getAllUsers();

    if (users.empty()) {
        std::cout << "No users found." << std::endl;
        return;
    }

    for (const auto& user : users) {
        std::cout << "ID: " << user->getUserId()
                  << " | Name: " << user->getName()
                  << " | Email: " << user->getEmail() << std::endl;
    }
}

// Display all accounts (admin function)
void BankSystem::displayAllAccounts() const {
    std::cout << "\n=== All Accounts ===" << std::endl;
    auto accounts = db_handler.getAllAccounts();

    if (accounts.empty()) {
        std::cout << "No accounts found." << std::endl;
        return;
    }

    for (const auto& account : accounts) {
        std::cout << "Account ID: " << account->getAccountId()
                  << " | User ID: " << account->getUserId()
                  << " | Type: " << account->getAccountTypeString()
                  << " | Balance: $" << std::fixed << std::setprecision(2) << account->getBalance()
                  << std::endl;
    }
}
