
#include "threadcontrol.h"
#include <windows.h>
#include <vector>
#include <cstddef>
#include "ult_context.h"
extern void* scheduler_fiber;
extern size_t g_current_idx;
extern std::vector<ULTContext> g_contexts;

void ThreadControl::waitUntilRunnable() {
    SwitchToFiber(scheduler_fiber);
}

void ThreadControl::finish() {
    g_contexts[g_current_idx].finished = true;
}

bool ThreadControl::isFinished() const {
    return g_contexts[g_current_idx].finished;
}