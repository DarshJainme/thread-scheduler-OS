#ifndef ULT_SYNC_H
#define ULT_SYNC_H

#include <vector>
#include <deque>
#include <cstddef>
#include "ucontext_stubs.h"

// Externals from scheduler
extern ucontext_t sched_ctx;
extern std::deque<size_t> ready_queue;      // indices of runnable ULTs (using deque)
extern size_t g_current_idx;
extern std::vector<ULTContext> g_contexts;

// User-Level Mutex
class ULTMutex {
public:
    ULTMutex() : locked(false) {}
    void lock();
    void unlock();
private:
    bool locked;
    std::deque<size_t> waiters;  // ULT indices waiting on this mutex
};

// User-Level Condition Variable
class ULTCondVar {
public:
    ULTCondVar() {}
    void wait(ULTMutex &m);
    void signal();
    void broadcast();
private:
    std::deque<size_t> waiters;  // ULT indices waiting on this condvar
};

#endif
