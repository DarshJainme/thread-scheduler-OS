// File: threadedscheduler.h
#ifndef THREADEDSCHEDULER_H
#define THREADEDSCHEDULER_H

#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <functional>
#include "threadcontrol.h"

enum ThreadedAlgorithm {
    T_FCFS, T_RR, T_PRIORITY
};

struct ThreadedTask {
    int id;
    int priority;
    int remaining_time;
    std::thread thread;
    ThreadControl control;

    ThreadedTask(int i, int p, int r)
        : id(i), priority(p), remaining_time(r) {}
};

struct ThreadedTimelineEntry {
    int task_id;
    int start_time;
    int end_time;
};

class ThreadedScheduler {
public:
    ThreadedScheduler(ThreadedAlgorithm algo, int tq = 100, std::function<void(const std::string&)> log = nullptr);
    void run();
    const std::vector<ThreadedTimelineEntry>& timeline() const;

private:
    ThreadedAlgorithm algorithm;
    int time_quantum;
    std::function<void(const std::string&)> logger;
    std::vector<std::unique_ptr<ThreadedTask>> tasks;
    std::vector<ThreadedTimelineEntry> _timeline;

    void log(const std::string& msg);
    static void task_worker(ThreadedTask* task);

    void runFCFS();
    void runRR();
    void runPriority();
};

#endif // THREADEDSCHEDULER_H