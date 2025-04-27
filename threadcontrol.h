#ifndef THREADCONTROL_H
#define THREADCONTROL_H

#include <cstddef>
#include <vector>
#include "ult_context.h" // Include for ULTContext struct definition

// Forward declarations of global variables used by the Fiber-based ULT system
extern void* scheduler_fiber;
extern size_t g_current_idx;
extern std::vector<ULTContext> g_contexts;

class ThreadControl {
public:
    ThreadControl();

    // Called inside a ULT: yields control to the scheduler until resumed
    void waitUntilRunnable();

    // No-op for ULT: the scheduler will resume it explicitly
    void wake();

    // Mark this ULT as finished
    void finish();

    // Check if the current ULT has finished execution
    bool isFinished() const;
};

#endif // THREADCONTROL_H
