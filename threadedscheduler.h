#ifndef THREADEDSCHEDULER_H
#define THREADEDSCHEDULER_H

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "ult_context.h"    
#include "ult_sync.h"      

enum ThreadedAlgorithm {
    T_FCFS,
    T_RR,
    T_PRIORITY,
    T_MLFQ,
    T_CFS
};

enum class ThreadState { NEW, READY, RUNNING, FINISHED };

// user‐level thread/task
struct ThreadedTask {
    int id;
    int priority;
    int arrival_time;
    int remaining_time;
    int queue_level;
    int time_run_in_level;
    double vruntime;
    double weight;
    int nice;
    ThreadState state;
  
    ThreadedTask(int i, int p, int r, int arr)
        : id(i), priority(p), arrival_time(arr), remaining_time(r),
          queue_level(0), time_run_in_level(0), vruntime(0.0),
          weight(1.0), nice(1), state(ThreadState::NEW) {}
};

// for recording run timeline
struct ThreadedTimelineEntry {
    int id;
    int start_time;
    int end_time;
    ThreadState state;
    int arrival_time;

    ThreadedTimelineEntry(int id_, int s, int e, ThreadState st, int arr)
        : id(id_), start_time(s), end_time(e), state(st), arrival_time(arr) {}
};

typedef std::function<void(const std::string&)> Logger;

class ThreadedScheduler {
public:
    ThreadedScheduler(ThreadedAlgorithm algo, int tq = 100, Logger log = nullptr);

    // run chosen scheduling algorithm
    void run();
    const std::vector<ThreadedTimelineEntry>& timeline() const;
    const std::vector<std::unique_ptr<ThreadedTask>>& get_tasks() const { return tasks; }

    ThreadedAlgorithm algorithm;
    int time_quantum;
    Logger logger;
    std::vector<std::unique_ptr<ThreadedTask>> tasks;
    std::vector<ThreadedTimelineEntry> _timeline;

private:
    void log(const std::string& msg);
    void runFCFS();
    void runRR();
    void runPriority();
    void runMLFQ();
    void runCFS();
};

#endif // THREADEDSCHEDULER_H