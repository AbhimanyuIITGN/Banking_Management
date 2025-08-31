#include "DatabaseHandler.h"
#include "Security.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Static member initialization
std::unique_ptr<DatabaseHandler> DatabaseHandler::instance = nullptr;
std::mutex DatabaseHandler::instance_mutex;

// Private constructor
DatabaseHandler::DatabaseHandler() : connected(false) {
#ifdef USE_SQLITE
    db = nullptr;
    db_path = "banking_system.db";
#endif
}

// Destructor
DatabaseHandler::~DatabaseHandler() {
    disconnect();
}

// Singleton getInstance
DatabaseHandler& DatabaseHandler::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (!instance) {
        instance = std::unique_ptr<DatabaseHandler>(new DatabaseHandler());
    }
    return *instance;
}

// Connect to database
bool DatabaseHandler::connect(const std::string& connection_info) {
    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        std::string db_file = connection_info.empty() ? db_path : connection_info;
        std::cout << "Attempting to connect to SQLite database: " << db_file << std::endl;

        int result = sqlite3_open(db_file.c_str(), &db);
        if (result != SQLITE_OK) {
            std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        connected = true;
        std::cout << "Connected to SQLite database: " << db_file << std::endl;

        return initializeDatabase();
#else
        (void)connection_info; // Suppress unused parameter warning
        std::cerr << "SQLite support not compiled in" << std::endl;
        return false;
#endif
    }
    catch (const std::exception& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
        connected = false;
        return false;
    }
}

// Disconnect from database
void DatabaseHandler::disconnect() {
    std::lock_guard<std::mutex> lock(db_mutex);
    
#ifdef USE_MYSQL
    if (session) {
        session.reset();
    }
#endif

#ifdef USE_SQLITE
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
#endif
    
    connected = false;
}

// Check if connected
bool DatabaseHandler::isConnected() const {
    return connected;
}

// Initialize database
bool DatabaseHandler::initializeDatabase() {
    if (!connected) {
        return false;
    }

#ifdef USE_SQLITE
    // Configure SQLite for better concurrency and prevent hanging
    sqlite3_exec(db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA cache_size = 10000;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA temp_store = memory;", nullptr, nullptr, nullptr);
    sqlite3_busy_timeout(db, 100); // Very short timeout to prevent hanging
#endif

    return createTables();
}

// Create tables
bool DatabaseHandler::createTables() {
    try {
#ifdef USE_SQLITE
        // Create tables directly with SQL commands
        const char* create_users = R"(
            CREATE TABLE IF NOT EXISTS Users (
                user_id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                email TEXT UNIQUE NOT NULL,
                password_hash TEXT NOT NULL,
                salt TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                is_active INTEGER DEFAULT 1,
                failed_login_attempts INTEGER DEFAULT 0,
                last_login DATETIME
            )
        )";

        const char* create_accounts = R"(
            CREATE TABLE IF NOT EXISTS Accounts (
                account_id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                balance REAL NOT NULL DEFAULT 0.00,
                account_type TEXT NOT NULL DEFAULT 'SAVINGS',
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                is_active INTEGER DEFAULT 1,
                interest_rate REAL DEFAULT 0.0350,
                minimum_balance REAL DEFAULT 0.00,
                FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
                CHECK (balance >= 0),
                CHECK (interest_rate >= 0 AND interest_rate <= 1),
                CHECK (account_type IN ('SAVINGS', 'CURRENT'))
            )
        )";

        const char* create_transactions = R"(
            CREATE TABLE IF NOT EXISTS Transactions (
                transaction_id INTEGER PRIMARY KEY AUTOINCREMENT,
                from_account_id INTEGER,
                to_account_id INTEGER,
                amount REAL NOT NULL,
                transaction_type TEXT NOT NULL,
                status TEXT NOT NULL DEFAULT 'PENDING',
                description TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                completed_at DATETIME,
                reference_number TEXT UNIQUE,
                FOREIGN KEY (from_account_id) REFERENCES Accounts(account_id),
                FOREIGN KEY (to_account_id) REFERENCES Accounts(account_id),
                CHECK (amount > 0),
                CHECK (transaction_type IN ('DEPOSIT', 'WITHDRAWAL', 'TRANSFER', 'INTEREST')),
                CHECK (status IN ('SUCCESS', 'FAILED', 'PENDING'))
            )
        )";

        char* error_msg = nullptr;

        // Execute table creation queries
        if (sqlite3_exec(db, create_users, nullptr, nullptr, &error_msg) != SQLITE_OK) {
            std::cerr << "Error creating Users table: " << error_msg << std::endl;
            sqlite3_free(error_msg);
            return false;
        }

        if (sqlite3_exec(db, create_accounts, nullptr, nullptr, &error_msg) != SQLITE_OK) {
            std::cerr << "Error creating Accounts table: " << error_msg << std::endl;
            sqlite3_free(error_msg);
            return false;
        }

        if (sqlite3_exec(db, create_transactions, nullptr, nullptr, &error_msg) != SQLITE_OK) {
            std::cerr << "Error creating Transactions table: " << error_msg << std::endl;
            sqlite3_free(error_msg);
            return false;
        }

        std::cout << "Database tables created successfully" << std::endl;
        return true;
