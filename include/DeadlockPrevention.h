#ifndef DEADLOCK_PREVENTION_H
#define DEADLOCK_PREVENTION_H

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>
#include <chrono>
#include <condition_variable>

enum class DeadlockStrategy {
    LOCK_ORDERING,      // Always lock accounts in ascending order of account_id
    WAIT_DIE,          // Older transaction waits, younger dies
    WOUND_WAIT,        // Older transaction wounds younger, younger waits
    TIMEOUT_ROLLBACK   // Rollback if waiting too long
};

struct LockRequest {
    size_t thread_hash;
    int account_id;
    std::chrono::steady_clock::time_point request_time;
    int transaction_id;

    LockRequest(size_t t_hash, int acc_id, int txn_id)
        : thread_hash(t_hash), account_id(acc_id),
          request_time(std::chrono::steady_clock::now()), transaction_id(txn_id) {}
};

class DeadlockPrevention {
private:
    DeadlockStrategy strategy;
    std::mutex wait_graph_mutex;
    std::unordered_map<size_t, std::unordered_set<int>> thread_locks;
    std::unordered_map<size_t, std::vector<LockRequest>> waiting_requests;
    std::unordered_map<int, size_t> account_owners;
    std::unordered_map<size_t, int> thread_timestamps;
    
    // Timeout settings
    std::chrono::milliseconds lock_timeout;
    std::chrono::milliseconds deadlock_check_interval;
    
    // Statistics
    int deadlocks_detected;
    int deadlocks_prevented;
    int transactions_aborted;

public:
    // Constructor
    explicit DeadlockPrevention(DeadlockStrategy strategy = DeadlockStrategy::LOCK_ORDERING);
    
    // Destructor
    ~DeadlockPrevention();

    // Lock management with deadlock prevention
    bool requestLocks(const std::vector<int>& account_ids, int transaction_id);
    void releaseLocks(const std::vector<int>& account_ids);
    void releaseAllLocks();
    
    // Deadlock detection and resolution
    bool detectDeadlock();
    bool resolveDeadlock();
    std::vector<size_t> findDeadlockCycle();
    
    // Strategy-specific methods
    bool lockOrderingStrategy(const std::vector<int>& account_ids, int transaction_id);
    bool waitDieStrategy(const std::vector<int>& account_ids, int transaction_id);
    bool woundWaitStrategy(const std::vector<int>& account_ids, int transaction_id);
    bool timeoutRollbackStrategy(const std::vector<int>& account_ids, int transaction_id);
    
    // Utility methods
    void setStrategy(DeadlockStrategy new_strategy);
    DeadlockStrategy getStrategy() const;
    void setTimeout(std::chrono::milliseconds timeout);
    
    // Statistics and monitoring
    int getDeadlocksDetected() const;
    int getDeadlocksPrevented() const;
    int getTransactionsAborted() const;
    void resetStatistics();
    void displayStatistics() const;
    
    // Thread management
    void registerThread(int transaction_id);
    void unregisterThread();
    int getThreadTimestamp(size_t thread_hash) const;

    // Wait graph analysis
    bool hasCycle() const;
    std::vector<size_t> getWaitingThreads() const;
    std::vector<int> getLockedAccounts() const;

private:
    // Helper methods
    bool tryLockAccounts(const std::vector<int>& account_ids);
    void addToWaitGraph(size_t waiter, const std::vector<int>& account_ids);
    void removeFromWaitGraph(size_t thread_hash);
    bool isOlderTransaction(size_t t1, size_t t2) const;
    void abortTransaction(size_t thread_hash);
    size_t getThreadHash() const;

    // Cycle detection using DFS
    bool dfsHasCycle(size_t current,
                     std::unordered_set<size_t>& visited,
                     std::unordered_set<size_t>& recursion_stack) const;
};

#endif // DEADLOCK_PREVENTION_H
