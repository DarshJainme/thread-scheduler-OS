// File: threadcontrol.cpp — user‐level thread control for ULT scheduler
#include "threadcontrol.h"
#include <ucontext.h>
#include <vector>
#include <cstddef>

// Externs from the ULT scheduler:
extern ucontext_t sched_ctx;
extern size_t g_current_idx;
struct ULTContext { ucontext_t ctx; bool finished; };
extern std::vector<ULTContext> g_contexts;

ThreadControl::ThreadControl() {
    // nothing to initialize for ULT
}

void ThreadControl::waitUntilRunnable() {
    // Park this ULT: save its context and switch back to scheduler
    swapcontext(&g_contexts[g_current_idx].ctx, &sched_ctx);
}

void ThreadControl::wake() {
    // No operation: scheduler resumes the ULT by swapping contexts
}

void ThreadControl::finish() {
    // Mark this ULT as finished so its trampoline can exit
    g_contexts[g_current_idx].finished = true;
}

bool ThreadControl::isFinished() const {
    return g_contexts[g_current_idx].finished;
}