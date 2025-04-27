#ifndef THREADEDSCHEDULER_H
#define THREADEDSCHEDULER_H

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "ult_sync.h" 

// Available scheduling algorithms
enum ThreadedAlgorithm {
    T_FCFS,
    T_RR,
    T_PRIORITY,
    T_MLFQ,
    T_CFS
};

// Represents a user-level thread task
struct ThreadedTask {
    int id;               // Task identifier
    int priority;         // Dynamic priority
    int remaining_time;   // Remaining work (ms)
    int queue_level;         // Current MLFQ level (0 = highest)
    int time_run_in_level;   // Time consumed in current level

    // CFS-specific fields
    double vruntime;         // Virtual runtime
    double weight;           // Weight based on niceness
    ThreadedTask(int i, int p, int r)
        : id(i), priority(p), remaining_time(r), queue_level(0), time_run_in_level(0),
        vruntime(0.0), weight(1.0) {}
};

// Records one slice of execution by a task
struct ThreadedTimelineEntry {
    int task_id;
    int start_time;
    int end_time;
};

// User-Level Thread Scheduler class
typedef std::function<void(const std::string&)> Logger;

class ThreadedScheduler {
public:
    ThreadedScheduler(ThreadedAlgorithm algo,
                      int tq = 100,
                      Logger log = nullptr);
    void run();
    const std::vector<ThreadedTimelineEntry>& timeline() const;

private:
    ThreadedAlgorithm algorithm;
    int time_quantum;
    Logger logger;

    std::vector<std::unique_ptr<ThreadedTask>> tasks;
    std::vector<ThreadedTimelineEntry> _timeline;

    void log(const std::string& msg);
    void runFCFS();
    void runRR();
    void runPriority();
    void runCFS();
    void runMLFQ();
};

#endif