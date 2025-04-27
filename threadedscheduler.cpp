// File: threadedscheduler.cpp — ULT-based scheduler implementation with mutex & condvar
#include "threadedscheduler.h"
#include "ult_sync.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <ucontext.h>
#include <vector>
#include <cstring>
#include <time.h>

// Constants
static const int STACK_SIZE = 64 * 1024;

// User-level synchronization primitives
static ULTMutex shared_mtx;
static ULTCondVar shared_cv;
static bool data_ready = false;

// Simulated shared resource
static int shared_counter = 0;

// ULT context structure
struct ULTContext {
    ucontext_t ctx;
    char stack[STACK_SIZE];
    bool finished;
};

// Scheduler globals
ucontext_t sched_ctx;
ThreadedScheduler* g_sched_ptr = nullptr;
std::vector<ULTContext> g_contexts;
size_t g_current_idx = 0;

// Trampoline function for each ULT
template<typename Func>
static void task_trampoline(uintptr_t idx) {
    ULTContext& c = g_contexts[idx];
    auto& tk = g_sched_ptr->tasks[idx];
    // Initial yield to scheduler
    swapcontext(&c.ctx, &sched_ctx);

    // Demonstrate condvar: only thread 0 signals, others wait
    if (idx == 0) {
        shared_mtx.lock();
        data_ready = true;
        shared_cv.broadcast();
        shared_mtx.unlock();
    } else {
        shared_mtx.lock();
        while (!data_ready) {
            shared_cv.wait(shared_mtx);
        }
        shared_mtx.unlock();
    }

    // Main execution loop
    while (!c.finished) {
        std::cout << "[Thread " << tk->id
                  << "] is now running task " << tk->id << "\n";

        // Critical section protected by ULTMutex
        shared_mtx.lock();
        std::cout << "[Thread " << tk->id
                  << "] ENTER critical section (shared_counter = " << shared_counter << ")\n";
        shared_counter += 1;
        shared_mtx.unlock();

        struct timespec ts{0, 30 * 1000000};
        nanosleep(&ts, nullptr);

        std::cout << "[Thread " << tk->id
                  << "] EXIT critical section (shared_counter = " << shared_counter << ")\n";
        std::cout << "[Thread " << tk->id
                  << "] finished this slice\n";

        // Yield back to scheduler
        swapcontext(&c.ctx, &sched_ctx);
    }

    std::cout << "[Thread " << tk->id
              << "] exiting task " << tk->id << "\n";
    swapcontext(&c.ctx, &sched_ctx);
}

// Setup ULT contexts for all tasks
static void setup_contexts(ThreadedScheduler* sched) {
    g_sched_ptr = sched;
    size_t n = sched->tasks.size();
    g_contexts.clear();
    g_contexts.resize(n);
    for (size_t i = 0; i < n; ++i) {
        ULTContext& c = g_contexts[i];
        c.finished = false;
        getcontext(&c.ctx);
        c.ctx.uc_stack.ss_sp = c.stack;
        c.ctx.uc_stack.ss_size = sizeof(c.stack);
        c.ctx.uc_link = &sched_ctx;
        makecontext(&c.ctx, (void(*)())task_trampoline<void>, 1, (uintptr_t)i);
    }
}

// Run one ULT slice
inline void run_ult_slice(size_t idx, int /*run*/) {
    g_current_idx = idx;
    swapcontext(&sched_ctx, &g_contexts[idx].ctx);
}

// Constructor
ThreadedScheduler::ThreadedScheduler(ThreadedAlgorithm algo, int tq, std::function<void(const std::string&)> log)
    : algorithm(algo), time_quantum(tq), logger(log) {
    tasks.emplace_back(std::make_unique<ThreadedTask>(1, 15, 250));
    tasks.emplace_back(std::make_unique<ThreadedTask>(2, 5, 100));
    tasks.emplace_back(std::make_unique<ThreadedTask>(3, 20, 300));
    tasks.emplace_back(std::make_unique<ThreadedTask>(4, 10, 150));
}

void ThreadedScheduler::log(const std::string& msg) {
    if (logger) logger(msg);
    else std::cout << msg << "\n";
}

const std::vector<ThreadedTimelineEntry>& ThreadedScheduler::timeline() const {
    return _timeline;
}

void ThreadedScheduler::run() {
    setup_contexts(this);
    getcontext(&sched_ctx);
    switch (algorithm) {
        case T_FCFS:    runFCFS();    break;
        case T_RR:      runRR();      break;
        case T_PRIORITY:runPriority();break;
    }
}

void ThreadedScheduler::runFCFS() {
    log("[T-FCFS] Starting");
    int t = 0;
    for (size_t i = 0; i < tasks.size(); ++i) {
        auto& tk = tasks[i];
        int s = t;
        int e = s + tk->remaining_time;
        _timeline.push_back({tk->id, s, e});
        log("[T-FCFS] Task " + std::to_string(tk->id) + " " + std::to_string(s) + "->" + std::to_string(e));
        run_ult_slice(i, tk->remaining_time);
        t = e;
        g_contexts[i].finished = true;
        swapcontext(&sched_ctx, &g_contexts[i].ctx);
    }
    log("[T-FCFS] Done");
}

void ThreadedScheduler::runRR() {
    log("[T-RR] Starting");
    int t = 0;
    bool work_left = true;
    while (work_left) {
        work_left = false;
        for (size_t i = 0; i < tasks.size(); ++i) {
            auto& tk = tasks[i];
            if (tk->remaining_time > 0) {
                work_left = true;
                int run = std::min(tk->remaining_time, time_quantum);
                int s = t;
                int e = s + run;
                _timeline.push_back({tk->id, s, e});
                log("[T-RR] Task " + std::to_string(tk->id) + " " + std::to_string(s) + "->" + std::to_string(e));
                run_ult_slice(i, run);
                tk->remaining_time -= run;
                t = e;
                if (tk->remaining_time <= 0) {
                    g_contexts[i].finished = true;
                    swapcontext(&sched_ctx, &g_contexts[i].ctx);
                }
            }
        }
    }
    log("[T-RR] Done");
}

void ThreadedScheduler::runPriority() {
    log("[T-PRIORITY] Starting");
    int t = 0;
    const int FF = 50, AG = 1;
    size_t done = 0;
    while (done < tasks.size()) {
        std::vector<size_t> order;
        for (size_t i = 0; i < tasks.size(); ++i)
            if (!g_contexts[i].finished) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
            return tasks[a]->priority > tasks[b]->priority;
        });
        size_t idx = order.front();
        auto& tk = tasks[idx];
        int run = std::min(tk->remaining_time, time_quantum);
        int s = t;
        int e = s + run;
        _timeline.push_back({tk->id, s, e});
        log("[T-PRIORITY] Task " + std::to_string(tk->id) + " pr=" + std::to_string(tk->priority));
        run_ult_slice(idx, run);
        tk->remaining_time -= run;
        t = e;
        int dec = run / FF;
        tk->priority = std::max(1, tk->priority - dec);
        for (auto j : order) if (j != idx) tasks[j]->priority += AG;
        if (tk->remaining_time <= 0) {
            g_contexts[idx].finished = true;
            swapcontext(&sched_ctx, &g_contexts[idx].ctx);
            ++done;
        }
    }
    log("[T-PRIORITY] Done");
}
