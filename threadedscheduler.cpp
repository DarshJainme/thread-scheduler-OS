// File: threadedscheduler.cpp
#include "threadedscheduler.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <mutex>

std::mutex printLock;
std::mutex printMutex;
int shared_counter = 0;
std::mutex shared_mtx;

ThreadedScheduler::ThreadedScheduler(ThreadedAlgorithm algo, int tq, std::function<void(const std::string&)> log)
    : algorithm(algo), time_quantum(tq), logger(log) {
    tasks.emplace_back(std::make_unique<ThreadedTask>(1, 15, 250));
    tasks.emplace_back(std::make_unique<ThreadedTask>(2, 5, 100));
    tasks.emplace_back(std::make_unique<ThreadedTask>(3, 20, 300));
    tasks.emplace_back(std::make_unique<ThreadedTask>(4, 10, 150));
}

void ThreadedScheduler::log(const std::string& msg) {
    if (logger) logger(msg);
    else std::cout << msg << "\n";
}

const std::vector<ThreadedTimelineEntry>& ThreadedScheduler::timeline() const {
    return _timeline;
}

void ThreadedScheduler::task_worker(ThreadedTask* tk) {
    while (!tk->control.isFinished()) {
        {
            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << "[Thread " << std::this_thread::get_id()
                      << "] waiting for permission to run...\n";
        }

        tk->control.waitUntilRunnable();  // will block here

        {
            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << "[Thread " << std::this_thread::get_id()
                      << "] is now running task " << tk->id << "\n";
        }

        // Simulate doing some work
        {
            std::lock_guard<std::mutex> lock(shared_mtx);
            std::cout << "[Thread " << std::this_thread::get_id()
                      << "] ENTER critical section (shared_counter = " << shared_counter << ")\n";
        
            shared_counter += 1;  // Shared resource modification
        
            std::this_thread::sleep_for(std::chrono::milliseconds(30));  // Emphasize mutex delay
        
            std::cout << "[Thread " << std::this_thread::get_id()
                      << "] EXIT critical section (shared_counter = " << shared_counter << ")\n";
        }
        
        {
            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << "[Thread " << std::this_thread::get_id()
                      << "] finished this slice\n";
        }
    }

    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "[Thread " << std::this_thread::get_id()
                  << "] exiting task " << tk->id << "\n";
    }
}

void ThreadedScheduler::run() {
    switch (algorithm) {
        case T_FCFS: runFCFS(); break;
        case T_RR: runRR(); break;
        case T_PRIORITY: runPriority(); break;
    }
}

void ThreadedScheduler::runFCFS() {
    log("[T-FCFS] Starting");
    int t = 0;
    for (auto& tk : tasks) {
        tk->thread = std::thread(task_worker, tk.get());
        int s = t, e = s + tk->remaining_time;
        _timeline.push_back({tk->id, s, e});
        log("[T-FCFS] Task " + std::to_string(tk->id) + " " + std::to_string(s) + "->" + std::to_string(e));
        tk->control.wake();
        std::this_thread::sleep_for(std::chrono::milliseconds(tk->remaining_time / 10));
        t = e;
        tk->control.finish();
        tk->thread.join();
    }
    log("[T-FCFS] Done");
}

void ThreadedScheduler::runRR() {
    log("[T-RR] Starting");
    int t = 0;
    for (auto& tk : tasks) {
        tk->thread = std::thread(task_worker, tk.get());
    }
    bool work_left = true;
    while (work_left) {
        work_left = false;
        for (auto& tk : tasks) {
            if (tk->remaining_time > 0) {
                work_left = true;
                int run = std::min(tk->remaining_time, time_quantum);
                int s = t, e = s + run;
                _timeline.push_back({tk->id, s, e});
                log("[T-RR] Task " + std::to_string(tk->id) + " " + std::to_string(s) + "->" + std::to_string(e));
                tk->control.wake();
                std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));
                tk->remaining_time -= run;
                t = e;
            }
        }
    }
    for (auto& tk : tasks) {
        tk->control.finish();
        tk->thread.join();
    }
    log("[T-RR] Done");
}

void ThreadedScheduler::runPriority() {
    log("[T-PRIORITY] Starting");
    int t = 0;
    for (auto& tk : tasks) {
        tk->thread = std::thread(task_worker, tk.get());
    }
    const int FF = 50, AG = 1;
    while (!tasks.empty()) {
        std::sort(tasks.begin(), tasks.end(), [](const auto& a, const auto& b) {
            return a->priority > b->priority;
        });
        ThreadedTask* tk = tasks.front().get();
        int run = std::min(tk->remaining_time, time_quantum);
        int s = t, e = s + run;
        _timeline.push_back({tk->id, s, e});
        log("[T-PRIORITY] Task " + std::to_string(tk->id) + " pr=" + std::to_string(tk->priority));
        tk->control.wake();
        std::this_thread::sleep_for(std::chrono::milliseconds(run / 10));
        tk->remaining_time -= run;
        t = e;
        int dec = (time_quantum - tk->remaining_time) / FF;
        tk->priority = std::max(1, tk->priority - dec);
        for (size_t i = 1; i < tasks.size(); ++i) tasks[i]->priority += AG;
        if (tk->remaining_time <= 0) {
            tk->control.finish();
            tk->thread.join();
            tasks.erase(tasks.begin());
        }
    }
    log("[T-PRIORITY] Done");
}