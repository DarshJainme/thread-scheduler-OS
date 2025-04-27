// File: analysis.cpp
// Purpose: Compare performance metrics (response time, waiting time, turnaround time)
// across different scheduling algorithms using the Scheduler class.

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <iomanip>
#include <random>
#include "analysis.h"
#include "scheduler.h"
#include "bits/stdc++.h"

void analyzeAlgorithms() {
    // Define a common set of originalTasks (arrival at time 0)
    std::vector<Task> originalTasks;
    std::random_device rd;
    std::mt19937 gen(rd());
    // choose your ranges here:
    std::uniform_int_distribution<> pri_d(1, 10);
    std::uniform_int_distribution<> rem_d(1, 500);
    std::uniform_int_distribution<> arr_d(0, 10);
    std::uniform_int_distribution<> dl_d(1, 500);
    std::uniform_int_distribution<> lv_d(1, 5);
    
    struct Metrics { std::string name; double resp, tat, wait; };
    std::vector<Metrics> all;

    originalTasks.reserve(1e3);
    for(int i = 1; i <= 1e3; ++i){
        Task tk;
        tk.id             = i;
        tk.priority       = pri_d(gen);
        tk.remaining_time = rem_d(gen);
        tk.arrival_time   = arr_d(gen);
        tk.deadline       = dl_d(gen);
        originalTasks.push_back(tk);
    }

    // Algorithms to test
    std::vector<Algorithm> algos = {
        FCFS, RR, PRIORITY, SJF, MLQ, MLFQ, EDF, CFS
    };
    std::vector<std::string> names = {
        "FCFS", "RR", "PRIORITY", "SJF",
        "MLQ", "MLFQ", "EDF", "CFS"
    };

    const int timeQuantum = 50;

    std::cout << std::fixed << std::setprecision(2);

    // Run each algorithm and compute metrics
    for (size_t i = 0; i < algos.size(); ++i) {
        Algorithm algo = algos[i];
        std::string name = names[i];

        // Instantiate scheduler with a no-op logger
        Scheduler sched(algo, timeQuantum, [](const std::string&) {});
        // Override originalTasks with our common set
        sched.tasks = originalTasks;
        // start timer (chronos::high_resolution_clock::now())
        auto start = std::chrono::high_resolution_clock::now();

        // Run the scheduler
        sched.run();

        // end timer
        auto end = std::chrono::high_resolution_clock::now();
        // Calculate elapsed time in milliseconds
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // Collect first start time and completion time per task
        std::map<int, int> firstStart;
        std::map<int, int> completion;
        for (const auto &entry : sched.timeline()) {
            int id = entry.task_id;
            // record first start
            if (!firstStart.count(id))
                firstStart[id] = entry.start_time;
            // update completion to latest end
            completion[id] = entry.end_time;
        }

        // Compute and accumulate metrics
        double totalResponse = 0.0;
        double totalTurnaround = 0.0;
        double totalWaiting = 0.0;
        int n = originalTasks.size();

        for (const auto &task : originalTasks) {
            int id = task.id;
            int burst = task.remaining_time;
            int resp = firstStart[id];            // arrival=0
            int tat = completion[id];           // completion - arrival(0)
            int wt  = tat - burst;              // turnaround - burst

            totalResponse   += resp;
            totalTurnaround += tat;
            totalWaiting    += wt;
        }

        // Compute averages
        double avgResp = totalResponse / n;
        double avgTat  = totalTurnaround / n;
        double avgWait = totalWaiting / n;

        // Print results
        std::cout << name << " Metrics:\n";
        std::cout << "  Elapsed Time       = " << elapsed << " ms\n";
        std::cout << "  Avg Response Time   = " << avgResp   << "\n";
        std::cout << "  Avg Turnaround Time = " << avgTat    << "\n";
        std::cout << "  Avg Waiting Time    = " << avgWait   << "\n\n";
        all.push_back({ name, avgResp, avgTat, avgWait });

        std::ofstream fout("metrics.csv");
        fout << "Algorithm,Response,Turnaround,Waiting\n";
        for (auto &m : all) {
            fout << m.name << ","
                << m.resp << ","
                << m.tat  << ","
                << m.wait << "\n";
        }
        fout.close();

        std::system(
            "python \"C:\\Users\\DELL\\thread-scheduler\\plot_metrics.py\""
        );
    }
}

// int main(){
//     analyzeAlgorithms();
//     return 0;
// }