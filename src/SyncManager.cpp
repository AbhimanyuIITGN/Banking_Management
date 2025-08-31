#include "SyncManager.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

SyncManager* SyncManager::instance = nullptr;
std::mutex SyncManager::instance_mutex;

SyncManager::SyncManager() 
    : sync_file_path("account_balances.sync"), 
      transaction_file_path("transactions.sync") {
}

SyncManager& SyncManager::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (instance == nullptr) {
        instance = new SyncManager();
    }
    return *instance;
}

// Sync account balance to file
bool SyncManager::syncAccountBalance(int account_id, double balance) {
    std::lock_guard<std::mutex> lock(sync_mutex);
    
    try {
        // Load existing balances
        std::unordered_map<int, double> balances;
        loadAccountBalances(balances);
        
        // Update balance
        balances[account_id] = balance;
        
        // Save back to file
        return saveAccountBalances(balances);
    } catch (const std::exception& e) {
        std::cerr << "Error syncing account balance: " << e.what() << std::endl;
        return false;
    }
}

// Get account balance from file
double SyncManager::getAccountBalance(int account_id) {
    std::lock_guard<std::mutex> lock(sync_mutex);
    
    try {
        std::unordered_map<int, double> balances;
        if (loadAccountBalances(balances)) {
            auto it = balances.find(account_id);
            if (it != balances.end()) {
                return it->second;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting account balance: " << e.what() << std::endl;
    }
    
    return 0.0; // Default balance
}

// Check if account exists in sync file
bool SyncManager::accountExists(int account_id) {
    std::lock_guard<std::mutex> lock(sync_mutex);
    
    std::unordered_map<int, double> balances;
    if (loadAccountBalances(balances)) {
        return balances.find(account_id) != balances.end();
    }
    
    return false;
}

// Sync transaction to file
bool SyncManager::syncTransaction(const Transaction& transaction) {
    std::lock_guard<std::mutex> lock(sync_mutex);
    
    try {
        return saveTransaction(transaction);
    } catch (const std::exception& e) {
        std::cerr << "Error syncing transaction: " << e.what() << std::endl;
        return false;
    }
}

// Get account transactions from file
std::vector<std::shared_ptr<Transaction>> SyncManager::getAccountTransactions(int account_id) {
    std::lock_guard<std::mutex> lock(sync_mutex);
    
    std::vector<std::shared_ptr<Transaction>> account_transactions;
    std::vector<std::shared_ptr<Transaction>> all_transactions;
    
    if (loadTransactions(all_transactions)) {
        for (const auto& txn : all_transactions) {
            if (txn->getFromAccountId() == account_id || txn->getToAccountId() == account_id) {
                account_transactions.push_back(txn);
            }
        }
    }
    
    return account_transactions;
}

// Load account balances from file
bool SyncManager::loadAccountBalances(std::unordered_map<int, double>& balances) {
    std::ifstream file(sync_file_path);
    if (!file.is_open()) {
        return true; // File doesn't exist yet, that's okay
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int account_id;
        double balance;
        if (iss >> account_id >> balance) {
            balances[account_id] = balance;
        }
    }
    
    return true;
}

// Save account balances to file
bool SyncManager::saveAccountBalances(const std::unordered_map<int, double>& balances) {
    std::ofstream file(sync_file_path);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& pair : balances) {
        file << pair.first << " " << std::fixed << std::setprecision(2) << pair.second << std::endl;
    }
    
    return true;
}

// Load transactions from file
bool SyncManager::loadTransactions(std::vector<std::shared_ptr<Transaction>>& transactions) {
    std::ifstream file(transaction_file_path);
    if (!file.is_open()) {
        return true; // File doesn't exist yet, that's okay
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Parse transaction line: txn_id|from_account|to_account|amount|type|status|description|timestamp
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
            
            TransactionType type = TransactionType::DEPOSIT;
            if (tokens[4] == "WITHDRAWAL") type = TransactionType::WITHDRAWAL;
            else if (tokens[4] == "TRANSFER") type = TransactionType::TRANSFER;
            else if (tokens[4] == "INTEREST") type = TransactionType::INTEREST;
            
            TransactionStatus status = TransactionStatus::SUCCESS;
            if (tokens[5] == "FAILED") status = TransactionStatus::FAILED;
            else if (tokens[5] == "PENDING") status = TransactionStatus::PENDING;
            
            auto transaction = std::make_shared<Transaction>(txn_id, from_account, to_account, amount, type, status);
            transaction->setDescription(tokens[6]);
            transactions.push_back(transaction);
        }
    }
    
    return true;
}

// Save transaction to file
bool SyncManager::saveTransaction(const Transaction& transaction) {
    std::ofstream file(transaction_file_path, std::ios::app);
    if (!file.is_open()) {
        return false;
    }
    
    file << transaction.getTransactionId() << "|"
         << transaction.getFromAccountId() << "|"
         << transaction.getToAccountId() << "|"
         << std::fixed << std::setprecision(2) << transaction.getAmount() << "|"
         << transaction.getTypeString() << "|"
         << transaction.getStatusString() << "|"
         << transaction.getDescription() << "|"
         << transaction.getTimestamp() << std::endl;
    
    return true;
}

// Clear sync files
void SyncManager::clearSyncFiles() {
    std::filesystem::remove(sync_file_path);
    std::filesystem::remove(transaction_file_path);
}
