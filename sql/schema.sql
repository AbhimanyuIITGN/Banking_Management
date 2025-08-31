-- Banking Management System Database Schema
-- Compatible with both MySQL and SQLite

-- Drop tables if they exist (for clean setup)
DROP TABLE IF EXISTS Transactions;
DROP TABLE IF EXISTS Accounts;
DROP TABLE IF EXISTS Users;

-- Users table
CREATE TABLE Users (
    user_id INTEGER PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    salt VARCHAR(32),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    failed_login_attempts INTEGER DEFAULT 0,
    last_login TIMESTAMP NULL,
    INDEX idx_email (email),
    INDEX idx_active (is_active)
);

-- Accounts table
CREATE TABLE Accounts (
    account_id INTEGER PRIMARY KEY AUTO_INCREMENT,
    user_id INTEGER NOT NULL,
    balance DECIMAL(15,2) NOT NULL DEFAULT 0.00,
    account_type ENUM('SAVINGS', 'CURRENT') NOT NULL DEFAULT 'SAVINGS',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    interest_rate DECIMAL(5,4) DEFAULT 0.0350, -- 3.5% annual interest for savings
    minimum_balance DECIMAL(10,2) DEFAULT 0.00,
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_account_type (account_type),
    INDEX idx_active (is_active),
    CONSTRAINT chk_balance CHECK (balance >= 0),
    CONSTRAINT chk_interest_rate CHECK (interest_rate >= 0 AND interest_rate <= 1)
);

-- Transactions table
CREATE TABLE Transactions (
    transaction_id INTEGER PRIMARY KEY AUTO_INCREMENT,
    from_account_id INTEGER,
    to_account_id INTEGER,
    amount DECIMAL(15,2) NOT NULL,
    transaction_type ENUM('DEPOSIT', 'WITHDRAWAL', 'TRANSFER', 'INTEREST') NOT NULL,
    status ENUM('SUCCESS', 'FAILED', 'PENDING') NOT NULL DEFAULT 'PENDING',
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP NULL,
    reference_number VARCHAR(50) UNIQUE,
    FOREIGN KEY (from_account_id) REFERENCES Accounts(account_id),
    FOREIGN KEY (to_account_id) REFERENCES Accounts(account_id),
    INDEX idx_from_account (from_account_id),
    INDEX idx_to_account (to_account_id),
    INDEX idx_transaction_type (transaction_type),
    INDEX idx_status (status),
    INDEX idx_created_at (created_at),
    INDEX idx_reference_number (reference_number),
    CONSTRAINT chk_amount CHECK (amount > 0),
    CONSTRAINT chk_accounts CHECK (
        (transaction_type = 'DEPOSIT' AND from_account_id IS NULL AND to_account_id IS NOT NULL) OR
        (transaction_type = 'WITHDRAWAL' AND from_account_id IS NOT NULL AND to_account_id IS NULL) OR
        (transaction_type = 'TRANSFER' AND from_account_id IS NOT NULL AND to_account_id IS NOT NULL AND from_account_id != to_account_id) OR
        (transaction_type = 'INTEREST' AND from_account_id IS NULL AND to_account_id IS NOT NULL)
    )
);

-- Session management table (for security)
CREATE TABLE Sessions (
    session_id VARCHAR(128) PRIMARY KEY,
    user_id INTEGER NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    ip_address VARCHAR(45),
    user_agent TEXT,
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_expires_at (expires_at),
    INDEX idx_active (is_active)
);

-- Audit log table (for security and compliance)
CREATE TABLE AuditLog (
    log_id INTEGER PRIMARY KEY AUTO_INCREMENT,
    user_id INTEGER,
    action VARCHAR(100) NOT NULL,
    table_name VARCHAR(50),
    record_id INTEGER,
    old_values JSON,
    new_values JSON,
    ip_address VARCHAR(45),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    INDEX idx_user_id (user_id),
    INDEX idx_action (action),
    INDEX idx_created_at (created_at)
);

-- Insert default admin user (password: admin123)
INSERT INTO Users (name, email, password_hash, salt) VALUES 
('System Administrator', 'admin@bank.com', 
 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 
 'admin_salt_2024');

-- Insert sample accounts for testing
INSERT INTO Accounts (user_id, balance, account_type, minimum_balance) VALUES 
(1, 10000.00, 'SAVINGS', 1000.00),
(1, 5000.00, 'CURRENT', 0.00);

-- Create views for reporting
CREATE VIEW AccountSummary AS
SELECT 
    u.user_id,
    u.name,
    u.email,
    a.account_id,
    a.account_type,
    a.balance,
    a.created_at as account_created,
    (SELECT COUNT(*) FROM Transactions t 
     WHERE t.from_account_id = a.account_id OR t.to_account_id = a.account_id) as transaction_count
FROM Users u
JOIN Accounts a ON u.user_id = a.user_id
WHERE u.is_active = TRUE AND a.is_active = TRUE;

CREATE VIEW TransactionSummary AS
SELECT 
    t.transaction_id,
    t.transaction_type,
    t.amount,
    t.status,
    t.created_at,
    u_from.name as from_user_name,
    u_to.name as to_user_name,
    a_from.account_id as from_account,
    a_to.account_id as to_account
FROM Transactions t
LEFT JOIN Accounts a_from ON t.from_account_id = a_from.account_id
LEFT JOIN Accounts a_to ON t.to_account_id = a_to.account_id
LEFT JOIN Users u_from ON a_from.user_id = u_from.user_id
LEFT JOIN Users u_to ON a_to.user_id = u_to.user_id
ORDER BY t.created_at DESC;
