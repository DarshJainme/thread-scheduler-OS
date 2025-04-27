//threadscheduler.h
#include "threadedscheduler.h"
#include "ult_sync.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include <cstring>
#include <time.h>
#include <queue>
#pragma once

int getcontext(ucontext_t*) ;
int setcontext(const ucontext_t*) ;
void makecontext(ucontext_t*, void (*)(void), int, ...) ;
int swapcontext(ucontext_t*, const ucontext_t*) ;
static ULTMutex shared_mtx;
static ULTCondVar shared_cv;
static bool data_ready = false;
static int shared_counter = 0;

// ULT context structure

ucontext_t sched_ctx;
ThreadedScheduler* g_sched_ptr = nullptr;
std::vector<ULTContext> g_contexts;
size_t g_current_idx = 0;
static void task_trampoline(uintptr_t idx) {
    ULTContext& c = g_contexts[idx];
    auto& tk = g_sched_ptr->tasks[idx];
    swapcontext(&c.ctx, &sched_ctx);
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
    while (!c.finished) {
        std::cout << "[Thread " << tk->id
                  << "] is now running task " << tk->id << "\n";
        shared_mtx.lock();
        std::cout << "[Thread " << tk->id
                  << "] ENTER critical section (shared_counter = " << shared_counter << ")\n";
        shared_counter += 1;
        shared_mtx.unlock();

        struct timespec ts{0, 30 * 1000000}; // Sleep for 30ms
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
    g_contexts.assign(n, ULTContext{});

    for (size_t i = 0; i < n; ++i) {
        ULTContext& c = g_contexts[i];
        c.finished = false;

        getcontext(&c.ctx);
        c.ctx.uc_stack.ss_sp   = c.stack_mem;
        c.ctx.uc_stack.ss_size = sizeof(c.stack_mem);
        c.ctx.uc_link          = &sched_ctx;

        makecontext(&c.ctx, (void(*)())task_trampoline, 1, (uintptr_t)i);
    }
}

inline void run_ult_slice(size_t idx, int run_time) {
    g_current_idx = idx;
    swapcontext(&sched_ctx, &g_contexts[idx].ctx);
}

ThreadedScheduler::ThreadedScheduler(ThreadedAlgorithm algo, int tq, std::function<void(const std::string&)>
    log)
    : algorithm(algo), time_quantum(tq), logger(log) {
    // Hardcoded tasks
    tasks.emplace_back(std::make_unique<ThreadedTask>(1, 8, 250, 0));
    tasks.emplace_back(std::make_unique<ThreadedTask>(2, 13, 100, 100));
    tasks.emplace_back(std::make_unique<ThreadedTask>(3, 19, 300, 220));
    tasks.emplace_back(std::make_unique<ThreadedTask>(4, 21, 150, 500));
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
        case T_MLFQ:    runMLFQ();    break;
        case T_CFS:     runCFS();     break;
    }
}

void ThreadedScheduler::runFCFS() {
    log("[T-FCFS] Starting");
    int t = 0;
    // Sort tasks by arrival_time
    std::vector<size_t> order(tasks.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return tasks[a]->arrival_time < tasks[b]->arrival_time;
    });
    for (auto idx : order) {
        auto& tk = tasks[idx];
        // Wait for arrival
        if (t < tk->arrival_time) t = tk->arrival_time;
        tk->state = ThreadState::RUNNING;
        int s = t;
        int e = s + tk->remaining_time;
        _timeline.push_back({tk->id, s, e, tk->state, tk->arrival_time});
        log("[T-FCFS] Task " + std::to_string(tk->id) + ", arrival=" + std::to_string(tk->arrival_time) + ", "
            + std::to_string(s) + "->" + std::to_string(e));
        run_ult_slice(idx, tk->remaining_time);
        t = e;
        tk->state = ThreadState::FINISHED;
        g_contexts[idx].finished = true;
        swapcontext(&sched_ctx, &g_contexts[idx].ctx);
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
            if (tk->arrival_time > t || tk->remaining_time <= 0) continue;
            work_left = true;
            tk->state = ThreadState::RUNNING;
            int run = std::min(tk->remaining_time, time_quantum);
            int s = t;
            int e = s + run;
            _timeline.push_back({tk->id, s, e, tk->state, tk->arrival_time});
            log("[T-RR] Task " + std::to_string(tk->id)
                + ", arrival=" + std::to_string(tk->arrival_time)
                + ", " + std::to_string(s) + "->" + std::to_string(e));
            run_ult_slice(i, run);
            tk->remaining_time -= run;
            t = e;
            if (tk->remaining_time <= 0) {
                tk->state = ThreadState::FINISHED;
                g_contexts[i].finished = true;
                swapcontext(&sched_ctx, &g_contexts[i].ctx);
            } else {
                tk->state = ThreadState::READY;
            }
        }
    }
    log("[T-RR] Done");
}
void ThreadedScheduler::runPriority() {
    log("[T-PRIORITY] Starting");
    int t = 0;
    bool work_left = true;
    while (work_left) {
        work_left = false;
        std::sort(tasks.begin(), tasks.end(), [&](const std::unique_ptr<ThreadedTask>& a, const std::unique_ptr<ThreadedTask>& b) {
            return a->priority > b->priority;  // Adjust sorting based on priority
        });

        for (size_t i = 0; i < tasks.size(); ++i) {
            auto& tk = tasks[i];

            if (tk->arrival_time > t || tk->remaining_time <= 0) continue;

            work_left = true;
            tk->state = ThreadState::RUNNING;
            int run = std::min(tk->remaining_time, time_quantum);
            int s = t;
            int e = s + run;
            _timeline.push_back({tk->id, s, e, tk->state, tk->arrival_time});
            log("[T-PRIORITY] Task " + std::to_string(tk->id)
                + ", arrival=" + std::to_string(tk->arrival_time)
                + ", " + std::to_string(s) + "->" + std::to_string(e));
            run_ult_slice(i, run);
            tk->remaining_time -= run;
            t = e;
            if (tk->remaining_time <= 0) {
                tk->state = ThreadState::FINISHED;
                g_contexts[i].finished = true;
                swapcontext(&sched_ctx, &g_contexts[i].ctx);
            } else {
                tk->state = ThreadState::READY;
            }
        }
    }
    log("[T-PRIORITY] Done");
}

