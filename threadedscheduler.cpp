#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <iostream>
#include <queue>
#include <numeric>
#include "ult_context.h"  
#include "ult_sync.h"
#include "threadedscheduler.h" 
#include <QDebug>   


LPVOID scheduler_fiber = nullptr;
std::vector<ULTContext> g_contexts;
size_t g_current_idx = 0;
std::deque<size_t> ready_queue;
ThreadedScheduler* g_sched_ptr = nullptr;

static ULTMutex shared_mtx;
static ULTCondVar shared_cv;
static bool data_ready = false;
static int shared_counter = 0;


static void __stdcall task_trampoline(void* arg) {
    size_t idx = reinterpret_cast<size_t>(arg);
    ULTContext& ctx = g_contexts[idx];
    auto& tk = g_sched_ptr->tasks[idx];

    // initial handshake: yield back so scheduler records start
    SwitchToFiber(scheduler_fiber);

    if (idx == 0) {
        shared_mtx.lock();
        data_ready = true;
        shared_cv.broadcast();
        shared_mtx.unlock();
    } else {
        shared_mtx.lock();
        while (!data_ready) shared_cv.wait(shared_mtx);
        shared_mtx.unlock();
    }

    // run until finished flag set by scheduler
    while (!ctx.finished) {
        std::cout << "[ULT " << tk->id << "] slice start" << std::endl;

        // CRITICAL SECTION
        shared_mtx.lock();
        ++shared_counter;
        std::cout << " [shared_counter=" << shared_counter << "]" << std::endl;
        shared_mtx.unlock();

        // simulate work
        Sleep(30);

        std::cout << "[ULT " << tk->id << "] slice end" << std::endl;

        // yield back to scheduler for next slice
        SwitchToFiber(scheduler_fiber);
    }

    // notify scheduler of exit
    SwitchToFiber(scheduler_fiber);
}

// Build ULT contexts (fibers)
static void setup_contexts(ThreadedScheduler* sched) {
    g_sched_ptr = sched;

    size_t n = sched->tasks.size();
    g_contexts.resize(n);

    for (size_t i = 0; i < n; ++i) {
        g_contexts[i].finished = false;
        g_contexts[i].fiber = CreateFiber(
            ULT_STACK_SIZE,
            task_trampoline,
            reinterpret_cast<void*>(i)
        );
        if (!g_contexts[i].fiber) {
            qFatal("CreateFiber failed for ULT %zu (error %u)", i, GetLastError());
        }
    }
}

// Helper to schedule one slice
inline void schedule_slice(size_t idx) {
    g_current_idx = idx;
    // switch into the ULT’s fiber
    SwitchToFiber(g_contexts[idx].fiber);
}

ThreadedScheduler::ThreadedScheduler(ThreadedAlgorithm algo,
                                     int tq,
                                     Logger log)
    : algorithm(algo), time_quantum(tq), logger(log) {

    tasks.emplace_back(std::make_unique<ThreadedTask>(1, 5, 200, 0));
    tasks.emplace_back(std::make_unique<ThreadedTask>(2, 3, 150, 50));
    tasks.emplace_back(std::make_unique<ThreadedTask>(3, 8, 300, 100));
}

void ThreadedScheduler::log(const std::string& msg) {
    if (logger) logger(msg);
    else std::cout << msg << std::endl;
}

const std::vector<ThreadedTimelineEntry>& ThreadedScheduler::timeline() const {
    return _timeline;
}

void ThreadedScheduler::run() {
    // 1) build fiber contexts
    setup_contexts(this);

    // 2) invoke the chosen policy
    switch (algorithm) {
        case T_FCFS:     runFCFS();     break;
        case T_RR:       runRR();       break;
        case T_PRIORITY: runPriority(); break;
        case T_MLFQ:     runMLFQ();     break;
        case T_CFS:      runCFS();      break;
    }

    // 3) Clean up worker fibers so repeated runs work
    for (auto &ctx : g_contexts) {
        if (ctx.fiber) {
            DeleteFiber(ctx.fiber);
            ctx.fiber = nullptr;
        }
    }
    g_contexts.clear();
}

