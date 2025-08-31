#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <memory>
#include "Account.h"
#include "Transaction.h"

class SyncManager {
private:
    static SyncManager* instance;
    static std::mutex instance_mutex;
    
    std::string sync_file_path;
    std::string transaction_file_path;
    mutable std::mutex sync_mutex;
    
    SyncManager();

public:
    static SyncManager& getInstance();
    
    // Account synchronization
    bool syncAccountBalance(int account_id, double balance);
    double getAccountBalance(int account_id);
    bool accountExists(int account_id);
    
    // Transaction synchronization
    bool syncTransaction(const Transaction& transaction);
    std::vector<std::shared_ptr<Transaction>> getAccountTransactions(int account_id);
    
    // File operations
    bool loadAccountBalances(std::unordered_map<int, double>& balances);
    bool saveAccountBalances(const std::unordered_map<int, double>& balances);
    
    bool loadTransactions(std::vector<std::shared_ptr<Transaction>>& transactions);
    bool saveTransaction(const Transaction& transaction);
    
    // Cleanup
    void clearSyncFiles();
};

#endif // SYNC_MANAGER_H
