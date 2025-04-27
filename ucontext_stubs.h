// File: ucontext_stubs.h
#pragma once
#include <cstddef>

// Simulated stack descriptor
typedef struct stack_t {
    void*    ss_sp;     // stack pointer
    size_t   ss_size;   // stack size
    int      ss_flags;  // (unused)
} stack_t;

// Simulated ucontext
typedef struct ucontext_t {
    stack_t           uc_stack;  // stack info
    struct ucontext_t* uc_link;  // linked context
} ucontext_t;

// Fake context‐switching stubs
static inline int getcontext(ucontext_t*)            { return 0; }
static inline int setcontext(const ucontext_t*)      { return 0; }
static inline void makecontext(ucontext_t*, void (*)(void), int, ...) {}
static inline int swapcontext(ucontext_t*, const ucontext_t*) { return 0; }

// Single, central ULTContext
static const int ULT_STACK_SIZE = 64 * 1024;
struct ULTContext {
    ucontext_t ctx;
    char       stack_mem[ULT_STACK_SIZE];
    bool       finished;
};
