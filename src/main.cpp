#include <iostream>
#include <string>
#include <limits>
#include <thread>
#include <vector>
#include <future>
#include <iomanip>
#include "BankSystem.h"
#include "User.h"
#include "Account.h"
#include "Transaction.h"
#include "Security.h"

class BankingCLI {
private:
    BankSystem& bank_system;
    bool running;

public:
    BankingCLI() : bank_system(BankSystem::getInstance()), running(true) {}

    void run() {
        std::cout << "=== Welcome to Banking Management System ===" << std::endl;
        std::cout << "Initializing system..." << std::endl;
        
        if (!bank_system.initialize()) {
            std::cerr << "Failed to initialize banking system" << std::endl;
            return;
        }
        
        while (running) {
            if (bank_system.isUserLoggedIn()) {
                showUserMenu();
            } else {
                showMainMenu();
            }
        }
        
        bank_system.shutdown();
        std::cout << "Thank you for using Banking Management System!" << std::endl;
    }

private:
    void showMainMenu() {
        std::cout << "\n=== Main Menu ===" << std::endl;
        std::cout << "1. Login" << std::endl;
        std::cout << "2. Register" << std::endl;
        std::cout << "3. Admin Panel" << std::endl;
        std::cout << "4. Exit" << std::endl;
        std::cout << "Choose an option: ";
        
        int choice = getIntInput();
        
        switch (choice) {
            case 1:
                handleLogin();
                break;
            case 2:
                handleRegistration();
                break;
            case 3:
                handleAdminPanel();
                break;
            case 4:
                running = false;
                break;
            default:
                std::cout << "Invalid option. Please try again." << std::endl;
        }
    }

    void showUserMenu() {
        auto user = bank_system.getCurrentUser();
        std::cout << "\n=== User Menu - Welcome " << user->getName() << " ===" << std::endl;
        std::cout << "1. View Accounts" << std::endl;
        std::cout << "2. Create Account" << std::endl;
        std::cout << "3. Deposit" << std::endl;
        std::cout << "4. Withdraw" << std::endl;
        std::cout << "5. Transfer" << std::endl;
        std::cout << "6. Transaction History" << std::endl;
        std::cout << "7. Account Details" << std::endl;
        std::cout << "8. Test Concurrent Transfers" << std::endl;
        std::cout << "9. Logout" << std::endl;
        std::cout << "Choose an option: ";
        
        int choice = getIntInput();
        
        switch (choice) {
            case 1:
                viewAccounts();
                break;
            case 2:
                createAccount();
                break;
            case 3:
                handleDeposit();
                break;
            case 4:
                handleWithdraw();
                break;
            case 5:
                handleTransfer();
                break;
            case 6:
                viewTransactionHistory();
                break;
            case 7:
                viewAccountDetails();
                break;
            case 8:
                testConcurrentTransfers();
                break;
            case 9:
                bank_system.logoutUser();
                break;
            default:
                std::cout << "Invalid option. Please try again." << std::endl;
        }
    }

    void handleLogin() {
        std::cout << "\n=== Login ===" << std::endl;
        std::cout << "Email: ";
        std::string email = getStringInput();
        
        std::cout << "Password: ";
        std::string password = getPasswordInput();
        
        bank_system.loginUser(email, password);
    }

    void handleRegistration() {
        std::cout << "\n=== Register New User ===" << std::endl;
        std::cout << "Full Name: ";
        std::string name = getStringInput();
        
        std::cout << "Email: ";
        std::string email = getStringInput();
        
        std::cout << "Password (min 8 chars, must include uppercase, lowercase, digit, special char): ";
        std::string password = getPasswordInput();
        
        if (bank_system.registerUser(name, email, password)) {
            std::cout << "Registration successful! You can now login." << std::endl;
        } else {
            std::cout << "Registration failed. Please check your input and try again." << std::endl;
        }
    }

    void viewAccounts() {
        auto accounts = bank_system.getUserAccounts();
        
        if (accounts.empty()) {
            std::cout << "No accounts found. Create an account first." << std::endl;
            return;
        }
        
        std::cout << "\n=== Your Accounts ===" << std::endl;
        for (const auto& account : accounts) {
            std::cout << "Account ID: " << account->getAccountId()
                      << " | Type: " << account->getAccountTypeString()
                      << " | Balance: $" << std::fixed << std::setprecision(2) << account->getBalance()
                      << std::endl;
        }
    }

    void createAccount() {
        std::cout << "\n=== Create New Account ===" << std::endl;
        std::cout << "Account Type:" << std::endl;
        std::cout << "1. Savings Account" << std::endl;
        std::cout << "2. Current Account" << std::endl;
        std::cout << "Choose type: ";
        
        int type_choice = getIntInput();
        AccountType type = (type_choice == 1) ? AccountType::SAVINGS : AccountType::CURRENT;
        
        std::cout << "Initial deposit amount: $";
        double initial_balance = getDoubleInput();
        
        int account_id = bank_system.createAccount(type, initial_balance);
        if (account_id > 0) {
            std::cout << "Account created successfully!" << std::endl;
        }
    }

