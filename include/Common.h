#ifndef COMMON_H
#define COMMON_H

// Common enums and types used across the banking system

enum class TransactionStatus {
    SUCCESS,
    FAILED,
    PENDING
};

enum class AccountType {
    SAVINGS,
    CURRENT
};

enum class TransactionType {
    DEPOSIT,
    WITHDRAWAL,
    TRANSFER,
    INTEREST
};

// Common constants
namespace BankingConstants {
    const double MAX_TRANSACTION_AMOUNT = 1000000.0;
    const double MIN_TRANSACTION_AMOUNT = 0.01;
    const double DEFAULT_SAVINGS_INTEREST_RATE = 0.035; // 3.5%
    const int MAX_LOGIN_ATTEMPTS = 5;
    const int RATE_LIMIT_WINDOW_MINUTES = 15;
    const int SESSION_TIMEOUT_HOURS = 24;
}

#endif // COMMON_H
