# ATM Machine Client

This is the ATM machine client that connects to the Bank Server over the network.

## Features

- **Secure Communication**: XOR + Base64 encrypted messages
- **User Authentication**: Login with email and password
- **Balance Checking**: Real-time account balance queries
- **Money Withdrawal**: Secure withdrawal with transaction tracking
- **Session Management**: Secure logout and session cleanup

## Building

```bash
make all
```

## Running

### Connect to Local Bank Server (localhost:8080)
```bash
make run
```

### Connect to Remote Bank Server
```bash
make run-server HOST=192.168.1.100 PORT=8080
```

Or run directly:
```bash
./bin/atm_client 192.168.1.100 8080
```

## Usage

1. **Start the ATM Client**
   - The ATM will display a welcome screen and connect to the bank server
   - Each ATM gets a unique ID (e.g., ATM-1234)

2. **Login**
   - Enter your email address
   - Enter your password
   - The system will authenticate with the bank server

3. **ATM Operations**
   - **Check Balance**: View account balance and type
   - **Withdraw Money**: Withdraw cash with real-time balance updates
   - **Logout**: Secure session termination

4. **Exit**
   - Choose option 4 to exit the ATM

## Network Protocol

- **Encryption**: XOR cipher with Base64 encoding
- **Message Format**: `MESSAGE_TYPE|JSON_PAYLOAD`
- **Session Security**: Token-based authentication
- **Transport**: TCP sockets

## Example Session

```
=== ATM Login ===
Enter email: user@example.com
Enter password: ********
Login successful! Welcome, John Doe

=== ATM Menu ===
1. Check Balance
2. Withdraw Money
3. Logout
4. Exit

Select option: 1
Enter account number: 6
Current Balance: $500.00

Select option: 2
Enter account number: 6
Enter withdrawal amount: $100
Withdrawal Successful! New Balance: $400.00
Transaction ID: TXN-1756592652
```

## Requirements

- C++17 compatible compiler
- Network access to bank server
- LLVM/Clang++ (recommended)

## Security Features

- All communication is encrypted
- Session tokens for authentication
- Secure password handling
- Transaction logging
- Account ownership validation
