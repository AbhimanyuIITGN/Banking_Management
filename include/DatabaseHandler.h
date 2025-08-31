#ifndef DATABASE_HANDLER_H
#define DATABASE_HANDLER_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "User.h"
#include "Account.h"
#include "Transaction.h"

#ifdef USE_MYSQL
#include <mysqlx/xdevapi.h>
#endif

#ifdef USE_SQLITE
#include <sqlite3.h>
#endif

class DatabaseHandler {
private:
    static std::unique_ptr<DatabaseHandler> instance;
    static std::mutex instance_mutex;
    
#ifdef USE_MYSQL
    std::unique_ptr<mysqlx::Session> session;
    std::string connection_string;
#endif

#ifdef USE_SQLITE
    sqlite3* db;
    std::string db_path;
#endif

    mutable std::mutex db_mutex;
    bool connected;

    // Private constructor for singleton
    DatabaseHandler();

public:
    // Destructor
    ~DatabaseHandler();

    // Singleton pattern
    static DatabaseHandler& getInstance();
    
    // Delete copy constructor and assignment operator
    DatabaseHandler(const DatabaseHandler&) = delete;
    DatabaseHandler& operator=(const DatabaseHandler&) = delete;

    // Connection management
    bool connect(const std::string& connection_info = "");
    void disconnect();
    bool isConnected() const;

    // Database initialization
    bool initializeDatabase();
    bool createTables();
    bool dropTables();

    // User operations
    bool insertUser(const User& user);
    std::shared_ptr<User> getUserById(int user_id);
    std::shared_ptr<User> getUserByEmail(const std::string& email);
    bool updateUser(const User& user);
    bool deleteUser(int user_id);
    std::vector<std::shared_ptr<User>> getAllUsers();

    // Account operations
    bool insertAccount(const Account& account);
    std::shared_ptr<Account> getAccountById(int account_id);
    std::vector<std::shared_ptr<Account>> getAccountsByUserId(int user_id);
    bool updateAccount(const Account& account);
    bool deleteAccount(int account_id);
    std::vector<std::shared_ptr<Account>> getAllAccounts();

    // Transaction operations
    bool insertTransaction(const Transaction& transaction);
    std::shared_ptr<Transaction> getTransactionById(int transaction_id);
    std::vector<std::shared_ptr<Transaction>> getTransactionsByAccountId(int account_id);
    std::vector<std::shared_ptr<Transaction>> getTransactionsByUserId(int user_id);
    bool updateTransaction(const Transaction& transaction);
    std::vector<std::shared_ptr<Transaction>> getAllTransactions();

    // Transaction management (ACID compliance)
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // Utility methods
    int getNextUserId();
    int getNextAccountId();
    int getNextTransactionId();
    
    // Database maintenance
    bool vacuum();
    bool backup(const std::string& backup_path);
    bool restore(const std::string& backup_path);

private:
    // Helper methods
    bool executeQuery(const std::string& query);
    std::string escapeString(const std::string& input);
    
#ifdef USE_SQLITE
    bool prepareSQLiteStatement(const std::string& query, sqlite3_stmt** stmt);
#endif
};

#endif // DATABASE_HANDLER_H
