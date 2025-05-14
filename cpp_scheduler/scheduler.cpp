// File: cpp_scheduler/scheduler.cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <algorithm>
#include <sstream>

// ------------------------------
// Configuration constants
// ------------------------------

const int TIME_QUANTUM_MS = 100; // Time quantum in milliseconds
const int WORK_SLICE_MS = 10;    // Each work slice duration (ms)
const int TOTAL_WORK_MS = 500;   // Total work required per task (ms)
const int FEEDBACK_FACTOR = 50;  // Factor for dynamic feedback adjustment

// ------------------------------
// Task Structure
// ------------------------------
struct Task
{
    int id;
    int basePriority;    // Initial fixed priority
    int dynamicPriority; // Priority adjusted dynamically
    int cpuTime;         // Accumulated CPU time (ms)
    int remainingWork;   // Work left (ms)
    bool run_flag;       // Flag to signal running
    bool finished;       // Finished flag

    std::mutex mtx;
    std::condition_variable cv;

    Task(int id, int basePriority)
        : id(id),
          basePriority(basePriority),
          dynamicPriority(basePriority),
          cpuTime(0),
          remainingWork(TOTAL_WORK_MS),
          run_flag(false),
          finished(false)
    {
    }

    // Simulated work: loop until work is complete.
    void work()
    {
        while (true)
        {
            // Wait until run_flag is set or finished.
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]
                    { return run_flag || finished; });
            if (finished)
                break;
            lock.unlock();

            // Simulate work by sleeping WORK_SLICE_MS.
            // std::this_thread::sleep_for(std::chrono::milliseconds(WORK_SLICE_MS));
            cpuTime += WORK_SLICE_MS;
            remainingWork -= WORK_SLICE_MS;

            // Log progress.
            std::cout << "[Task " << id << "] Running... CPU time = "
                      << cpuTime << " ms, remaining work = "
                      << remainingWork << " ms\n";

            if (remainingWork <= 0)
            {
                std::unique_lock<std::mutex> lock2(mtx);
                finished = true;
                run_flag = false;
                cv.notify_all();
                std::cout << "[Task " << id << "] Finished execution.\n";
                break;
            }
        }
    }
};

// ------------------------------
// Scheduler Engine (C++ Class)
// ------------------------------
enum SchedulerType
{
    FCFS,
    RR,
    PRIORITY
};

class SchedulerEngine
{
public:
    SchedulerEngine(SchedulerType type)
        : schedType(type)
    {
    }

    // Add a task pointer to our list.
    void addTask(Task *t)
    {
        std::lock_guard<std::mutex> lock(engine_mtx);
        tasks.push_back(t);
    }

    // Run the scheduler until all tasks are finished.
    void run()
    {
        // Start each task's work in a separate thread.
        for (Task *t : tasks)
        {
            taskThreads.emplace_back(&Task::work, t);
        }

        while (true)
        {
            {
                std::lock_guard<std::mutex> lock(engine_mtx);
                // Remove finished tasks.
                tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                                           [](Task *t)
                                           { return t->finished; }),
                            tasks.end());
                if (tasks.empty())
                    break;
            }
            // For each remaining task (using simple round-robin or chosen scheduling),
            // signal it to run for one time quantum.
            Task *nextTask = selectTask();
            if (nextTask)
            {
                runTaskForQuantum(nextTask);
                applyDynamicFeedback(nextTask);
            }
        }

        // Wait for all task threads to finish.
        for (std::thread &th : taskThreads)
        {
            if (th.joinable())
                th.join();
        }
        std::cout << "All tasks finished.\n";
    }

    // Generate a summary string of all tasks' states.
    std::string getStates()
    {
        std::lock_guard<std::mutex> lock(engine_mtx);
        std::stringstream ss;
        for (auto t : allTasks)
        {
            ss << t->id << "," << (t->finished ? "Finished" : "Running") << ","
               << t->dynamicPriority << "," << t->cpuTime << ";";
        }
        return ss.str();
    }

    // Register tasks for final summary.
    void registerTask(Task *t)
    {
        allTasks.push_back(t);
    }

