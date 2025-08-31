#include "DeadlockPrevention.h"
#include <iostream>
#include <algorithm>
#include <thread>

// Constructor
DeadlockPrevention::DeadlockPrevention(DeadlockStrategy strategy)
    : strategy(strategy), lock_timeout(std::chrono::milliseconds(5000)),
      deadlock_check_interval(std::chrono::milliseconds(100)),
      deadlocks_detected(0), deadlocks_prevented(0), transactions_aborted(0) {}

// Destructor
DeadlockPrevention::~DeadlockPrevention() {}

// Request locks with deadlock prevention
bool DeadlockPrevention::requestLocks(const std::vector<int>& account_ids, int transaction_id) {
    registerThread(transaction_id);
    
    switch (strategy) {
        case DeadlockStrategy::LOCK_ORDERING:
            return lockOrderingStrategy(account_ids, transaction_id);
        case DeadlockStrategy::WAIT_DIE:
            return waitDieStrategy(account_ids, transaction_id);
        case DeadlockStrategy::WOUND_WAIT:
            return woundWaitStrategy(account_ids, transaction_id);
        case DeadlockStrategy::TIMEOUT_ROLLBACK:
            return timeoutRollbackStrategy(account_ids, transaction_id);
        default:
            return lockOrderingStrategy(account_ids, transaction_id);
    }
}

// Release locks for specific accounts
void DeadlockPrevention::releaseLocks(const std::vector<int>& account_ids) {
    std::lock_guard<std::mutex> lock(wait_graph_mutex);

    size_t thread_hash = getThreadHash();

    // Remove from thread_locks
    if (thread_locks.find(thread_hash) != thread_locks.end()) {
        for (int account_id : account_ids) {
            thread_locks[thread_hash].erase(account_id);
            account_owners.erase(account_id);
        }

        if (thread_locks[thread_hash].empty()) {
            thread_locks.erase(thread_hash);
        }
    }

    // Remove from waiting requests
    waiting_requests.erase(thread_hash);
}

// Release all locks for current thread
void DeadlockPrevention::releaseAllLocks() {
    std::lock_guard<std::mutex> lock(wait_graph_mutex);

    size_t thread_hash = getThreadHash();

    // Remove all locks held by this thread
    if (thread_locks.find(thread_hash) != thread_locks.end()) {
        for (int account_id : thread_locks[thread_hash]) {
            account_owners.erase(account_id);
        }
        thread_locks.erase(thread_hash);
    }

    // Remove from waiting requests
    waiting_requests.erase(thread_hash);

    // Unregister thread
    unregisterThread();
}

// Get thread hash (simplified version)
size_t DeadlockPrevention::getThreadHash() const {
    // Simple hash based on thread id address (for demonstration)
    auto tid = std::this_thread::get_id();
    return reinterpret_cast<size_t>(&tid) % 1000000;
}

// Lock ordering strategy (prevent deadlock by ordering)
bool DeadlockPrevention::lockOrderingStrategy(const std::vector<int>& account_ids, int transaction_id) {
    (void)transaction_id; // Suppress unused parameter warning

    // Sort account IDs to ensure consistent locking order
    std::vector<int> sorted_ids = account_ids;
    std::sort(sorted_ids.begin(), sorted_ids.end());

    std::lock_guard<std::mutex> lock(wait_graph_mutex);

    size_t thread_hash = getThreadHash();

    // Check if all accounts are available
    for (int account_id : sorted_ids) {
        if (account_owners.find(account_id) != account_owners.end() &&
            account_owners[account_id] != thread_hash) {
            return false; // Account is locked by another thread
        }
    }

    // Acquire all locks
    for (int account_id : sorted_ids) {
        account_owners[account_id] = thread_hash;
        thread_locks[thread_hash].insert(account_id);
    }

    deadlocks_prevented++;
    return true;
}

// Wait-Die strategy
bool DeadlockPrevention::waitDieStrategy(const std::vector<int>& account_ids, int transaction_id) {
    std::lock_guard<std::mutex> lock(wait_graph_mutex);

    size_t thread_hash = getThreadHash();

    // Check if any required account is locked
    for (int account_id : account_ids) {
        if (account_owners.find(account_id) != account_owners.end()) {
            auto owner_thread = account_owners[account_id];

            if (owner_thread != thread_hash) {
                // If this transaction is older, wait; if younger, die
                if (isOlderTransaction(thread_hash, owner_thread)) {
                    // Wait - add to waiting list
                    for (int acc_id : account_ids) {
                        waiting_requests[thread_hash].emplace_back(thread_hash, acc_id, transaction_id);
                    }
                    return false;
                } else {
                    // Die - abort this transaction
                    transactions_aborted++;
                    return false;
                }
            }
        }
    }

    // All accounts available, acquire locks
    for (int account_id : account_ids) {
        account_owners[account_id] = thread_hash;
        thread_locks[thread_hash].insert(account_id);
    }

    return true;
}