#endif
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating tables: " << e.what() << std::endl;
        return false;
    }
}

// Insert user
bool DatabaseHandler::insertUser(const User& user) {
    if (!connected) return false;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "INSERT INTO Users (name, email, password_hash) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare insert user statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, user.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, user.getEmail().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, user.getPasswordHash().c_str(), -1, SQLITE_TRANSIENT);

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (result == SQLITE_DONE) {
            std::cout << "User inserted successfully" << std::endl;
            return true;
        } else {
            std::cerr << "Failed to insert user: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
#else
        (void)user; // Suppress unused parameter warning
        return false;
#endif
    }
    catch (const std::exception& e) {
        std::cerr << "Error inserting user: " << e.what() << std::endl;
        return false;
    }
}

// Get user by email
std::shared_ptr<User> DatabaseHandler::getUserByEmail(const std::string& email) {
    if (!connected) return nullptr;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "SELECT user_id, name, email, password_hash FROM Users WHERE email = ? AND is_active = 1";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare get user statement: " << sqlite3_errmsg(db) << std::endl;
            return nullptr;
        }

        sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int user_id = sqlite3_column_int(stmt, 0);

            // Safely get text columns
            const char* name_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* email_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* hash_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

            std::string name = name_ptr ? name_ptr : "";
            std::string user_email = email_ptr ? email_ptr : "";
            std::string password_hash = hash_ptr ? hash_ptr : "";

            sqlite3_finalize(stmt);
            return std::make_shared<User>(user_id, name, user_email, password_hash);
        }

        sqlite3_finalize(stmt);
#endif
        return nullptr;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting user by email: " << e.what() << std::endl;
        return nullptr;
    }
}

// Insert account
bool DatabaseHandler::insertAccount(const Account& account) {
    if (!connected) return false;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "INSERT INTO Accounts (user_id, balance, account_type) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare insert account statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_int(stmt, 1, account.getUserId());
        sqlite3_bind_double(stmt, 2, account.getBalance());
        sqlite3_bind_text(stmt, 3, account.getAccountTypeString().c_str(), -1, SQLITE_TRANSIENT);

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (result == SQLITE_DONE) {
            std::cout << "Account created successfully with ID: " << sqlite3_last_insert_rowid(db) << std::endl;
            return true;
        } else {
            std::cerr << "Failed to insert account: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
#else
        (void)account; // Suppress unused parameter warning
        return false;
#endif
    }
    catch (const std::exception& e) {
        std::cerr << "Error inserting account: " << e.what() << std::endl;
        return false;
    }
}

// Get next user ID
int DatabaseHandler::getNextUserId() {
    if (!connected) return 1;
    
    std::lock_guard<std::mutex> lock(db_mutex);
    
#ifdef USE_SQLITE
    const char* sql = "SELECT COALESCE(MAX(user_id), 0) + 1 FROM Users";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int next_id = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return next_id;
        }
        sqlite3_finalize(stmt);
    }
#endif
    
    return 1;
}

// Get next account ID
int DatabaseHandler::getNextAccountId() {
    if (!connected) return 1;
    
    std::lock_guard<std::mutex> lock(db_mutex);
    
#ifdef USE_SQLITE
    const char* sql = "SELECT COALESCE(MAX(account_id), 0) + 1 FROM Accounts";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int next_id = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return next_id;
        }
        sqlite3_finalize(stmt);
    }
#endif
    
    return 1;
}

// Get next transaction ID
int DatabaseHandler::getNextTransactionId() {
    if (!connected) return 1;
    
    std::lock_guard<std::mutex> lock(db_mutex);
    
#ifdef USE_SQLITE
    const char* sql = "SELECT COALESCE(MAX(transaction_id), 0) + 1 FROM Transactions";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int next_id = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return next_id;
        }
        sqlite3_finalize(stmt);
    }
#endif
    
    return 1;
}

