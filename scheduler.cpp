// File: scheduler.cpp — updated initializer and includes
#include "scheduler.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <set>
#include <deque>
#include <vector>
#include <unordered_map>
#include <queue>
#include <functional>
#include <string>

using namespace std;

Scheduler::Scheduler(Algorithm algo, int tq, function<void(const string&)> lg)
    : algorithm(algo), time_quantum(tq), logger(lg)
{
    // tasks[i] = {id, priority, remaining_time, arrival_time, deadline, level}
    tasks = {
        {1, 8,   250, 0,   300, 0},
        {2, 13,  100, 100, 350, 0},
        {3, 19,  300, 220, 600, 0},
        {4, 21,  150, 500, 700, 0}
    };
}

void Scheduler::log(const string &msg) {
    if (logger) logger(msg);
    else cout << msg << "\n";
}

const vector<TimelineEntry>& Scheduler::timeline() const {
    return _timeline;
}

void Scheduler::run(){
    switch (algorithm) {
        case FCFS:    runFCFS();    break;
        case RR:      runRR();      break;
        case PRIORITY:runPriority();break;
        case SJF:     runSJF();     break;
        case MLQ:     runMLQ();     break;
        case MLFQ:    runMLFQ();    break;
        case EDF:     runEDF();     break;
        case CFS:     runCFS();     break;
    }
}

// FCFS
void Scheduler::runFCFS(){
    log("[FCFS] Starting");
    int t = 0;
    for(auto &tk : tasks){
        // Don’t start before the task actually arrives
        int s = std::max(t, tk.arrival_time);
        int e = s + tk.remaining_time;

        _timeline.push_back({tk.id, s, e});
        log("[FCFS] T" + std::to_string(tk.id) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        // Simulate execution
        std::this_thread::sleep_for(std::chrono::milliseconds(tk.remaining_time/10));

        // Advance your clock to end of this slice
        t = e;
    }
    log("[FCFS] Done");
}

void Scheduler::runRR() {
    log("[RR] Starting");

    int t = 0;                              // current time
    std::deque<Task*> rq;                   // ready queue
    size_t next = 0;                        // index of next task to arrive

    // Prime the queue with all tasks arriving at t=0
    while (next < tasks.size() && tasks[next].arrival_time <= t) {
        rq.push_back(&tasks[next++]);
    }

    // Main loop: while there’s work pending or tasks yet to arrive
    while (!rq.empty() || next < tasks.size()) {
        if (rq.empty()) {
            // No ready tasks → jump time to next arrival
            t = tasks[next].arrival_time;
            rq.push_back(&tasks[next++]);
        }

        Task* tk = rq.front(); rq.pop_front();
        // Ensure we don't start before the task actually arrived
        int s = std::max(t, tk->arrival_time);
        int run = std::min(tk->remaining_time, time_quantum);
        int e = s + run;

        // Record the slice
        _timeline.push_back({ tk->id, s, e });
        log("[RR] T" + std::to_string(tk->id) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        // Simulate execution
        std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));

        // Advance time & decrement remaining
        t = e;
        tk->remaining_time -= run;

        // Enqueue any newly arrived tasks up through time t
        while (next < tasks.size() && tasks[next].arrival_time <= t) {
            rq.push_back(&tasks[next++]);
        }

        // If this task still has work, requeue it
        if (tk->remaining_time > 0) {
            rq.push_back(tk);
        }
    }

    log("[RR] Done");
}

// Updated Priority Scheduling (with feedback + aging) to account for task arrival times
// Pre-requisite: Task struct has int arrival_time; and tasks[] is sorted by arrival_time