// Wound-Wait strategy
bool DeadlockPrevention::woundWaitStrategy(const std::vector<int>& account_ids, int transaction_id) {
    (void)transaction_id; // Suppress unused parameter warning

    std::lock_guard<std::mutex> lock(wait_graph_mutex);

    size_t thread_hash = getThreadHash();

    // Check if any required account is locked
    for (int account_id : account_ids) {
        if (account_owners.find(account_id) != account_owners.end()) {
            auto owner_thread = account_owners[account_id];

            if (owner_thread != thread_hash) {
                if (isOlderTransaction(thread_hash, owner_thread)) {
                    // Wound the younger transaction
                    abortTransaction(owner_thread);
                } else {
                    // Wait
                    for (int acc_id : account_ids) {
                        waiting_requests[thread_hash].emplace_back(thread_hash, acc_id, transaction_id);
                    }
                    return false;
                }
            }
        }
    }

    // Acquire locks
    for (int account_id : account_ids) {
        account_owners[account_id] = thread_hash;
        thread_locks[thread_hash].insert(account_id);
    }

    return true;
}

// Timeout rollback strategy
bool DeadlockPrevention::timeoutRollbackStrategy(const std::vector<int>& account_ids, int transaction_id) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < lock_timeout) {
        {
            std::lock_guard<std::mutex> lock(wait_graph_mutex);
            
            // Try to acquire all locks
            if (tryLockAccounts(account_ids)) {
                size_t thread_hash = getThreadHash();
                for (int account_id : account_ids) {
                    account_owners[account_id] = thread_hash;
                    thread_locks[thread_hash].insert(account_id);
                }
                return true;
            }
        }
        
        // Wait before retrying
        std::this_thread::sleep_for(deadlock_check_interval);
    }
    
    // Timeout reached
    transactions_aborted++;
    return false;
}

// Detect deadlock
bool DeadlockPrevention::detectDeadlock() {
    std::lock_guard<std::mutex> lock(wait_graph_mutex);
    return hasCycle();
}

// Resolve deadlock
bool DeadlockPrevention::resolveDeadlock() {
    auto cycle = findDeadlockCycle();
    if (!cycle.empty()) {
        // Abort the youngest transaction in the cycle
        auto youngest = *std::max_element(cycle.begin(), cycle.end(),
            [this](const auto& a, const auto& b) {
                return isOlderTransaction(a, b);
            });
        
        abortTransaction(youngest);
        deadlocks_detected++;
        return true;
    }
    return false;
}

// Find deadlock cycle
std::vector<size_t> DeadlockPrevention::findDeadlockCycle() {
    std::unordered_set<size_t> visited;
    std::unordered_set<size_t> recursion_stack;

    for (const auto& [thread_hash, locks] : thread_locks) {
        if (visited.find(thread_hash) == visited.end()) {
            if (dfsHasCycle(thread_hash, visited, recursion_stack)) {
                // Return the cycle (simplified - return all threads in recursion stack)
                std::vector<size_t> cycle;
                for (const auto& tid : recursion_stack) {
                    cycle.push_back(tid);
                }
                return cycle;
            }
        }
    }

    return {};
}

// Register thread with timestamp
void DeadlockPrevention::registerThread(int transaction_id) {
    std::lock_guard<std::mutex> lock(wait_graph_mutex);
    thread_timestamps[getThreadHash()] = transaction_id;
}

// Unregister thread
void DeadlockPrevention::unregisterThread() {
    std::lock_guard<std::mutex> lock(wait_graph_mutex);
    size_t thread_hash = getThreadHash();
    thread_timestamps.erase(thread_hash);
    thread_locks.erase(thread_hash);
    waiting_requests.erase(thread_hash);
}

// Get thread timestamp
int DeadlockPrevention::getThreadTimestamp(size_t thread_hash) const {
    auto it = thread_timestamps.find(thread_hash);
    return (it != thread_timestamps.end()) ? it->second : 0;
}

// Check if transaction is older
bool DeadlockPrevention::isOlderTransaction(size_t t1, size_t t2) const {
    return getThreadTimestamp(t1) < getThreadTimestamp(t2);
}

