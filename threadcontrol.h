// File: threadcontrol.h — updated for ULT context management
#ifndef THREADCONTROL_H
#define THREADCONTROL_H

#include <cstddef>
#include <ucontext.h>
#include <vector>

// Forward-declared scheduler context globals
extern ucontext_t sched_ctx;
extern size_t g_current_idx;

// User-level thread context structure
struct ULTContext {
    ucontext_t ctx;
    bool finished;
};

// Shared contexts array (defined in scheduler.cpp)
extern std::vector<ULTContext> g_contexts;

class ThreadControl {
public:
    ThreadControl();

    // Called inside a ULT: yields to scheduler until resumed
    void waitUntilRunnable();

    // No-op for ULT: scheduler directly swaps in
    void wake();

    // Signal that this ULT should terminate
    void finish();

    // Check if finish() has been called for this ULT
    bool isFinished() const;
};

#endif // THREADCONTROL_H
