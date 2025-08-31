#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <chrono>
#include <memory>
#include "Common.h"

class Transaction {
private:
    int transaction_id;
    int from_account_id;
    int to_account_id;
    double amount;
    TransactionType type;
    TransactionStatus status;
    std::string timestamp;
    std::string description;

public:
    // Constructors
    Transaction();
    Transaction(int txn_id, int from_account, int to_account, 
               double amount, TransactionType type, 
               TransactionStatus status = TransactionStatus::PENDING);
    
    // Destructor
    ~Transaction();

    // Getters
    int getTransactionId() const;
    int getFromAccountId() const;
    int getToAccountId() const;
    double getAmount() const;
    TransactionType getType() const;
    TransactionStatus getStatus() const;
    std::string getTimestamp() const;
    std::string getDescription() const;

    // Setters
    void setTransactionId(int id);
    void setFromAccountId(int from_id);
    void setToAccountId(int to_id);
    void setAmount(double amount);
    void setType(TransactionType type);
    void setStatus(TransactionStatus status);
    void setDescription(const std::string& desc);

    // Transaction operations
    bool execute();
    bool rollback();
    void commit();

    // Utility methods
    std::string getTypeString() const;
    std::string getStatusString() const;
    static std::string getCurrentTimestamp();
    
    // Validation
    bool isValid() const;
    
    // Display methods
    void displayTransaction() const;
    std::string toString() const;

    // Static factory methods
    static std::shared_ptr<Transaction> createDeposit(int account_id, double amount);
    static std::shared_ptr<Transaction> createWithdrawal(int account_id, double amount);
    static std::shared_ptr<Transaction> createTransfer(int from_account, int to_account, double amount);
};

#endif // TRANSACTION_H
