#ifndef THREADCONTROL_H
#define THREADCONTROL_H

#include <cstddef>
#include <vector>
#include "ucontext_stubs.h"  // Include the fake ucontext_t definition

// Forward-declared scheduler context globals
extern ucontext_t sched_ctx;
extern size_t g_current_idx;

// User-level thread context structure
// struct ULTContext {
//     ucontext_t ctx; // Using the simulated ucontext_t from ucontext_stubs.h
//     void* stack;    // Pointer to stack (for simulation)
//     bool finished;
// };

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