void Scheduler::runPriority() {
    log("[PR] Starting with feedback+aging");
    int t = 0;

    // Comparator: higher priority first, tie-break on lower id
    auto pr_cmp = [](Task* a, Task* b) {
        if (a->priority != b->priority)
            return a->priority > b->priority;
        return a->id < b->id;
    };
    std::set<Task*, decltype(pr_cmp)> rq(pr_cmp);

    size_t next = 0;  // index for next arrival in tasks
    // Enqueue all tasks arriving at t=0
    while (next < tasks.size() && tasks[next].arrival_time <= t) {
        rq.insert(&tasks[next++]);
    }

    const int FF = 50;  // feedback factor
    const int AG = 1;   // aging increment

    while (!rq.empty() || next < tasks.size()) {
        if (rq.empty()) {
            // Idle until next task arrives
            t = tasks[next].arrival_time;
            rq.insert(&tasks[next++]);
        }

        // Pick highest-priority task
        auto it = rq.begin();
        Task* tk = *it;
        rq.erase(it);

        // Ensure we respect arrival
        int s = std::max(t, tk->arrival_time);
        int run = std::min(tk->remaining_time, time_quantum);
        int e = s + run;

        // Log and record
        _timeline.push_back({tk->id, s, e});
        log("[PR ] T" + std::to_string(tk->id) +
            " pr=" + std::to_string(tk->priority) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));

        // Advance time and update remaining
        t = e;
        tk->remaining_time -= run;

        // Decrease priority (feedback)
        int dec = run / FF;
        tk->priority = std::max(1, tk->priority - dec);
        // Age other ready tasks
        for (auto x : rq) {
            x->priority += AG;
        }

        // Enqueue any newly arrived tasks up through time t
        while (next < tasks.size() && tasks[next].arrival_time <= t) {
            rq.insert(&tasks[next++]);
        }

        // If task still has work, re-insert it
        if (tk->remaining_time > 0) {
            rq.insert(tk);
        }
    }

    log("[PR] Done");
}

// Updated SJF scheduling in scheduler.cpp to account for task arrival times
// Pre-requisite: extend Task struct in scheduler.h to include:
//    int arrival_time;

