-- Banking Management System Database Schema for SQLite
-- SQLite-compatible version

-- Drop tables if they exist (for clean setup)
DROP TABLE IF EXISTS Transactions;
DROP TABLE IF EXISTS Accounts;
DROP TABLE IF EXISTS Users;

-- Users table
CREATE TABLE Users (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    email TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    salt TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    is_active INTEGER DEFAULT 1,
    failed_login_attempts INTEGER DEFAULT 0,
    last_login DATETIME
);

-- Create indexes for Users table
CREATE INDEX idx_users_email ON Users(email);
CREATE INDEX idx_users_active ON Users(is_active);

-- Accounts table
CREATE TABLE Accounts (
    account_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    balance REAL NOT NULL DEFAULT 0.00,
    account_type TEXT NOT NULL DEFAULT 'SAVINGS',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    is_active INTEGER DEFAULT 1,
    interest_rate REAL DEFAULT 0.0350,
    minimum_balance REAL DEFAULT 0.00,
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    CHECK (balance >= 0),
    CHECK (interest_rate >= 0 AND interest_rate <= 1),
    CHECK (account_type IN ('SAVINGS', 'CURRENT'))
);

-- Create indexes for Accounts table
CREATE INDEX idx_accounts_user_id ON Accounts(user_id);
CREATE INDEX idx_accounts_type ON Accounts(account_type);
CREATE INDEX idx_accounts_active ON Accounts(is_active);

-- Transactions table
CREATE TABLE Transactions (
    transaction_id INTEGER PRIMARY KEY AUTOINCREMENT,
    from_account_id INTEGER,
    to_account_id INTEGER,
    amount REAL NOT NULL,
    transaction_type TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'PENDING',
    description TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    completed_at DATETIME,
    reference_number TEXT UNIQUE,
    FOREIGN KEY (from_account_id) REFERENCES Accounts(account_id),
    FOREIGN KEY (to_account_id) REFERENCES Accounts(account_id),
    CHECK (amount > 0),
    CHECK (transaction_type IN ('DEPOSIT', 'WITHDRAWAL', 'TRANSFER', 'INTEREST')),
    CHECK (status IN ('SUCCESS', 'FAILED', 'PENDING'))
);

-- Create indexes for Transactions table
CREATE INDEX idx_transactions_from_account ON Transactions(from_account_id);
CREATE INDEX idx_transactions_to_account ON Transactions(to_account_id);
CREATE INDEX idx_transactions_type ON Transactions(transaction_type);
CREATE INDEX idx_transactions_status ON Transactions(status);
CREATE INDEX idx_transactions_created_at ON Transactions(created_at);
CREATE INDEX idx_transactions_reference ON Transactions(reference_number);

-- Sessions table (for security)
CREATE TABLE Sessions (
    session_id TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME NOT NULL,
    is_active INTEGER DEFAULT 1,
    ip_address TEXT,
    user_agent TEXT,
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE
);

-- Create indexes for Sessions table
CREATE INDEX idx_sessions_user_id ON Sessions(user_id);
CREATE INDEX idx_sessions_expires_at ON Sessions(expires_at);
CREATE INDEX idx_sessions_active ON Sessions(is_active);

-- Audit log table
CREATE TABLE AuditLog (
    log_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    action TEXT NOT NULL,
    table_name TEXT,
    record_id INTEGER,
    old_values TEXT,
    new_values TEXT,
    ip_address TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES Users(user_id)
);

-- Create indexes for AuditLog table
CREATE INDEX idx_audit_user_id ON AuditLog(user_id);
CREATE INDEX idx_audit_action ON AuditLog(action);
CREATE INDEX idx_audit_created_at ON AuditLog(created_at);

-- Insert default admin user (password: admin123)
-- Hash for 'admin123' with default salt
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
WHERE u.is_active = 1 AND a.is_active = 1;

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
