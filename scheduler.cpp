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

Scheduler::Scheduler(Algorithm algo, int tq, function<void(const string &)> lg)
    : algorithm(algo), time_quantum(tq), logger(lg)
{
    // tasks[i] = {id, priority, remaining_time, arrival_time, deadline, level}
    tasks = {
        {1, 8, 250, 0, 300, 0},
        {2, 13, 100, 100, 350, 0},
        {3, 19, 300, 400, 600, 0},
        {4, 21, 150, 500, 700, 0}};
}

void Scheduler::log(const string &msg)
{
    if (logger)
        logger(msg);
    else
        cout << msg << "\n";
}

const vector<TimelineEntry> &Scheduler::timeline() const
{
    return _timeline;
}

void Scheduler::run()
{
    switch (algorithm)
    {
    case FCFS:
        runFCFS();
        break;
    case RR:
        runRR();
        break;
    case PRIORITY:
        runPriority();
        break;
    case SJF:
        runSJF();
        break;
    case MLQ:
        runMLQ();
        break;
    case MLFQ:
        runMLFQ();
        break;
    case EDF:
        runEDF();
        break;
    case CFS:
        runCFS();
        break;
    }
}

void Scheduler::runFCFS()
{
    log("[FCFS] Starting");
    int t = 0;
    for (auto &tk : tasks)
    {
        int s = std::max(t, tk.arrival_time);
        int e = s + tk.remaining_time;

        _timeline.push_back({tk.id, s, e});
        log("[FCFS] T" + std::to_string(tk.id) + " " + std::to_string(s) + "->" + std::to_string(e));

        std::this_thread::sleep_for(std::chrono::milliseconds(tk.remaining_time / 10));
        t = e;
    }
    log("[FCFS] Done");
}

void Scheduler::runRR()
{
    log("[RR] Starting");

    int t = 0;             // current time
    std::deque<Task *> rq; // ready queue
    size_t next = 0;       // index of next task to arrive

    // we initialize the ready queue with tasks arriving at time 0
    while (next < tasks.size() && tasks[next].arrival_time <= t)
    {
        rq.push_back(&tasks[next++]);
    }

    while (!rq.empty() || next < tasks.size())
    {
        if (rq.empty())
        {
            t = tasks[next].arrival_time;
            rq.push_back(&tasks[next++]);
        }

        Task *tk = rq.front();
        rq.pop_front();
        int s = std::max(t, tk->arrival_time);
        int run = std::min(tk->remaining_time, time_quantum);
        int e = s + run;

        _timeline.push_back({tk->id, s, e});
        log("[RR] T" + std::to_string(tk->id) + " " + std::to_string(s) + "->" + std::to_string(e));

        std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));

        t = e;
        tk->remaining_time -= run;

        while (next < tasks.size() && tasks[next].arrival_time <= t)
        {
            rq.push_back(&tasks[next++]);
        }

        if (tk->remaining_time > 0)
        {
            rq.push_back(tk);
        }
    }

    log("[RR] Done");
}

void Scheduler::runPriority()
{
    // tasks are already sorted by arrival_time
    log("[PR] Starting with feedback+aging");
    int t = 0;

    // higher number = higher priority
    auto pr_cmp = [](Task *a, Task *b)
    {
        if (a->priority != b->priority)
            return a->priority > b->priority;
        return a->id < b->id;
    };

    // set to hold tasks in order of priority
    std::set<Task *, decltype(pr_cmp)> rq(pr_cmp);

    size_t next = 0;

    while (next < tasks.size() && tasks[next].arrival_time <= t)
    {
        rq.insert(&tasks[next++]);
    }

    const int FF = 50; // feedback factor
    const int AG = 1;  // aging increment

    while (!rq.empty() || next < tasks.size())
    {
        if (rq.empty())
        {
            t = tasks[next].arrival_time;
            rq.insert(&tasks[next++]);
        }

        // highest-priority task will be at the front of the set
        auto it = rq.begin();
        Task *tk = *it;
        rq.erase(it);

        int s = std::max(t, tk->arrival_time);
        int run = std::min(tk->remaining_time, time_quantum);
        int e = s + run;

        // record in the timeline
        _timeline.push_back({tk->id, s, e});
        log("[PR ] T" + std::to_string(tk->id) +
            " pr=" + std::to_string(tk->priority) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));

        t = e;
        tk->remaining_time -= run;

        // decrease priority of the current task based on feedback
        int dec = run / FF;
        tk->priority = std::max(1, tk->priority - dec);
        
        // then increase priority of all tasks in the queue
        // this is the aging part: tasks that wait longer get higher priority
        for (auto x : rq)
        {
            x->priority += AG;
        }

        while (next < tasks.size() && tasks[next].arrival_time <= t)
        {
            rq.insert(&tasks[next++]);
        }

        if (tk->remaining_time > 0)
        {
            rq.insert(tk);
        }
    }

    log("[PR] Done");
}