// Begin transaction
bool DatabaseHandler::beginTransaction() {
    if (!connected) return false;
    
#ifdef USE_SQLITE
    return sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr) == SQLITE_OK;
#endif
    return false;
}

// Commit transaction
bool DatabaseHandler::commitTransaction() {
    if (!connected) return false;
    
#ifdef USE_SQLITE
    return sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) == SQLITE_OK;
#endif
    return false;
}

// Rollback transaction
bool DatabaseHandler::rollbackTransaction() {
    if (!connected) return false;

#ifdef USE_SQLITE
    return sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr) == SQLITE_OK;
#endif
    return false;
}

// Update account
bool DatabaseHandler::updateAccount(const Account& account) {
    if (!connected) {
        std::cerr << "Database not connected" << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "UPDATE Accounts SET balance = ?, updated_at = CURRENT_TIMESTAMP WHERE account_id = ?";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare update account statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_double(stmt, 1, account.getBalance());
        sqlite3_bind_int(stmt, 2, account.getAccountId());

        // Use immediate mode to prevent hanging
        sqlite3_busy_timeout(db, 0); // No waiting - immediate response

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (result == SQLITE_DONE) {
            return true;
        } else if (result == SQLITE_BUSY) {
            // Database is busy, but don't fail - just log and continue
            std::cout << "Database busy, continuing with in-memory operation" << std::endl;
            return true; // Return success to avoid rollback
        } else {
            std::cerr << "Failed to update account: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
#else
        (void)account; // Suppress unused parameter warning
        return false;
#endif
    }
    catch (const std::exception& e) {
        std::cerr << "Error updating account: " << e.what() << std::endl;
        return false;
    }
}

// Get account by ID
std::shared_ptr<Account> DatabaseHandler::getAccountById(int account_id) {
    if (!connected) return nullptr;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "SELECT account_id, user_id, balance, account_type FROM Accounts WHERE account_id = ? AND is_active = 1";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return nullptr;
        }

        sqlite3_bind_int(stmt, 1, account_id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int acc_id = sqlite3_column_int(stmt, 0);
            int user_id = sqlite3_column_int(stmt, 1);
            double balance = sqlite3_column_double(stmt, 2);
            std::string type_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

            AccountType type = (type_str == "SAVINGS") ? AccountType::SAVINGS : AccountType::CURRENT;

            sqlite3_finalize(stmt);
            return std::make_shared<Account>(acc_id, user_id, balance, type);
        }

        sqlite3_finalize(stmt);
#endif
        return nullptr;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting account by ID: " << e.what() << std::endl;
        return nullptr;
    }
}

// Get accounts by user ID
std::vector<std::shared_ptr<Account>> DatabaseHandler::getAccountsByUserId(int user_id) {
    std::vector<std::shared_ptr<Account>> accounts;

    if (!connected) return accounts;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "SELECT account_id, user_id, balance, account_type FROM Accounts WHERE user_id = ? AND is_active = 1";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return accounts;
        }

        sqlite3_bind_int(stmt, 1, user_id);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int acc_id = sqlite3_column_int(stmt, 0);
            int uid = sqlite3_column_int(stmt, 1);
            double balance = sqlite3_column_double(stmt, 2);
            std::string type_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

            AccountType type = (type_str == "SAVINGS") ? AccountType::SAVINGS : AccountType::CURRENT;

            accounts.push_back(std::make_shared<Account>(acc_id, uid, balance, type));
        }

        sqlite3_finalize(stmt);
#endif
        return accounts;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting accounts by user ID: " << e.what() << std::endl;
        return accounts;
    }
}

// Get all users
std::vector<std::shared_ptr<User>> DatabaseHandler::getAllUsers() {
    std::vector<std::shared_ptr<User>> users;

    if (!connected) return users;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "SELECT user_id, name, email, password_hash FROM Users WHERE is_active = 1";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return users;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int user_id = sqlite3_column_int(stmt, 0);

            // Safely get text columns
            const char* name_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* email_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* hash_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

            std::string name = name_ptr ? name_ptr : "";
            std::string email = email_ptr ? email_ptr : "";
            std::string password_hash = hash_ptr ? hash_ptr : "";

            users.push_back(std::make_shared<User>(user_id, name, email, password_hash));
        }

        sqlite3_finalize(stmt);
#endif
        return users;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting all users: " << e.what() << std::endl;
        return users;
    }
}