private:
    SchedulerType schedType;
    std::vector<Task *> tasks;
    std::vector<Task *> allTasks; // For summary (all tasks ever added)
    std::vector<std::thread> taskThreads;
    std::mutex engine_mtx;

    // Choose next task based on scheduling algorithm.
    Task *selectTask()
    {
        std::lock_guard<std::mutex> lock(engine_mtx);
        if (tasks.empty())
            return nullptr;
        if (schedType == FCFS)
        {
            return tasks.front();
        }
        else if (schedType == RR)
        {
            static size_t rr_index = 0;
            Task *t = tasks[rr_index % tasks.size()];
            rr_index = (rr_index + 1) % tasks.size();
            return t;
        }
        else if (schedType == PRIORITY)
        {
            auto it = std::max_element(tasks.begin(), tasks.end(),
                                       [](Task *a, Task *b)
                                       { return a->dynamicPriority < b->dynamicPriority; });
            return (it != tasks.end()) ? *it : nullptr;
        }
        return nullptr;
    }

    // Signal a task to run for one time quantum.
    void runTaskForQuantum(Task *task)
    {
        {
            std::unique_lock<std::mutex> lock(task->mtx);
            task->run_flag = true;
            task->cv.notify_one();
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds(TIME_QUANTUM_MS));
        {
            std::unique_lock<std::mutex> lock(task->mtx);
            task->run_flag = false;
        }
    }

    // Adjust the dynamic priority based on CPU time.
    void applyDynamicFeedback(Task *task)
    {
        int newPriority = task->basePriority - (task->cpuTime / FEEDBACK_FACTOR);
        if (newPriority < 1)
            newPriority = 1;
        std::cout << "[Scheduler] Adjusting Task " << task->id
                  << " priority from " << task->dynamicPriority
                  << " to " << newPriority << "\n";
        task->dynamicPriority = newPriority;
    }
};

// ------------------------------
// Global scheduler engine instance pointer.
// ------------------------------
SchedulerEngine *gScheduler = nullptr;

// ------------------------------
// Exported C functions (for Python binding)
// ------------------------------
extern "C"
{

    // Initialize the scheduler with a given algorithm.
    // type: 0 = FCFS, 1 = RR, 2 = PRIORITY.
    void init_scheduler(int type)
    {
        SchedulerType schedType = RR;
        if (type == 0)
            schedType = FCFS;
        else if (type == 2)
            schedType = PRIORITY;
        gScheduler = new SchedulerEngine(schedType);
    }

    // Add a task with id, base priority, and burst (in time quanta)
    void add_thread(int thread_id, int base_priority, int burst_quanta)
    {
        if (gScheduler == nullptr)
            return;
        Task *t = new Task(thread_id, base_priority);
        // For simulation, set total work as burst_quanta * TIME_QUANTUM_MS.
        t->remainingWork = burst_quanta * TIME_QUANTUM_MS;
        gScheduler->addTask(t);
        gScheduler->registerTask(t); // Keep a record for final reporting.
    }

    // Run the scheduler.
    void run_scheduler()
    {
        if (gScheduler == nullptr)
            return;
        gScheduler->run();
    }

    // Get the scheduler’s tasks state as a CSV string.
    // Format per task: "id,state,dynamicPriority,cpuTime;"
    const char *get_thread_states()
    {
        static std::string out;
        if (gScheduler == nullptr)
            return "";
        out = gScheduler->getStates();
        return out.c_str();
    }

    // Clean up the scheduler instance.
    void cleanup_scheduler()
    {
        if (gScheduler)
        {
            delete gScheduler;
            gScheduler = nullptr;
        }
    }
} // extern "C"
