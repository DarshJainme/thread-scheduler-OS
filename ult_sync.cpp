#include "ult_sync.h"

void ULTMutex::lock() {
    if (!locked) {
        locked = true;
        return;
    }
    // enqueue current ULT and yield to scheduler
    waiters.push_back(g_current_idx);
    swapcontext(&g_contexts[g_current_idx].ctx, &sched_ctx);
}

void ULTMutex::unlock() {
    if (waiters.empty()) {
        locked = false;
    } else {
        size_t next = waiters.front(); waiters.pop_front();
        // make next thread runnable
        ready_queue.push_back(next);
    }
}

void ULTCondVar::wait(ULTMutex &m) {
    // enqueue on condvar
    waiters.push_back(g_current_idx);
    // release lock
    m.unlock();
    // yield to scheduler
    swapcontext(&g_contexts[g_current_idx].ctx, &sched_ctx);
    // reacquire lock on wake
    m.lock();
}

void ULTCondVar::signal() {
    if (!waiters.empty()) {
        size_t idx = waiters.front(); waiters.pop_front();
        ready_queue.push_back(idx);
    }
}

void ULTCondVar::broadcast() {
    while (!waiters.empty()) {
        size_t idx = waiters.front(); waiters.pop_front();
        ready_queue.push_back(idx);
    }
}
