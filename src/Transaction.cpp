#include "Transaction.h"
#include "DatabaseHandler.h"
#include "Security.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <random>

// Default constructor
Transaction::Transaction() 
    : transaction_id(0), from_account_id(0), to_account_id(0), 
      amount(0.0), type(TransactionType::DEPOSIT), 
      status(TransactionStatus::PENDING) {
    timestamp = getCurrentTimestamp();
}

// Parameterized constructor
Transaction::Transaction(int txn_id, int from_account, int to_account, 
                        double amount, TransactionType type, TransactionStatus status)
    : transaction_id(txn_id), from_account_id(from_account), to_account_id(to_account),
      amount(amount), type(type), status(status) {
    timestamp = getCurrentTimestamp();
}

// Destructor
Transaction::~Transaction() {}

// Getters
int Transaction::getTransactionId() const {
    return transaction_id;
}

int Transaction::getFromAccountId() const {
    return from_account_id;
}

int Transaction::getToAccountId() const {
    return to_account_id;
}

double Transaction::getAmount() const {
    return amount;
}

TransactionType Transaction::getType() const {
    return type;
}

TransactionStatus Transaction::getStatus() const {
    return status;
}

std::string Transaction::getTimestamp() const {
    return timestamp;
}

std::string Transaction::getDescription() const {
    return description;
}

// Setters
void Transaction::setTransactionId(int id) {
    transaction_id = id;
}

void Transaction::setFromAccountId(int from_id) {
    from_account_id = from_id;
}

void Transaction::setToAccountId(int to_id) {
    to_account_id = to_id;
}

void Transaction::setAmount(double amount) {
    this->amount = amount;
}

void Transaction::setType(TransactionType type) {
    this->type = type;
}

void Transaction::setStatus(TransactionStatus status) {
    this->status = status;
}

void Transaction::setDescription(const std::string& desc) {
    description = desc;
}

// Execute transaction
bool Transaction::execute() {
    try {
        DatabaseHandler& db = DatabaseHandler::getInstance();
        
        // Insert transaction record
        if (db.insertTransaction(*this)) {
            status = TransactionStatus::SUCCESS;
            return true;
        }
        
        status = TransactionStatus::FAILED;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Transaction execution error: " << e.what() << std::endl;
        status = TransactionStatus::FAILED;
        return false;
    }
}

// Rollback transaction
bool Transaction::rollback() {
    try {
        status = TransactionStatus::FAILED;
        DatabaseHandler& db = DatabaseHandler::getInstance();
        return db.updateTransaction(*this);
    }
    catch (const std::exception& e) {
        std::cerr << "Transaction rollback error: " << e.what() << std::endl;
        return false;
    }
}

// Commit transaction
void Transaction::commit() {
    status = TransactionStatus::SUCCESS;
    DatabaseHandler& db = DatabaseHandler::getInstance();
    db.updateTransaction(*this);
}

// Get transaction type as string
std::string Transaction::getTypeString() const {
    switch (type) {
        case TransactionType::DEPOSIT:
            return "DEPOSIT";
        case TransactionType::WITHDRAWAL:
            return "WITHDRAWAL";
        case TransactionType::TRANSFER:
            return "TRANSFER";
        case TransactionType::INTEREST:
            return "INTEREST";
        default:
            return "UNKNOWN";
    }
}

// Get transaction status as string
std::string Transaction::getStatusString() const {
    switch (status) {
        case TransactionStatus::SUCCESS:
            return "SUCCESS";
        case TransactionStatus::FAILED:
            return "FAILED";
        case TransactionStatus::PENDING:
            return "PENDING";
        default:
            return "UNKNOWN";
    }
}

// Get current timestamp
std::string Transaction::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Validate transaction
bool Transaction::isValid() const {
    if (!Security::isValidAmount(amount)) {
        return false;
    }
    
    switch (type) {
        case TransactionType::DEPOSIT:
            return (from_account_id == 0 && to_account_id > 0);
        case TransactionType::WITHDRAWAL:
            return (from_account_id > 0 && to_account_id == 0);
        case TransactionType::TRANSFER:
            return (from_account_id > 0 && to_account_id > 0 && from_account_id != to_account_id);
        case TransactionType::INTEREST:
            return (from_account_id == 0 && to_account_id > 0);
        default:
            return false;
    }
}

// Display transaction
void Transaction::displayTransaction() const {
    std::cout << "=== Transaction Details ===" << std::endl;
    std::cout << "Transaction ID: " << transaction_id << std::endl;
    std::cout << "Type: " << getTypeString() << std::endl;
    std::cout << "Amount: $" << std::fixed << std::setprecision(2) << amount << std::endl;
    std::cout << "Status: " << getStatusString() << std::endl;
    std::cout << "Timestamp: " << timestamp << std::endl;
    
    if (from_account_id > 0) {
        std::cout << "From Account: " << from_account_id << std::endl;
    }
    if (to_account_id > 0) {
        std::cout << "To Account: " << to_account_id << std::endl;
    }
    if (!description.empty()) {
        std::cout << "Description: " << description << std::endl;
    }
    std::cout << "===========================" << std::endl;
}

// Convert to string
std::string Transaction::toString() const {
    std::stringstream ss;
    ss << "TXN-" << transaction_id << " | " << getTypeString() 
       << " | $" << std::fixed << std::setprecision(2) << amount 
       << " | " << getStatusString() << " | " << timestamp;
    return ss.str();
}

// Factory methods
std::shared_ptr<Transaction> Transaction::createDeposit(int account_id, double amount) {
    DatabaseHandler& db = DatabaseHandler::getInstance();
    int txn_id = db.getNextTransactionId();
    return std::make_shared<Transaction>(txn_id, 0, account_id, amount, TransactionType::DEPOSIT);
}

std::shared_ptr<Transaction> Transaction::createWithdrawal(int account_id, double amount) {
    DatabaseHandler& db = DatabaseHandler::getInstance();
    int txn_id = db.getNextTransactionId();
    return std::make_shared<Transaction>(txn_id, account_id, 0, amount, TransactionType::WITHDRAWAL);
}

std::shared_ptr<Transaction> Transaction::createTransfer(int from_account, int to_account, double amount) {
    DatabaseHandler& db = DatabaseHandler::getInstance();
    int txn_id = db.getNextTransactionId();
    return std::make_shared<Transaction>(txn_id, from_account, to_account, amount, TransactionType::TRANSFER);
}
