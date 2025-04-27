#pragma once
#include <windows.h>
#include <vector>
#include <deque>
#include <cstddef>

static const std::size_t ULT_STACK_SIZE = 64 * 1024;

struct ULTContext {
  LPVOID fiber;     // the fiber handle
  bool   finished;  // ULT exited?
};

extern LPVOID                scheduler_fiber;   
extern std::vector<ULTContext> g_contexts;      // all ULTs
extern std::size_t           g_current_idx;     // which ULT is running
extern std::deque<std::size_t> ready_queue;     // ready list for scheuler