void Scheduler::runSJF() {
    log("[SJF] Starting");
    int t = 0;
    size_t next = 0;
    std::vector<Task*> rq;
    rq.reserve(tasks.size());

    // Enqueue tasks that arrive at time 0
    while (next < tasks.size() && tasks[next].arrival_time <= t) {
        rq.push_back(&tasks[next++]);
    }

    // Process until all tasks are handled
    while (!rq.empty() || next < tasks.size()) {
        if (rq.empty()) {
            // Idle until next arrival
            t = tasks[next].arrival_time;
            rq.push_back(&tasks[next++]);
        }

        // Find task with smallest remaining time
        auto it = std::min_element(rq.begin(), rq.end(), [](Task* a, Task* b) {
            if (a->remaining_time != b->remaining_time)
                return a->remaining_time < b->remaining_time;
            return a->id < b->id; // tie-break by id
        });
        Task* tk = *it;
        rq.erase(it);

        // Determine start and end, respecting arrival
        int s = std::max(t, tk->arrival_time);
        int e = s + tk->remaining_time;

        // Log and record
        _timeline.push_back({ tk->id, s, e });
        log("[SJF] T" + std::to_string(tk->id) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        // Simulate execution
        std::this_thread::sleep_for(std::chrono::milliseconds(tk->remaining_time / 10));

        // Advance time
        t = e;

        // Add newly arrived tasks up through current time
        while (next < tasks.size() && tasks[next].arrival_time <= t) {
            rq.push_back(&tasks[next++]);
        }
    }

    log("[SJF] Done");
}

// Updated MLQ scheduling in scheduler.cpp to account for arrival times and three priority levels:
//  1-10: low, 11-20: medium, >20: high
// Pre-requisite: Task struct has int arrival_time; and tasks are sorted by arrival_time

void Scheduler::runMLQ() {
    log("[MLQ] Starting (3-level queues)");
    int t = 0;
    size_t next = 0;
    std::deque<Task*> lowQ, medQ, highQ;

    // Enqueue tasks arriving at t=0
    while (next < tasks.size() && tasks[next].arrival_time <= t) {
        Task* tk = &tasks[next++];
        if (tk->priority > 20)       highQ.push_back(tk);
        else if (tk->priority > 10)  medQ.push_back(tk);
        else                          lowQ.push_back(tk);
    }

    // Main loop: run until all tasks processed
    while (!highQ.empty() || !medQ.empty() || !lowQ.empty() || next < tasks.size()) {
        // If no ready tasks, fast-forward time to next arrival
        if (highQ.empty() && medQ.empty() && lowQ.empty()) {
            t = tasks[next].arrival_time;
            Task* tk = &tasks[next++];
            if (tk->priority > 20)       highQ.push_back(tk);
            else if (tk->priority > 10)  medQ.push_back(tk);
            else                          lowQ.push_back(tk);
        }

        // Select next task: high > medium > low
        Task* tk;
        if (!highQ.empty()) {
            tk = highQ.front(); highQ.pop_front();
        } else if (!medQ.empty()) {
            tk = medQ.front(); medQ.pop_front();
        } else {
            tk = lowQ.front(); lowQ.pop_front();
        }

        // Compute start/end times, ensuring not before arrival
        int s = std::max(t, tk->arrival_time);
        int e = s + tk->remaining_time;

        // Record and log
        _timeline.push_back({tk->id, s, e});
        log("[MLQ] T" + std::to_string(tk->id) +
            " pr=" + std::to_string(tk->priority) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(tk->remaining_time / 10));

        // Advance current time
        t = e;

        // Enqueue newly arrived tasks up to time t
        while (next < tasks.size() && tasks[next].arrival_time <= t) {
            Task* nt = &tasks[next++];
            if (nt->priority > 20)       highQ.push_back(nt);
            else if (nt->priority > 10)  medQ.push_back(nt);
            else                          lowQ.push_back(nt);
        }
    }

    log("[MLQ] Done");
}

// Updated MLFQ scheduling in scheduler.cpp to account for arrival times
// Uses 3 levels (like MLQ): level 0 (q0), level 1 (q1), level 2 (q2)
// Pre-requisite: Task struct has int arrival_time; tasks sorted by arrival_time

void Scheduler::runMLFQ() {
    log("[MLFQ] Starting (3-level MLFQ)");
    int t = 0;                           // current time
    size_t next = 0;                    // index for next task arrival
    const int levels = 3;
    std::vector<std::deque<Task*>> queues(levels);

    // Enqueue tasks arriving at time 0 into level 0
    while (next < tasks.size() && tasks[next].arrival_time <= t) {
        queues[0].push_back(&tasks[next++]);
    }

    // Process until all tasks are done
    while (next < tasks.size() ||                 
           !queues[0].empty() || !queues[1].empty() || !queues[2].empty()) {
        // Select highest-priority non-empty queue
        int lvl = -1;
        for (int i = 0; i < levels; ++i) {
            if (!queues[i].empty()) { lvl = i; break; }
        }
        // If no ready tasks, fast-forward to next arrival
        if (lvl < 0) {
            t = tasks[next].arrival_time;
            queues[0].push_back(&tasks[next++]);
            lvl = 0;
        }

        // Dequeue next task
        Task* tk = queues[lvl].front();
        queues[lvl].pop_front();

        // Determine quantum: time_quantum * (2^lvl)
        int quantum = time_quantum * (1 << lvl);
        int s = std::max(t, tk->arrival_time);
        int run = std::min(tk->remaining_time, quantum);
        int e = s + run;

        // Log and record
        _timeline.push_back({tk->id, s, e});
        log("[MLFQ] T" + std::to_string(tk->id) +
            " L" + std::to_string(lvl) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        // Simulate execution
        std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));

        // Advance time and decrement remaining
        t = e;
        tk->remaining_time -= run;

        // Enqueue tasks that arrived up to time t into level 0
        while (next < tasks.size() && tasks[next].arrival_time <= t) {
            queues[0].push_back(&tasks[next++]);
        }

        // If unfinished, demote to next level (max level 2)
        if (tk->remaining_time > 0) {
            int newLvl = std::min(lvl + 1, levels - 1);
            queues[newLvl].push_back(tk);
        }
    }

    log("[MLFQ] Done");
}

// Updated EDF scheduling in scheduler.cpp to account for arrival times
// Pre-requisite: Task struct includes int arrival_time; tasks sorted by arrival_time

