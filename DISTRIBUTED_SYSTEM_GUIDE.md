# Distributed ATM Banking System

This project implements a complete distributed ATM banking system with separate Bank Server and ATM Machine components that communicate over the network.

## 🏗️ System Architecture

```
┌─────────────────────┐         Network         ┌─────────────────────┐
│   BANK SERVER       │◄─────── TCP/IP ────────►│   ATM MACHINE       │
│   (Main System)     │      Encrypted JSON     │   (Remote Client)   │
│                     │                         │                     │
│ • Database          │                         │ • User Interface    │
│ • Authentication    │                         │ • Input Validation  │
│ • Transaction Logic │                         │ • Network Client    │
│ • Session Management│                         │ • Local Operations  │
└─────────────────────┘                         └─────────────────────┘
```

## 📁 Project Structure

### Bank Server (Main Directory)
```
Banking_Management/
├── src/
│   ├── main.cpp                 # Main banking system
│   ├── bank_server_main.cpp     # Bank server application
│   ├── BankServer.cpp           # Server implementation
│   ├── BankSystem.cpp           # Core banking logic
│   ├── DatabaseHandler.cpp     # Database operations
│   ├── User.cpp, Account.cpp    # Core entities
│   ├── Security.cpp             # Password hashing
│   ├── Encryption.cpp           # Network encryption
│   ├── JsonHandler.cpp          # JSON protocol
│   └── NetworkProtocol.cpp      # Network messages
├── include/                     # Header files
├── bin/
│   ├── banking_system           # Main banking app
│   └── bank_server              # Network server
└── Makefile                     # Build system
```

### ATM Machine (Separate Directory)
```
ATM_Machine/
├── src/
│   ├── atm_main.cpp             # ATM application entry
│   ├── ATMClient.cpp            # ATM client implementation
│   ├── Encryption.cpp           # Shared encryption
│   ├── JsonHandler.cpp          # Shared JSON handling
│   └── NetworkProtocol.cpp      # Shared network protocol
├── include/
│   ├── ATMClient.h              # ATM client header
│   ├── Common.h                 # Shared constants
│   ├── Encryption.h             # Shared encryption
│   ├── JsonHandler.h            # Shared JSON handling
│   └── NetworkProtocol.h        # Shared network protocol
├── bin/
│   └── atm_client               # ATM executable
├── Makefile                     # ATM build system
└── README.md                    # ATM documentation
```

## 🚀 Quick Start

### 1. Start Bank Server
```bash
# In main directory
make run-server
```

### 2. Start ATM Machine
```bash
# In ATM_Machine directory
cd ATM_Machine
make run
```

### 3. Connect to Remote Bank Server
```bash
# ATM connecting to remote server
cd ATM_Machine
make run-server HOST=192.168.1.100 PORT=8080
```

## 🔧 Building Components

### Build Bank Server
```bash
make all                    # Build both banking system and server
make run                    # Run main banking system
make run-server            # Run network server
```

### Build ATM Machine
```bash
cd ATM_Machine
make all                    # Build ATM client
make run                    # Connect to localhost:8080
make run-server HOST=<ip>   # Connect to remote server
```

## 🌐 Network Communication

### Protocol
- **Transport**: TCP sockets
- **Encryption**: XOR cipher + Base64 encoding
- **Format**: `MESSAGE_TYPE|JSON_PAYLOAD`
- **Port**: 8080 (default)

### Message Types
- `LOGIN_REQUEST` / `LOGIN_RESPONSE`
- `BALANCE_REQUEST` / `BALANCE_RESPONSE`
- `WITHDRAW_REQUEST` / `WITHDRAW_RESPONSE`
- `LOGOUT_REQUEST` / `LOGOUT_RESPONSE`

### Security Features
- Session-based authentication
- Encrypted communication
- Password hashing (bcrypt-style)
- Account ownership validation
- Transaction logging

## 🧪 Testing the System

### Test Scenario 1: Local Connection
1. Start bank server: `make run-server`
2. Start ATM: `cd ATM_Machine && make run`
3. Login with: `atm@test.com` / `Password123!`
4. Test balance check and withdrawal

### Test Scenario 2: Multiple ATMs
1. Start bank server: `make run-server`
2. Start ATM 1: `cd ATM_Machine && make run`
3. Start ATM 2: `cd ATM_Machine && ./bin/atm_client`
4. Test concurrent operations

### Test Scenario 3: Remote Connection
1. Start bank server on machine A
2. Start ATM on machine B: `./bin/atm_client <server_ip> 8080`
3. Test network communication

## 📊 System Features

### Bank Server Features
- ✅ Multi-threaded client handling
- ✅ SQLite database with ACID transactions
- ✅ Session management
- ✅ Real-time balance synchronization
- ✅ Transaction logging
- ✅ Deadlock prevention
- ✅ Security logging

### ATM Machine Features
- ✅ User-friendly interface
- ✅ Secure authentication
- ✅ Real-time balance checking
- ✅ Money withdrawal
- ✅ Transaction receipts
- ✅ Session management
- ✅ Network error handling

## 🔒 Security Implementation

- **Password Security**: Salted hash storage
- **Network Security**: XOR + Base64 encryption
- **Session Security**: Token-based authentication
- **Database Security**: Parameterized queries
- **Account Security**: Ownership validation

## 🎯 Production Deployment

The system is designed for real-world deployment:
- ATM machines can be deployed on separate hardware
- Bank server can handle multiple concurrent ATM connections
- Database persistence ensures data integrity
- Network protocol supports remote connections
- Security measures protect against common attacks

## 🏆 Successfully Tested Features

✅ **Authentication**: Login/logout with encrypted credentials
✅ **Balance Checking**: Real-time account balance queries  
✅ **Money Withdrawal**: Secure transactions with balance updates
✅ **Network Communication**: Encrypted client-server messaging
✅ **Session Management**: Secure token-based sessions
✅ **Database Persistence**: SQLite with transaction logging
✅ **Multi-client Support**: Multiple ATM connections
✅ **Error Handling**: Graceful error recovery
✅ **Separation of Concerns**: Clean ATM/Server separation
