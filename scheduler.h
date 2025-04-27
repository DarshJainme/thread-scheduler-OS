#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <string>
#include <functional>

// we support the following algorithms
enum Algorithm {
    FCFS, RR, PRIORITY,
    SJF, MLQ, MLFQ,
    EDF, CFS
};

// Task structure: represents a task with its properties
struct Task {
    int id;                   // Task ID
    int priority;            // priority level
    int remaining_time;      // remaining time to finish (initially it's the burst time)
    int arrival_time;       // time of arrival
    int deadline;           // deadline for the task
    int level;              // for MLQ/MLFQ, the current level of the task 
};

// we record the timeline so we can draw the Gantt chart later
struct TimelineEntry {
    int id;
    int start_time;
    int end_time;
};

class Scheduler {
public:
    Scheduler(Algorithm algo,
              int timeQuantum = 100,
              std::function<void(const std::string&)> logger = nullptr);

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

#endif