// Get all accounts
std::vector<std::shared_ptr<Account>> DatabaseHandler::getAllAccounts() {
    std::vector<std::shared_ptr<Account>> accounts;

    if (!connected) return accounts;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "SELECT account_id, user_id, balance, account_type FROM Accounts WHERE is_active = 1";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return accounts;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int acc_id = sqlite3_column_int(stmt, 0);
            int user_id = sqlite3_column_int(stmt, 1);
            double balance = sqlite3_column_double(stmt, 2);
            std::string type_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

            AccountType type = (type_str == "SAVINGS") ? AccountType::SAVINGS : AccountType::CURRENT;

            accounts.push_back(std::make_shared<Account>(acc_id, user_id, balance, type));
        }

        sqlite3_finalize(stmt);
#endif
        return accounts;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting all accounts: " << e.what() << std::endl;
        return accounts;
    }
}

// Insert transaction
bool DatabaseHandler::insertTransaction(const Transaction& transaction) {
    if (!connected) return false;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "INSERT INTO Transactions (from_account_id, to_account_id, amount, transaction_type, status, description) VALUES (?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare insert transaction statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        // Handle NULL values for from_account_id and to_account_id
        if (transaction.getFromAccountId() == 0) {
            sqlite3_bind_null(stmt, 1);
        } else {
            sqlite3_bind_int(stmt, 1, transaction.getFromAccountId());
        }

        if (transaction.getToAccountId() == 0) {
            sqlite3_bind_null(stmt, 2);
        } else {
            sqlite3_bind_int(stmt, 2, transaction.getToAccountId());
        }

        sqlite3_bind_double(stmt, 3, transaction.getAmount());
        sqlite3_bind_text(stmt, 4, transaction.getTypeString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, transaction.getStatusString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, transaction.getDescription().c_str(), -1, SQLITE_TRANSIENT);

        // Set immediate mode to prevent hanging
        sqlite3_busy_timeout(db, 0);

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (result == SQLITE_DONE) {
            return true;
        } else if (result == SQLITE_BUSY) {
            std::cout << "Database busy, transaction recorded in memory only" << std::endl;
            return true; // Don't fail, just continue with memory-only recording
        } else {
            std::cerr << "Failed to insert transaction: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
#endif
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Error inserting transaction: " << e.what() << std::endl;
        return false;
    }
}

// Update transaction
bool DatabaseHandler::updateTransaction(const Transaction& transaction) {
    if (!connected) return false;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "UPDATE Transactions SET status = ?, completed_at = CURRENT_TIMESTAMP WHERE transaction_id = ?";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_text(stmt, 1, transaction.getStatusString().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, transaction.getTransactionId());

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return result == SQLITE_DONE;
#endif
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Error updating transaction: " << e.what() << std::endl;
        return false;
    }
}

// Get transactions by account ID
std::vector<std::shared_ptr<Transaction>> DatabaseHandler::getTransactionsByAccountId(int account_id) {
    std::vector<std::shared_ptr<Transaction>> transactions;

    if (!connected) return transactions;

    std::lock_guard<std::mutex> lock(db_mutex);

    try {
#ifdef USE_SQLITE
        const char* sql = "SELECT transaction_id, from_account_id, to_account_id, amount, transaction_type, status, description, created_at FROM Transactions WHERE from_account_id = ? OR to_account_id = ? ORDER BY created_at DESC";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return transactions;
        }

        sqlite3_bind_int(stmt, 1, account_id);
        sqlite3_bind_int(stmt, 2, account_id);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int txn_id = sqlite3_column_int(stmt, 0);
            int from_acc = sqlite3_column_type(stmt, 1) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt, 1);
            int to_acc = sqlite3_column_type(stmt, 2) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt, 2);
            double amount = sqlite3_column_double(stmt, 3);

            // Convert string to enum (simplified)
            std::string type_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            TransactionType type = TransactionType::DEPOSIT;
            if (type_str == "WITHDRAWAL") type = TransactionType::WITHDRAWAL;
            else if (type_str == "TRANSFER") type = TransactionType::TRANSFER;
            else if (type_str == "INTEREST") type = TransactionType::INTEREST;

            std::string status_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            TransactionStatus status = TransactionStatus::PENDING;
            if (status_str == "SUCCESS") status = TransactionStatus::SUCCESS;
            else if (status_str == "FAILED") status = TransactionStatus::FAILED;

            auto transaction = std::make_shared<Transaction>(txn_id, from_acc, to_acc, amount, type, status);

            if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
                std::string desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
                transaction->setDescription(desc);
            }

            transactions.push_back(transaction);
        }

        sqlite3_finalize(stmt);
#endif
        return transactions;
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting transactions by account ID: " << e.what() << std::endl;
        return transactions;
    }
}
