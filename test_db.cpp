#include <iostream>
#include <sqlite3.h>

int main() {
    sqlite3* db;
    
    // Remove old database
    remove("test.db");
    
    // Open database
    int rc = sqlite3_open("test.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    std::cout << "Database opened successfully" << std::endl;
    
    // Create table
    const char* create_sql = "CREATE TABLE IF NOT EXISTS test_accounts ("
                            "id INTEGER PRIMARY KEY, "
                            "balance REAL)";
    
    rc = sqlite3_exec(db, create_sql, 0, 0, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    std::cout << "Table created successfully" << std::endl;
    
    // Insert test data
    const char* insert_sql = "INSERT INTO test_accounts (id, balance) VALUES (1, 100.0)";
    rc = sqlite3_exec(db, insert_sql, 0, 0, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "Insert error: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    std::cout << "Data inserted successfully" << std::endl;
    
    // Update test
    const char* update_sql = "UPDATE test_accounts SET balance = 150.0 WHERE id = 1";
    rc = sqlite3_exec(db, update_sql, 0, 0, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "Update error: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    std::cout << "Data updated successfully" << std::endl;
    
    // Test prepared statement update
    const char* prep_sql = "UPDATE test_accounts SET balance = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    
    rc = sqlite3_prepare_v2(db, prep_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare error: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    sqlite3_bind_double(stmt, 1, 200.0);
    sqlite3_bind_int(stmt, 2, 1);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        std::cout << "Prepared statement update successful" << std::endl;
    } else {
        std::cerr << "Prepared statement error: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    sqlite3_close(db);
    std::cout << "All database operations completed successfully!" << std::endl;
    
    return 0;
}