void Scheduler::runSJF()
{
    log("[SJF] Starting");
    int t = 0;
    size_t next = 0;
    std::vector<Task *> rq;
    rq.reserve(tasks.size());

    while (next < tasks.size() && tasks[next].arrival_time <= t)
    {
        rq.push_back(&tasks[next++]);
    }

    while (!rq.empty() || next < tasks.size())
    {
        if (rq.empty())
        {
            t = tasks[next].arrival_time;
            rq.push_back(&tasks[next++]);
        }

        // we find the task with the shortest remaining time
        // and remove it from the ready queue
        auto it = std::min_element(rq.begin(), rq.end(), [](Task *a, Task *b)
                                   {
                                       if (a->remaining_time != b->remaining_time)
                                           return a->remaining_time < b->remaining_time;
                                       return a->id < b->id; // tie-break by id
                                   });
        Task *tk = *it;
        rq.erase(it);

        int s = std::max(t, tk->arrival_time);
        int e = s + tk->remaining_time;

        _timeline.push_back({tk->id, s, e});
        log("[SJF] T" + std::to_string(tk->id) + " " + std::to_string(s) + "->" + std::to_string(e));

        std::this_thread::sleep_for(std::chrono::milliseconds(tk->remaining_time / 10));

        t = e;

        while (next < tasks.size() && tasks[next].arrival_time <= t)
        {
            rq.push_back(&tasks[next++]);
        }
    }

    log("[SJF] Done");
}


void Scheduler::runMLQ()
{
    log("[MLQ] Starting (3-level queues)");
    int t = 0;
    size_t next = 0;
    
    // we have 3 queues: low, medium, high priority
    // highQ: pr > 20, medQ: 10 < pr <= 20, lowQ: pr <= 10
    // we use deque for efficient pop_front() and push_back()
    std::deque<Task *> lowQ, medQ, highQ;

    while (next < tasks.size() && tasks[next].arrival_time <= t)
    {
        Task *tk = &tasks[next++];
        if (tk->priority > 20)
            highQ.push_back(tk);
        else if (tk->priority > 10)
            medQ.push_back(tk);
        else
            lowQ.push_back(tk);
    }

    while (!highQ.empty() || !medQ.empty() || !lowQ.empty() || next < tasks.size())
    {
        if (highQ.empty() && medQ.empty() && lowQ.empty())
        {
            t = tasks[next].arrival_time;
            Task *tk = &tasks[next++];
            if (tk->priority > 20)
                highQ.push_back(tk);
            else if (tk->priority > 10)
                medQ.push_back(tk);
            else
                lowQ.push_back(tk);
        }

        Task *tk;
        if (!highQ.empty())
        {
            tk = highQ.front();
            highQ.pop_front();
        }
        else if (!medQ.empty())
        {
            tk = medQ.front();
            medQ.pop_front();
        }
        else
        {
            tk = lowQ.front();
            lowQ.pop_front();
        }

        int s = std::max(t, tk->arrival_time);
        int e = s + tk->remaining_time;

        _timeline.push_back({tk->id, s, e});
        log("[MLQ] T" + std::to_string(tk->id) +
            " pr=" + std::to_string(tk->priority) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        std::this_thread::sleep_for(std::chrono::milliseconds(tk->remaining_time / 10));

        t = e;

        while (next < tasks.size() && tasks[next].arrival_time <= t)
        {
            Task *nt = &tasks[next++];
            if (nt->priority > 20)
                highQ.push_back(nt);
            else if (nt->priority > 10)
                medQ.push_back(nt);
            else
                lowQ.push_back(nt);
        }
    }

    log("[MLQ] Done");
}

void Scheduler::runMLFQ()
{
    log("[MLFQ] Starting (3-level MLFQ)");
    int t = 0;
    size_t next = 0;
    // we have 3 queues: q0, q1, q2
    // q0: time_quantum, q1: 2*quantum, q2: 4*quantum

    const int levels = 3;
    std::vector<std::deque<Task *>> queues(levels);

    while (next < tasks.size() && tasks[next].arrival_time <= t)
    {
        queues[0].push_back(&tasks[next++]);
    }

    while (next < tasks.size() || !queues[0].empty() || !queues[1].empty() || !queues[2].empty())
    {
        // we select the highest-priority non-empty queue
        int lvl = -1;
        for (int i = 0; i < levels; ++i)
        {
            if (!queues[i].empty())
            {
                lvl = i;
                break;
            }
        }

        if (lvl < 0)
        {
            t = tasks[next].arrival_time;
            queues[0].push_back(&tasks[next++]);
            lvl = 0;
        }

        Task *tk = queues[lvl].front();
        queues[lvl].pop_front();

        int quantum = time_quantum * (1 << lvl);
        int s = std::max(t, tk->arrival_time);
        int run = std::min(tk->remaining_time, quantum);
        int e = s + run;

        _timeline.push_back({tk->id, s, e});
        log("[MLFQ] T" + std::to_string(tk->id) +
            " L" + std::to_string(lvl) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));

        t = e;
        tk->remaining_time -= run;

        // we them have to enqueue tasks that arrived up to time t into level 0
        while (next < tasks.size() && tasks[next].arrival_time <= t)
        {
            queues[0].push_back(&tasks[next++]);
        }

        // if the task is not finished, we requeue it to the next level
        // if it is finished, we do not enqueue it again
        if (tk->remaining_time > 0)
        {
            int newLvl = std::min(lvl + 1, levels - 1);
            queues[newLvl].push_back(tk);
        }
    }

    log("[MLFQ] Done");
}