    void handleDeposit() {
        std::cout << "\n=== Deposit ===" << std::endl;
        std::cout << "Account ID: ";
        int account_id = getIntInput();
        
        std::cout << "Amount to deposit: $";
        double amount = getDoubleInput();
        
        bank_system.deposit(account_id, amount);
    }

    void handleWithdraw() {
        std::cout << "\n=== Withdraw ===" << std::endl;
        std::cout << "Account ID: ";
        int account_id = getIntInput();
        
        std::cout << "Amount to withdraw: $";
        double amount = getDoubleInput();
        
        bank_system.withdraw(account_id, amount);
    }

    void handleTransfer() {
        std::cout << "\n=== Transfer ===" << std::endl;
        std::cout << "From Account ID: ";
        int from_account = getIntInput();
        
        std::cout << "To Account ID: ";
        int to_account = getIntInput();
        
        std::cout << "Amount to transfer: $";
        double amount = getDoubleInput();
        
        bank_system.transfer(from_account, to_account, amount);
    }

    void viewTransactionHistory() {
        auto accounts = bank_system.getUserAccounts();

        if (accounts.empty()) {
            std::cout << "No accounts found." << std::endl;
            return;
        }

        std::cout << "\n=== Transaction History ===" << std::endl;
        for (const auto& account : accounts) {
            std::cout << "Account " << account->getAccountId() << " transactions:" << std::endl;

            // Get transactions from BankSystem (which uses the in-memory cache)
            auto transactions = bank_system.getAccountTransactions(account->getAccountId());

            if (transactions.empty()) {
                std::cout << "  No transactions found." << std::endl;
            } else {
                std::cout << "  " << transactions.size() << " transactions found:" << std::endl;
                for (const auto& txn : transactions) {
                    std::cout << "    " << txn->toString() << std::endl;
                }
            }
            std::cout << std::endl;
        }
    }

    void viewAccountDetails() {
        std::cout << "\n=== Account Details ===" << std::endl;
        std::cout << "Account ID: ";
        int account_id = getIntInput();
        
        auto account = bank_system.getAccount(account_id);
        if (account && bank_system.validateAccountOwnership(account_id, bank_system.getCurrentUser()->getUserId())) {
            account->displayAccountInfo();
        } else {
            std::cout << "Account not found or access denied." << std::endl;
        }
    }

    void testConcurrentTransfers() {
        std::cout << "\n=== Testing Concurrent Transfers ===" << std::endl;
        std::cout << "This will demonstrate deadlock prevention..." << std::endl;
        
        auto accounts = bank_system.getUserAccounts();
        if (accounts.size() < 2) {
            std::cout << "Need at least 2 accounts for concurrent transfer test." << std::endl;
            return;
        }
        
        int acc1 = accounts[0]->getAccountId();
        int acc2 = accounts[1]->getAccountId();
        
        std::cout << "Starting concurrent transfers between accounts " << acc1 << " and " << acc2 << std::endl;
        
        // Launch concurrent transfers
        auto future1 = std::async(std::launch::async, [this, acc1, acc2]() {
            return bank_system.transfer(acc1, acc2, 100.0);
        });
        
        auto future2 = std::async(std::launch::async, [this, acc2, acc1]() {
            return bank_system.transfer(acc2, acc1, 50.0);
        });
        
        bool result1 = future1.get();
        bool result2 = future2.get();
        
        std::cout << "Transfer 1 result: " << (result1 ? "Success" : "Failed") << std::endl;
        std::cout << "Transfer 2 result: " << (result2 ? "Success" : "Failed") << std::endl;
    }

    void handleAdminPanel() {
        std::cout << "\n=== Admin Panel ===" << std::endl;
        std::cout << "1. View System Statistics" << std::endl;
        std::cout << "2. View All Users" << std::endl;
        std::cout << "3. View All Accounts" << std::endl;
        std::cout << "4. Deadlock Statistics" << std::endl;
        std::cout << "5. Back to Main Menu" << std::endl;
        std::cout << "Choose an option: ";
        
        int choice = getIntInput();
        
        switch (choice) {
            case 1:
                bank_system.displaySystemStats();
                break;
            case 2:
                bank_system.displayAllUsers();
                break;
            case 3:
                bank_system.displayAllAccounts();
                break;
            case 4:
                bank_system.getDeadlockManager().displayStatistics();
                break;
            case 5:
                break;
            default:
                std::cout << "Invalid option." << std::endl;
        }
    }

    // Input helper methods
    int getIntInput() {
        int value;
        while (!(std::cin >> value)) {
            std::cout << "Invalid input. Please enter a number: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return value;
    }

    double getDoubleInput() {
        double value;
        while (!(std::cin >> value) || value < 0) {
            std::cout << "Invalid input. Please enter a positive number: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return value;
    }

    std::string getStringInput() {
        std::string input;
        std::getline(std::cin, input);
        return Security::sanitizeInput(input);
    }

    std::string getPasswordInput() {
        std::string password;
        std::getline(std::cin, password);
        return password; // Don't sanitize passwords
    }
};

int main() {
    try {
        BankingCLI cli;
        cli.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