void ThreadedScheduler::runFCFS() {
    log("[FCFS] starting");
    
    // initial dispatch into first ULT
    schedule_slice(0);

    // sort by arrival
    std::vector<size_t> order(tasks.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return tasks[a]->arrival_time < tasks[b]->arrival_time;
    });

    int current_time = 0;
    for (auto idx : order) {
        auto& tk = tasks[idx];
        // wait until arrival
        current_time = std::max(current_time, tk->arrival_time);
        tk->state = ThreadState::RUNNING;
        int slice = tk->remaining_time;
        // record timeline
        _timeline.emplace_back(tk->id, current_time, current_time + slice, tk->state, tk->arrival_time);

        // schedule one long slice
        schedule_slice(idx);

        // mark finished
        current_time += slice;
        tk->remaining_time = 0;
        tk->state = ThreadState::FINISHED;
        g_contexts[idx].finished = true;
    }
    log("[FCFS] done");
}

void ThreadedScheduler::runRR() {
    log("[RR] starting");
    // initial dispatch into first ULT
    schedule_slice(0);
    int current_time = 0;
    std::queue<size_t> q;
    // initially enqueue arrived tasks
    for (size_t i = 0; i < tasks.size(); ++i)
        if (tasks[i]->arrival_time == 0) q.push(i);

    // simple RR until all done
    int remaining = tasks.size();
    while (remaining > 0) {
        if (q.empty()) { current_time++; continue; }
        size_t idx = q.front(); q.pop();
        auto& tk = tasks[idx];
        if (tk->remaining_time <= 0) continue;

        // run one quantum or until finish
        tk->state = ThreadState::RUNNING;
        int run = std::min(tk->remaining_time, time_quantum);
        _timeline.emplace_back(tk->id, current_time, current_time + run, tk->state, tk->arrival_time);
        schedule_slice(idx);

        tk->remaining_time -= run;
        current_time += run;

        if (tk->remaining_time > 0) {
            tk->state = ThreadState::READY;
            q.push(idx);
        } else {
            tk->state = ThreadState::FINISHED;
            g_contexts[idx].finished = true;
            --remaining;
        }
        // enqueue newly arrived tasks
        for (size_t j = 0; j < tasks.size(); ++j)
            if (tasks[j]->arrival_time > current_time - run && tasks[j]->arrival_time <= current_time)
                q.push(j);
    }
    log("[RR] done");
}

void ThreadedScheduler::runPriority() {
    log("[PRIORITY] starting");
    // initial dispatch into first ULT
    schedule_slice(0);

    int current_time = 0;
    const int AGING_INTERVAL = time_quantum; // age tasks every quantum
    const int AGING_INCREMENT = 1;         // increase priority by 1 each interval

    // copy of original priorities to avoid unbounded growth
    std::vector<int> base_prio(tasks.size());
    for (size_t i = 0; i < tasks.size(); ++i) {
        base_prio[i] = tasks[i]->priority;
    }

    // track waiting time
    std::vector<int> wait_time(tasks.size(), 0);
    bool any;
    do {
        any = false;
        // age waiting tasks
        for (size_t i = 0; i < tasks.size(); ++i) {
            auto& tk = tasks[i];
            if (tk->arrival_time <= current_time && tk->remaining_time > 0) {
                // increment wait time
                wait_time[i] += AGING_INTERVAL;
                // if waited at least one interval, boost priority
                if (wait_time[i] >= AGING_INTERVAL) {
                    tasks[i]->priority += AGING_INCREMENT;
                    wait_time[i] = 0;
                }
            }
        }

        // pick highest-priority ready task
        size_t best_idx = 0;
        int best_prio = -1;
        for (size_t i = 0; i < tasks.size(); ++i) {
            auto& tk = tasks[i];
            if (tk->arrival_time <= current_time && tk->remaining_time > 0) {
                any = true;
                if (tk->priority > best_prio) {
                    best_prio = tk->priority;
                    best_idx = i;
                }
            }
        }
        if (!any) {
            current_time++;
            continue;
        }

        auto& tk = tasks[best_idx];
        tk->state = ThreadState::RUNNING;
        int run = std::min(tk->remaining_time, time_quantum);
        _timeline.emplace_back(tk->id, current_time, current_time + run, tk->state, tk->arrival_time);
        schedule_slice(best_idx);

        tk->remaining_time -= run;
        current_time += run;
        if (tk->remaining_time <= 0) {
            tk->state = ThreadState::FINISHED;
            g_contexts[best_idx].finished = true;
            // restore priority (optional)
            tasks[best_idx]->priority = base_prio[best_idx];
        } else {
            tk->state = ThreadState::READY;
        }
    } while (any);

    log("[PRIORITY] done");
}

