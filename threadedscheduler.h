// File: threadedscheduler.h — updated for ULT implementation
#ifndef THREADEDSCHEDULER_H
#define THREADEDSCHEDULER_H

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "threadcontrol.h"

// Available scheduling algorithms
enum ThreadedAlgorithm {
    T_FCFS,
    T_RR,
    T_PRIORITY
};

// Represents a user‐level thread task
struct ThreadedTask {
    int id;               // Task identifier
    int priority;         // Dynamic priority
    int remaining_time;   // Remaining work (ms)
    ThreadControl control;// ULT control block

    ThreadedTask(int i, int p, int r)
        : id(i), priority(p), remaining_time(r) {}
};

// Records a single slice execution by a task
struct ThreadedTimelineEntry {
    int task_id;
    int start_time;
    int end_time;
};

// User‐Level Thread Scheduler
class ThreadedScheduler {
public:
    ThreadedScheduler(ThreadedAlgorithm algo,
                      int tq = 100,
                      std::function<void(const std::string&)> log = nullptr);
    void run();
    const std::vector<ThreadedTimelineEntry>& timeline() const;

private:
    ThreadedAlgorithm algorithm;
    int time_quantum;
    std::function<void(const std::string&)> logger;
    std::vector<std::unique_ptr<ThreadedTask>> tasks;
    std::vector<ThreadedTimelineEntry> _timeline;

    void log(const std::string& msg);
    void runFCFS();
    void runRR();
    void runPriority();
};

#endif // THREADEDSCHEDULER_H
