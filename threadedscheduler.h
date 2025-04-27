#ifndef THREADEDSCHEDULER_H
#define THREADEDSCHEDULER_H

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "ult_sync.h"  // Assuming this header is for synchronization
#include "ucontext_stubs.h"
// Available scheduling algorithms
enum ThreadedAlgorithm {
    T_FCFS,      // First-Come, First-Served
    T_RR,        // Round Robin
    T_PRIORITY,   // Priority Scheduling
    T_MLFQ,      // Multi-Level Feedback Queue
    T_CFS        // Completely Fair Scheduler
};
// ThreadState enum definition
enum class ThreadState { NEW, READY, RUNNING, FINISHED };

// Represents a user-level thread task
struct ThreadedTask {
    int id;
    int priority;
    int arrival_time;
    int remaining_time;
    int queue_level;
    int time_run_in_level;
    double vruntime;
    double weight;
    ThreadState state;

    // Constructor
    ThreadedTask(int i, int p, int r, int arr)
        : id(i), priority(p), arrival_time(arr), remaining_time(r),
          queue_level(0), time_run_in_level(0), vruntime(0.0),
          weight(1.0), state(ThreadState::NEW) {}
};

// Records one slice of execution by a task
struct ThreadedTimelineEntry {
    int id;
    int start_time;
    int end_time;
    ThreadState state;
    int arrival_time;

    // Constructor to initialize the fields
    ThreadedTimelineEntry(int id, int start, int end, ThreadState state, int arrival_time)
        : id(id), start_time(start), end_time(end), state(state), arrival_time(arrival_time) {}
};


// User-Level Thread Scheduler class
typedef std::function<void(const std::string&)> Logger;

class ThreadedScheduler {
public:
    // Constructor
    ThreadedScheduler(ThreadedAlgorithm algo, int tq = 100, Logger log = nullptr);

    // Scheduler methods
    void run();  // Start running the scheduler
    const std::vector<ThreadedTimelineEntry>& timeline() const;  // Get the execution timeline

    // Getter for tasks (to be used by other parts of the code)
    const std::vector<std::unique_ptr<ThreadedTask>>& get_tasks() const;

// private:
    // Scheduler state
    ThreadedAlgorithm algorithm;       // Current scheduling algorithm
    int time_quantum;                  // Time quantum for scheduling
    Logger logger;                     // Logger for messages

    // Internal data structures
    std::vector<std::unique_ptr<ThreadedTask>> tasks;  // List of tasks to be scheduled
    std::vector<ThreadedTimelineEntry> _timeline;       // Timeline of task execution

    // Helper methods
    void log(const std::string& msg);         // Log a message
    void runFCFS();                          // Run First-Come First-Served algorithm
    void runRR();                            // Run Round Robin algorithm
    void runPriority();                      // Run Priority Scheduling algorithm
    void runCFS();                           // Run Completely Fair Scheduler algorithm
    void runMLFQ();                          // Run Multi-Level Feedback Queue algorithm
};


#endif  // THREADEDSCHEDULER_H
