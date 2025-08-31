#include "ATMClient.h"
#include <iostream>
#include <string>

void showWelcomeMessage() {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "           WELCOME TO SECURE ATM" << std::endl;
    std::cout << "         Banking Management System" << std::endl;
    std::cout << "              Version 2.0" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Features:" << std::endl;
    std::cout << "• Secure encrypted communication" << std::endl;
    std::cout << "• Real-time balance checking" << std::endl;
    std::cout << "• Safe money withdrawal" << std::endl;
    std::cout << "• Multi-terminal synchronization" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

void showConnectionInfo() {
    std::cout << "\nConnection Information:" << std::endl;
    std::cout << "• Server: localhost:8080" << std::endl;
    std::cout << "• Encryption: XOR + Base64" << std::endl;
    std::cout << "• Protocol: JSON over TCP" << std::endl;
    std::cout << "• Security: Session-based authentication" << std::endl;
}

int main(int argc, char* argv[]) {
    showWelcomeMessage();
    
    std::string server_host = "127.0.0.1";
    int server_port = DEFAULT_BANK_PORT;
    
    // Parse command line arguments
    if (argc >= 2) {
        server_host = argv[1];
    }
    if (argc >= 3) {
        try {
            server_port = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid port number. Using default port " << DEFAULT_BANK_PORT << std::endl;
            server_port = DEFAULT_BANK_PORT;
        }
    }
    
    showConnectionInfo();
    
    try {
        // Create ATM client
        ATMClient atm_client(server_host, server_port);
        
        std::cout << "\nStarting ATM client..." << std::endl;
        std::cout << "Connecting to bank server..." << std::endl;
        
        // Run ATM interface
        atm_client.run();
        
    } catch (const std::exception& e) {
        std::cerr << "ATM Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nATM session ended. Goodbye!" << std::endl;
    return 0;
}
