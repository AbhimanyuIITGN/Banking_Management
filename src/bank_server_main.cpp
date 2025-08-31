#include "BankServer.h"
#include "BankSystem.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

// Global server instance for signal handling
BankServer* global_server = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down server..." << std::endl;
    if (global_server) {
        global_server->stop();
    }
}

int main() {
    std::cout << "=== Banking System Server ===" << std::endl;
    std::cout << "Initializing bank server..." << std::endl;
    
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Initialize bank system
        BankSystem& bank_system = BankSystem::getInstance();

        // CRITICAL: Initialize the database connection
        if (!bank_system.initialize()) {
            std::cerr << "Failed to initialize bank system database" << std::endl;
            return 1;
        }

        // Create and start server
        BankServer server(DEFAULT_BANK_PORT);
        global_server = &server;

        std::cout << "Bank system initialized successfully" << std::endl;
        std::cout << "Starting server on port " << DEFAULT_BANK_PORT << "..." << std::endl;
        
        // Start server in a separate thread
        std::thread server_thread([&server]() {
            server.start();
        });
        
        // Server management loop
        std::string command;
        while (server.isRunning()) {
            std::cout << "\nServer Commands:" << std::endl;
            std::cout << "1. stats - Show server statistics" << std::endl;
            std::cout << "2. clients - Show connected ATMs" << std::endl;
            std::cout << "3. stop - Stop server" << std::endl;
            std::cout << "Enter command: ";
            
            std::getline(std::cin, command);
            
            if (command == "stats" || command == "1") {
                server.displayServerStats();
                bank_system.displaySystemStats();
            } else if (command == "clients" || command == "2") {
                std::cout << "Active ATM connections: " << server.getActiveClientCount() << std::endl;
            } else if (command == "stop" || command == "3") {
                std::cout << "Stopping server..." << std::endl;
                server.stop();
                break;
            } else if (command == "help") {
                std::cout << "Available commands: stats, clients, stop" << std::endl;
            } else if (!command.empty()) {
                std::cout << "Unknown command: " << command << std::endl;
                std::cout << "Type 'help' for available commands" << std::endl;
            }
            
            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Wait for server thread to finish
        if (server_thread.joinable()) {
            server_thread.join();
        }
        
        std::cout << "Bank server shutdown complete" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