void Scheduler::runEDF() {
    log("[EDF] Starting");
    int t = 0;
    size_t next = 0;

    auto edf_cmp = [](Task* a, Task* b) {
        if (a->deadline != b->deadline)
            return a->deadline < b->deadline;
        return a->id < b->id;
    };
    std::multiset<Task*, decltype(edf_cmp)> rq(edf_cmp);

    // Enqueue tasks that arrive at time 0
    while (next < tasks.size() && tasks[next].arrival_time <= t) {
        rq.insert(&tasks[next++]);
    }

    // Main loop: until all tasks are processed
    while (!rq.empty() || next < tasks.size()) {
        if (rq.empty()) {
            // Jump time to next arrival if idle
            t = tasks[next].arrival_time;
            rq.insert(&tasks[next++]);
        }

        // Select task with earliest deadline
        Task* tk = *rq.begin();
        rq.erase(rq.begin());

        // Compute start time (respect arrival)
        int s = std::max(t, tk->arrival_time);
        int run = std::min(tk->remaining_time, time_quantum);
        int e = s + run;

        // Record and log
        _timeline.push_back({tk->id, s, e});
        log("[EDF] T" + std::to_string(tk->id) +
            " dl=" + std::to_string(tk->deadline) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        // Simulate execution
        std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));

        // Advance time and update remaining
        t = e;
        tk->remaining_time -= run;

        // Enqueue newly arrived tasks up to current time
        while (next < tasks.size() && tasks[next].arrival_time <= t) {
            rq.insert(&tasks[next++]);
        }

        // Requeue if not finished
        if (tk->remaining_time > 0) {
            rq.insert(tk);
        }
    }

    log("[EDF] Done");
}

// CFS
void Scheduler::runCFS() {
    log("[CFS] Starting (with arrival times)");
    int t = 0;
    size_t next = 0;
    size_t n = tasks.size();

    // Prepare a list of task pointers sorted by arrival
    std::vector<Task*> upcoming;
    upcoming.reserve(n);
    for (auto &tk : tasks) upcoming.push_back(&tk);
    // tasks already sorted by arrival_time, but ensure anyway
    std::sort(upcoming.begin(), upcoming.end(), [](Task* a, Task* b){
        return a->arrival_time < b->arrival_time;
    });

    // Map to hold each task's accumulated vruntime
    std::unordered_map<Task*, double> vruntime;

    // Comparator: pick smallest vruntime, tie-break on id
    auto cmp = [&](Task* a, Task* b) {
        double va = vruntime[a], vb = vruntime[b];
        if (va != vb) return va < vb;
        return a->id < b->id;
    };
    std::multiset<Task*, decltype(cmp)> rq(cmp);

    // Enqueue tasks that arrive at time 0
    while (next < n && upcoming[next]->arrival_time <= t) {
        Task* p = upcoming[next++];
        vruntime[p] = 0.0;
        rq.insert(p);
    }

    // Main loop: until all tasks processed
    while (next < n || !rq.empty()) {
        if (rq.empty()) {
            // No ready tasks: jump to next arrival
            t = upcoming[next]->arrival_time;
            while (next < n && upcoming[next]->arrival_time <= t) {
                Task* p = upcoming[next++];
                vruntime[p] = 0.0;
                rq.insert(p);
            }
        }

        // Dequeue the task with minimum vruntime
        auto it = rq.begin();
        Task* tk = *it;
        rq.erase(it);

        // Calculate slice and times
        int slice = std::min(tk->remaining_time, time_quantum);
        int s = std::max(t, tk->arrival_time);
        int e = s + slice;

        // Record and log execution
        _timeline.push_back({tk->id, s, e});
        log("[CFS] T" + std::to_string(tk->id) +
            " vruntime=" + std::to_string(vruntime[tk]) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(slice / 10));

        // Advance time and update task state
        t = e;
        tk->remaining_time -= slice;
        vruntime[tk] += double(slice) / tk->priority;

        // Add any newly arrived tasks up to time t
        while (next < n && upcoming[next]->arrival_time <= t) {
            Task* p = upcoming[next++];
            vruntime[p] = 0.0;
            rq.insert(p);
        }

        // Reinsert unfinished task
        if (tk->remaining_time > 0) {
            rq.insert(tk);
        }
    }

    log("[CFS] Done");
}