// Try to lock accounts
bool DeadlockPrevention::tryLockAccounts(const std::vector<int>& account_ids) {
    for (int account_id : account_ids) {
        if (account_owners.find(account_id) != account_owners.end()) {
            return false;
        }
    }
    return true;
}

// Abort transaction
void DeadlockPrevention::abortTransaction(size_t thread_hash) {
    // Release all locks held by this thread
    if (thread_locks.find(thread_hash) != thread_locks.end()) {
        for (int account_id : thread_locks[thread_hash]) {
            account_owners.erase(account_id);
        }
        thread_locks.erase(thread_hash);
    }

    waiting_requests.erase(thread_hash);
    transactions_aborted++;
}

// DFS cycle detection
bool DeadlockPrevention::dfsHasCycle(size_t current,
                                   std::unordered_set<size_t>& visited,
                                   std::unordered_set<size_t>& recursion_stack) const {
    visited.insert(current);
    recursion_stack.insert(current);

    // Check all threads this thread is waiting for
    if (waiting_requests.find(current) != waiting_requests.end()) {
        for (const auto& request : waiting_requests.at(current)) {
            auto owner_it = account_owners.find(request.account_id);
            if (owner_it != account_owners.end()) {
                auto owner_thread = owner_it->second;

                if (recursion_stack.find(owner_thread) != recursion_stack.end()) {
                    return true; // Cycle detected
                }

                if (visited.find(owner_thread) == visited.end()) {
                    if (dfsHasCycle(owner_thread, visited, recursion_stack)) {
                        return true;
                    }
                }
            }
        }
    }

    recursion_stack.erase(current);
    return false;
}

// Display statistics
void DeadlockPrevention::displayStatistics() const {
    std::cout << "=== Deadlock Prevention Statistics ===" << std::endl;
    std::cout << "Strategy: ";
    switch (strategy) {
        case DeadlockStrategy::LOCK_ORDERING:
            std::cout << "Lock Ordering" << std::endl;
            break;
        case DeadlockStrategy::WAIT_DIE:
            std::cout << "Wait-Die" << std::endl;
            break;
        case DeadlockStrategy::WOUND_WAIT:
            std::cout << "Wound-Wait" << std::endl;
            break;
        case DeadlockStrategy::TIMEOUT_ROLLBACK:
            std::cout << "Timeout Rollback" << std::endl;
            break;
    }
    std::cout << "Deadlocks Detected: " << deadlocks_detected << std::endl;
    std::cout << "Deadlocks Prevented: " << deadlocks_prevented << std::endl;
    std::cout << "Transactions Aborted: " << transactions_aborted << std::endl;
    std::cout << "======================================" << std::endl;
}

// Check if there's a cycle in the wait graph
bool DeadlockPrevention::hasCycle() const {
    std::unordered_set<size_t> visited;
    std::unordered_set<size_t> recursion_stack;

    for (const auto& [thread_hash, locks] : thread_locks) {
        if (visited.find(thread_hash) == visited.end()) {
            if (dfsHasCycle(thread_hash, visited, recursion_stack)) {
                return true;
            }
        }
    }

    return false;
}

// Get strategy
DeadlockStrategy DeadlockPrevention::getStrategy() const {
    return strategy;
}

// Set strategy
void DeadlockPrevention::setStrategy(DeadlockStrategy new_strategy) {
    strategy = new_strategy;
}

// Set timeout
void DeadlockPrevention::setTimeout(std::chrono::milliseconds timeout) {
    lock_timeout = timeout;
}

// Get statistics
int DeadlockPrevention::getDeadlocksDetected() const {
    return deadlocks_detected;
}

int DeadlockPrevention::getDeadlocksPrevented() const {
    return deadlocks_prevented;
}

int DeadlockPrevention::getTransactionsAborted() const {
    return transactions_aborted;
}

// Reset statistics
void DeadlockPrevention::resetStatistics() {
    deadlocks_detected = 0;
    deadlocks_prevented = 0;
    transactions_aborted = 0;
}

// Get waiting threads
std::vector<size_t> DeadlockPrevention::getWaitingThreads() const {
    std::vector<size_t> waiting;
    for (const auto& [thread_hash, requests] : waiting_requests) {
        if (!requests.empty()) {
            waiting.push_back(thread_hash);
        }
    }
    return waiting;
}

// Get locked accounts
std::vector<int> DeadlockPrevention::getLockedAccounts() const {
    std::vector<int> locked;
    for (const auto& [account_id, owner] : account_owners) {
        locked.push_back(account_id);
    }
    return locked;
}
