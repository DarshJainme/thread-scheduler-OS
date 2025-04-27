#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <iomanip>
#include <random>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include "analysis.h"
#include "scheduler.h"

void analyzeAlgorithms() {
    std::vector<Task> originalTasks;
    std::random_device rd;
    std::mt19937 gen(rd());

    // generating random tasks for analysis
    std::uniform_int_distribution<> pri_d(1, 10);
    std::uniform_int_distribution<> rem_d(1, 500);
    std::uniform_int_distribution<> arr_d(0, 10);
    std::uniform_int_distribution<> dl_d(1, 500);

    for (int i = 1; i <= 100; ++i) {
        Task tk;
        tk.id             = i;
        tk.priority       = pri_d(gen);
        tk.remaining_time = rem_d(gen);
        tk.arrival_time   = arr_d(gen);
        tk.deadline       = dl_d(gen);
        originalTasks.push_back(tk);
    }

    struct Metrics { std::string name; double resp, tat, wait; };
    std::vector<Metrics> all;

    std::vector<Algorithm> algos = { FCFS, RR, PRIORITY, SJF, MLQ, MLFQ, EDF, CFS };
    std::vector<std::string> names = { "FCFS", "RR", "PRIORITY", "SJF", "MLQ", "MLFQ", "EDF", "CFS" };
    const int timeQuantum = 50;

    std::cout << std::fixed << std::setprecision(2);

    for (size_t i = 0; i < algos.size(); ++i) {
        Scheduler sched(algos[i], timeQuantum, [](const std::string&) {});
        sched.tasks = originalTasks;

        auto start = std::chrono::high_resolution_clock::now();
        sched.run();
        auto end   = std::chrono::high_resolution_clock::now();
        long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::map<int,int> firstStart, completion;
        for (const auto &e : sched.timeline()) {
            firstStart.try_emplace(e.id, e.start_time);
            completion[e.id] = e.end_time;
        }

        double totalResp=0, totalTat=0, totalWait=0;
        int n = originalTasks.size();
        for (auto &t : originalTasks) {
            int burst = t.remaining_time;
            int resp = firstStart[t.id] - t.arrival_time;
            int tat  = completion[t.id]    - t.arrival_time;
            int wt   = tat - burst;
            totalResp += resp;
            totalTat  += tat;
            totalWait += wt;
        }

        double avgResp = totalResp/n;
        double avgTat  = totalTat/n;
        double avgWait = totalWait/n;

        std::cout << names[i] << " Metrics:\n"
                  << "  Elapsed Time       = " << elapsed << " ms\n"
                  << "  Avg Response Time   = " << avgResp << "\n"
                  << "  Avg Turnaround Time = " << avgTat  << "\n"
                  << "  Avg Waiting Time    = " << avgWait << "\n\n";

        all.push_back({ names[i], avgResp, avgTat, avgWait });
    }

    std::ofstream fout("metrics.csv");
    std::cout<<"Writing metrics to metrics.csv\n";
    if (!fout) {
        std::cerr << "Error opening metrics.csv for writing\n";
        return;
    }
    fout << "Algorithm,Response,Turnaround,Waiting\n";
    for (auto &m : all) {
        std::cout << m.name << "," << m.resp << "," << m.tat << "," << m.wait << "\n";
        fout << m.name << "," << m.resp << "," << m.tat << "," << m.wait << "\n";
    }

    // int ret = std::system(R"(python "C:\Users\DELL\thread-scheduler\plot_metrics.py")");
    // std::cout << "return code: " << ret << "\n";
    // if (ret != 0) {
    //     std::cerr << "ERROR: plotting script returned code " << ret << "\n";
    // }
}