void Scheduler::runEDF()
{
    log("[EDF] Starting");
    int t = 0;
    size_t next = 0;

    // we use a multiset to keep track of the tasks in order of deadline
    auto edf_cmp = [](Task *a, Task *b)
    {
        if (a->deadline != b->deadline)
            return a->deadline < b->deadline;
        return a->id < b->id;
    };
    std::multiset<Task *, decltype(edf_cmp)> rq(edf_cmp);

    while (next < tasks.size() && tasks[next].arrival_time <= t)
    {
        rq.insert(&tasks[next++]);
    }

    while (!rq.empty() || next < tasks.size())
    {
        if (rq.empty())
        {
            t = tasks[next].arrival_time;
            rq.insert(&tasks[next++]);
        }

        // we dequeue the task with the earliest deadline
        Task *tk = *rq.begin();
        rq.erase(rq.begin());

        int s = std::max(t, tk->arrival_time);
        int run = std::min(tk->remaining_time, time_quantum);
        int e = s + run;

        _timeline.push_back({tk->id, s, e});
        log("[EDF] T" + std::to_string(tk->id) + " dl=" + std::to_string(tk->deadline) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));

        t = e;
        tk->remaining_time -= run;

        while (next < tasks.size() && tasks[next].arrival_time <= t)
        {
            rq.insert(&tasks[next++]);
        }

        if (tk->remaining_time > 0)
        {
            rq.insert(tk);
        }
    }

    log("[EDF] Done");
}

void Scheduler::runCFS()
{
    log("[CFS] Starting (with arrival times)");
    int t = 0;
    size_t next = 0;
    size_t n = tasks.size();

    // we use a vector to hold the tasks in order of arrival time
    std::vector<Task *> upcoming;
    upcoming.reserve(n);
    for (auto &tk : tasks)
        upcoming.push_back(&tk);

    // we use an unordered_map to keep track of the virtual runtime of each task
    // this is a mapping of task pointer to its virtual runtime
    std::unordered_map<Task *, double> vruntime;

    // pick smallest vruntime, tie-break on id
    auto cmp = [&](Task *a, Task *b)
    {
        double va = vruntime[a], vb = vruntime[b];
        if (va != vb)
            return va < vb;
        return a->id < b->id;
    };

    // we use a multiset (which is implemented using a red-black tree) to keep track of the tasks in order of vruntime
    std::multiset<Task *, decltype(cmp)> rq(cmp);

    while (next < n && upcoming[next]->arrival_time <= t)
    {
        Task *p = upcoming[next++];
        vruntime[p] = 0.0;
        rq.insert(p);
    }

    while (next < n || !rq.empty())
    {
        if (rq.empty())
        {
            t = upcoming[next]->arrival_time;
            while (next < n && upcoming[next]->arrival_time <= t)
            {
                Task *p = upcoming[next++];
                vruntime[p] = 0.0;
                rq.insert(p);
            }
        }

        // dequeue the task with minimum vruntime
        auto it = rq.begin();
        Task *tk = *it;
        rq.erase(it);

        // Calculate slice and times
        int slice = std::min(tk->remaining_time, time_quantum);
        int s = std::max(t, tk->arrival_time);
        int e = s + slice;

        _timeline.push_back({tk->id, s, e});
        log("[CFS] T" + std::to_string(tk->id) + " vruntime=" + std::to_string(vruntime[tk]) +
            " " + std::to_string(s) + "->" + std::to_string(e));

        std::this_thread::sleep_for(std::chrono::milliseconds(slice / 10));

        t = e;
        tk->remaining_time -= slice;
        vruntime[tk] += double(slice) / tk->priority;

        while (next < n && upcoming[next]->arrival_time <= t)
        {
            Task *p = upcoming[next++];
            vruntime[p] = 0.0;
            rq.insert(p);
        }

        if (tk->remaining_time > 0)
        {
            rq.insert(tk);
        }
    }

    log("[CFS] Done");
}
