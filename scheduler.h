// File: scheduler.h — updated to match initializer fields
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <string>
#include <functional>

// Supported scheduling algorithms
enum Algorithm {
    FCFS, RR, PRIORITY,
    SJF, MLQ, MLFQ,
    EDF, CFS
};

// Represents a task in the scheduler
struct Task {
    int id;
    int priority;
    int remaining_time;
    int arrival_time;
    int deadline;    // For EDF
    int level;       // For MLQ/MLFQ default queue level
};

// Single timeline entry: a slice of execution
struct TimelineEntry {
    int task_id;
    int start_time;
    int end_time;
};

// Main scheduler class
class Scheduler {
public:
    Scheduler(Algorithm algo,
              int timeQuantum = 100,
              std::function<void(const std::string&)> logger = nullptr);

    void run();
    const std::vector<TimelineEntry>& timeline() const;

    // Individual algorithm runners
    void runFCFS();
    void runRR();
    void runPriority();
    void runSJF();
    void runMLQ();
    void runMLFQ();
    void runEDF();
    void runCFS();

    // Logging utility
    void log(const std::string& msg);

private:
    Algorithm algorithm;
    int time_quantum;
    std::vector<Task> tasks;
    std::vector<TimelineEntry> _timeline;
    std::function<void(const std::string&)> logger;
};

#endif // SCHEDULER_H
