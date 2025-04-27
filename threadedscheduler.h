#ifndef THREADEDSCHEDULER_H
#define THREADEDSCHEDULER_H

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "ult_sync.h"
#include "ucontext_stubs.h"

enum ThreadedAlgorithm {
    T_FCFS,     
    T_RR, 
    T_PRIORITY,
    T_MLFQ, 
    T_CFS
};

enum class ThreadState { NEW, READY, RUNNING, FINISHED };

// the user-level thread task structure
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

    ThreadedTask(int i, int p, int r, int arr)
        : id(i), priority(p), arrival_time(arr), remaining_time(r),
          queue_level(0), time_run_in_level(0), vruntime(0.0),
          weight(1.0), state(ThreadState::NEW) {}
};

struct ThreadedTimelineEntry {
    int id;
    int start_time;
    int end_time;
    ThreadState state;
    int arrival_time;

    ThreadedTimelineEntry(int id, int start, int end, ThreadState state, int arrival_time)
        : id(id), start_time(start), end_time(end), state(state), arrival_time(arrival_time) {}
};

typedef std::function<void(const std::string&)> Logger;

class ThreadedScheduler {
public:
    ThreadedScheduler(ThreadedAlgorithm algo, int tq = 100, Logger log = nullptr);

    void run();
    const std::vector<ThreadedTimelineEntry>& timeline() const;
    const std::vector<std::unique_ptr<ThreadedTask>>& get_tasks() const;

    ThreadedAlgorithm algorithm;   
    int time_quantum;              
    Logger logger;                     

    // Internal data structures
    std::vector<std::unique_ptr<ThreadedTask>> tasks; 
    std::vector<ThreadedTimelineEntry> _timeline;

    // Helper methods
    void log(const std::string& msg);       
    void runFCFS();                          
    void runRR();                            
    void runPriority();                      
    void runCFS();                          
    void runMLFQ();                         
};


#endif 
