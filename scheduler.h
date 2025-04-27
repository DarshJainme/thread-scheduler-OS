// File: scheduler.h
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <string>
#include <functional>

enum Algorithm {
    FCFS, RR, PRIORITY, SJF, MLQ, MLFQ, EDF, CFS
};

struct Task {
    int id;
    int priority;
    int remaining_time;
    int arrival_time;
    int deadline;    // For EDF
};

struct TimelineEntry {
    int task_id;
    int start_time;
    int end_time;
};

class Scheduler {
public:
    Scheduler(Algorithm algo, int timeQuantum = 100, std::function<void(const std::string&)> logger = nullptr);
    void run();
    const std::vector<TimelineEntry>& timeline() const;
    void runFCFS();
    void runRR();
    void runPriority();
    void runSJF();
    void runMLQ();
    void runMLFQ();
    void runEDF();
    void runCFS();

    void log(const std::string& msg);

    Algorithm algorithm;
    int time_quantum;
    std::vector<Task> tasks;
    std::vector<TimelineEntry> _timeline;
    std::function<void(const std::string&)> logger;
};

#endif // SCHEDULER_H