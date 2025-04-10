    #include "threadcontrol.h"
    #include <iostream>
    #include <thread>

    ThreadControl::ThreadControl()
        : run_flag(false), finished(false) {}

    void ThreadControl::waitUntilRunnable() {
        std::unique_lock<std::mutex> lock(mtx);
        std::cout << "[Thread " << std::this_thread::get_id() << "] waiting...\n";
        cv.wait(lock, [&]() { return run_flag || finished; });
        std::cout << "[Thread " << std::this_thread::get_id() << "] woke up!\n";
    }

    void ThreadControl::wake() {
        std::lock_guard<std::mutex> lock(mtx);
        run_flag = true;
        std::cout << "[Scheduler] Waking thread...\n";
        cv.notify_one();
    }

    void ThreadControl::finish() {
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
        run_flag = false;
        cv.notify_all();
    }

    bool ThreadControl::isFinished() const {
        return finished;
    }
