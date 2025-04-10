#ifndef THREADCONTROL_H
#define THREADCONTROL_H

#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadControl {
public:
    ThreadControl();

    void waitUntilRunnable();        // Called inside thread
    void wake();                     // Called by scheduler
    void finish();                   // Signal thread to terminate
    bool isFinished() const;

private:
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> run_flag;
    std::atomic<bool> finished;
};

#endif // THREADCONTROL_H