void ThreadedScheduler::runMLFQ() {
    log("[MLFQ] starting");
        // initial dispatch into first ULT
        schedule_slice(0);
    int current_time = 0;
    // Three levels: 0 (high) to 2 (low)
    std::vector<std::queue<size_t>> queues(3);

    // initially enqueue arrivals at time 0 to queue 0
    for (size_t i = 0; i < tasks.size(); ++i)
        if (tasks[i]->arrival_time == 0) queues[0].push(i);

    int remaining = tasks.size();
    while (remaining > 0) {
        // find next non-empty queue
        int level = -1;
        for (int l = 0; l < 3; ++l) {
            if (!queues[l].empty()) { level = l; break; }
        }
        if (level < 0) { current_time++; // check new arrivals
            for (size_t i = 0; i < tasks.size(); ++i)
                if (tasks[i]->arrival_time == current_time) queues[0].push(i);
            continue;
        }
        size_t idx = queues[level].front(); queues[level].pop();
        auto& tk = tasks[idx];
        tk->state = ThreadState::RUNNING;
        int run = std::min(tk->remaining_time, time_quantum << level);
        _timeline.emplace_back(tk->id, current_time, current_time + run, tk->state, tk->arrival_time);
        schedule_slice(idx);
        tk->remaining_time -= run;
        current_time += run;
        // enqueue new arrivals
        for (size_t i = 0; i < tasks.size(); ++i)
            if (tasks[i]->arrival_time > current_time - run && tasks[i]->arrival_time <= current_time)
                queues[0].push(i);
        if (tk->remaining_time <= 0) {
            tk->state = ThreadState::FINISHED;
            g_contexts[idx].finished = true;
            --remaining;
        } else {
            tk->state = ThreadState::READY;
            // demote to lower queue unless already lowest
            int next_level = std::min(level + 1, 2);
            queues[next_level].push(idx);
        }
    }
    log("[MLFQ] done");
}

void ThreadedScheduler::runCFS() {
    log("[CFS] starting");

    //Initialize vruntime & weights, and initial dispatch
    const double DEFAULT_WEIGHT = 1024.0;
    for (auto &tk_ptr : tasks) {
        tk_ptr->weight   = DEFAULT_WEIGHT / (1 << tk_ptr->nice);
        tk_ptr->vruntime = 0.0;
        tk_ptr->state    = ThreadState::NEW;
    }
    schedule_slice(0);

    int current_time = 0;
    int remaining    = static_cast<int>(tasks.size());

    auto cmp = [&](size_t a, size_t b) {
        return tasks[a]->vruntime > tasks[b]->vruntime;
    };
    std::priority_queue<size_t, std::vector<size_t>, decltype(cmp)> run_queue(cmp);

    while (remaining > 0) {
        for (size_t i = 0; i < tasks.size(); ++i) {
            auto &tk = tasks[i];
            if (tk->arrival_time <= current_time
             && tk->remaining_time > 0
             && tk->state == ThreadState::NEW)
            {
                tk->state = ThreadState::READY;
                run_queue.push(i);
            }
        }

        if (run_queue.empty()) {
            ++current_time;
            continue;
        }

        size_t idx = run_queue.top();
        run_queue.pop();
        auto &tk = tasks[idx];

        tk->state = ThreadState::RUNNING;
        int slice = std::min(tk->remaining_time, time_quantum);

        _timeline.emplace_back(
            tk->id,
            current_time,
            current_time + slice,
            tk->state,
            tk->arrival_time
        );

        schedule_slice(idx);

        tk->remaining_time -= slice;
        current_time       += slice;

        double vdelta = slice * (DEFAULT_WEIGHT / tk->weight);
        tk->vruntime += vdelta;

        if (tk->remaining_time <= 0) {
            tk->state               = ThreadState::FINISHED;
            g_contexts[idx].finished = true;
            --remaining;
        } else {
            tk->state = ThreadState::READY;
            run_queue.push(idx);
        }
    }

    log("[CFS] done");
}