void ThreadedScheduler::runMLFQ() {
    log("[T-MLFQ] Starting");
    int t = 0;
    bool work_left = true;
    std::vector<std::queue<size_t>> queues(3);  // Priority levels (0, 1, 2) for MLFQ

    while (work_left) {
        work_left = false;
        for (size_t i = 0; i < tasks.size(); ++i) {
            auto& tk = tasks[i];
            if (tk->arrival_time > t || tk->remaining_time <= 0) continue;

            work_left = true;
            int priority = (tk->remaining_time > 100) ? 2 : (tk->remaining_time > 50) ? 1 : 0;
            queues[priority].push(i);

            tk->state = ThreadState::RUNNING;

            // Process tasks from highest priority to lowest
            for (int priority_level = 0; priority_level < 3; ++priority_level) {
                if (!queues[priority_level].empty()) {
                    size_t idx = queues[priority_level].front();
                    queues[priority_level].pop();
                    auto& task = tasks[idx];
                    int run = std::min(task->remaining_time, time_quantum);
                    int s = t;
                    int e = s + run;
                    _timeline.push_back({task->id, s, e, task->state, task->arrival_time});
                    log("[T-MLFQ] Task " + std::to_string(task->id)
                        + ", arrival=" + std::to_string(task->arrival_time)
                        + ", " + std::to_string(s) + "->" + std::to_string(e));
                    run_ult_slice(idx, run);
                    task->remaining_time -= run;
                    t = e;
                    if (task->remaining_time <= 0) {
                        task->state = ThreadState::FINISHED;
                        g_contexts[idx].finished = true;
                        swapcontext(&sched_ctx, &g_contexts[idx].ctx);
                    } else {
                        task->state = ThreadState::READY;
                    }
                    break;
                }
            }
        }
    }
    log("[T-MLFQ] Done");
}

void ThreadedScheduler::runCFS() {
    log("[T-CFS] Starting");
    int t = 0;
    bool work_left = true;
    std::priority_queue<std::pair<int, size_t>> queue;  // Min-heap based on task execution time

    while (work_left) {
        work_left = false;
        for (size_t i = 0; i < tasks.size(); ++i) {
            auto& tk = tasks[i];
            if (tk->arrival_time > t || tk->remaining_time <= 0) continue;

            work_left = true;

            queue.push({-tk->remaining_time, i});

            tk->state = ThreadState::RUNNING;
            if (!queue.empty()) {
                auto [remaining_time, idx] = queue.top();
                queue.pop();
                auto& task = tasks[idx];
                int run = std::min(task->remaining_time, time_quantum);
                int s = t;
                int e = s + run;
                _timeline.push_back({task->id, s, e, task->state, task->arrival_time});
                log("[T-CFS] Task " + std::to_string(task->id)
                    + ", arrival=" + std::to_string(task->arrival_time)
                    + ", " + std::to_string(s) + "->" + std::to_string(e));
                run_ult_slice(idx, run);
                task->remaining_time -= run;
                t = e;
                if (task->remaining_time <= 0) {
                    task->state = ThreadState::FINISHED;
                    g_contexts[idx].finished = true;
                    swapcontext(&sched_ctx, &g_contexts[idx].ctx);
                } else {
                    task->state = ThreadState::READY;
                }
            }
        }
    }
    log("[T-CFS] Done");
}