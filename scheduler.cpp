#include "scheduler.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>

Scheduler::Scheduler(Algorithm algo, int tq, std::function<void(const std::string&)> lg)
    : algorithm(algo), time_quantum(tq), logger(lg)
{
    // initialize tasks for each algorithm
    if (algo == PRIORITY) {
        tasks = {{1,15,250,0,0,0},{2,5,100,0,0,0},{3,20,300,0,0,0},{4,10,150,0,0,0}};
    } else {
        tasks = {{1,1,250,0,0,0},{2,1,100,0,0,0},{3,1,300,0,0,0},{4,1,150,0,0,0}};
    }
}

void Scheduler::log(const std::string &msg) {
    if (logger) logger(msg);
    else std::cout << msg << "\n";
}

const std::vector<TimelineEntry>& Scheduler::timeline() const {
    return _timeline;
}
void Scheduler::run() {
    switch (algorithm) {
        case FCFS:    runFCFS();    break;
        case RR:      runRR();      break;
        case PRIORITY:runPriority();break;
        case SJF:     runSJF();     break;
        case MLQ:     runMLQ();     break;
        case MLFQ:    runMLFQ();    break;
        case EDF:     runEDF();     break;
        case CFS:     runCFS();     break;
    }
}

// Each implementation sleeps run_time/10 ms to simulate work.

void Scheduler::runFCFS(){
    log("[FCFS] Starting");
    int t=0;
    for(auto &tk:tasks){
        int s=t, e=s+tk.remaining_time;
        _timeline.push_back({tk.id,s,e});
        log("[FCFS] T"+std::to_string(tk.id)+" "+std::to_string(s)+"->"+std::to_string(e));
        std::this_thread::sleep_for(std::chrono::milliseconds(tk.remaining_time/10));
        t=e;
    }
    log("[FCFS] Done");
}

void Scheduler::runRR(){
    log("[RR] Starting");
    int t=0; auto q=tasks;
    while(!q.empty()){
        for(size_t i=0;i<q.size();){
            auto &tk=q[i];
            int run=std::min(tk.remaining_time,time_quantum);
            int s=t,e=s+run;
            _timeline.push_back({tk.id,s,e});
            log("[RR] T"+std::to_string(tk.id)+" "+std::to_string(s)+"->"+std::to_string(e));
            std::this_thread::sleep_for(std::chrono::milliseconds(run/10));
            tk.remaining_time-=run; t=e;
            if(tk.remaining_time<=0) q.erase(q.begin()+i);
            else ++i;
        }
    }
    log("[RR] Done");
}

void Scheduler::runPriority(){
    log("[PR] Starting with feedback+aging");
    int t=0; auto q=tasks;
    const int FF=50, AG=1;
    while(!q.empty()){
        std::sort(q.begin(),q.end(),[](auto&a,auto&b){return a.priority>b.priority;});
        auto &tk=q[0];
        int run=std::min(tk.remaining_time,time_quantum), s=t,e=s+run;
        _timeline.push_back({tk.id,s,e});
        log("[PR] T"+std::to_string(tk.id)+" pr="+std::to_string(tk.priority));
        std::this_thread::sleep_for(std::chrono::milliseconds(run/10));
        tk.remaining_time-=run; t=e;
        int dec=(time_quantum - tk.remaining_time)/FF;
        tk.priority=std::max(1,tk.priority-dec);
        for(size_t i=1;i<q.size();++i) q[i].priority+=AG;
        if(tk.remaining_time<=0) q.erase(q.begin());
    }
    log("[PR] Done");
}

void Scheduler::runSJF(){
    log("[SJF] Starting");
    int t=0;
    auto q=tasks;
    std::sort(q.begin(),q.end(),[](auto&a,auto&b){return a.remaining_time<b.remaining_time;});
    for(auto &tk:q){
        int s=t,e=s+tk.remaining_time;
        _timeline.push_back({tk.id,s,e});
        log("[SJF] T"+std::to_string(tk.id)+" "+std::to_string(s)+"->"+std::to_string(e));
        std::this_thread::sleep_for(std::chrono::milliseconds(tk.remaining_time/10));
        t=e;
    }
    log("[SJF] Done");
}

void Scheduler::runMLQ(){
    log("[MLQ] Starting");
    int t=0;
    std::vector<Task> high,low;
    for(auto &tk:tasks) if(tk.priority>1) high.push_back(tk); else low.push_back(tk);
    for(auto &q: {high,low}){
        for(auto &tk:q){
            int s=t,e=s+tk.remaining_time;
            _timeline.push_back({tk.id,s,e});
            log("[MLQ] T"+std::to_string(tk.id)+" "+std::to_string(s)+"->"+std::to_string(e));
            std::this_thread::sleep_for(std::chrono::milliseconds(tk.remaining_time/10));
            t=e;
        }
    }
    log("[MLQ] Done");
}

void Scheduler::runMLFQ(){
    log("[MLFQ] Starting");
    int t=0;
    auto q0=tasks, q1=std::vector<Task>{};
    while(!q0.empty()||!q1.empty()){
        auto &q = !q0.empty()? q0 : q1;
        auto tk=q.front(); q.erase(q.begin());
        int quantum = (&q==&q0? time_quantum:2*time_quantum);
        int run=std::min(tk.remaining_time,quantum), s=t,e=s+run;
        _timeline.push_back({tk.id,s,e});
        log("[MLFQ] T"+std::to_string(tk.id)+" L"+std::to_string(tk.level));
        std::this_thread::sleep_for(std::chrono::milliseconds(run/10));
        t=e; tk.remaining_time-=run;
        if(tk.remaining_time>0){
            if(tk.level==0){ tk.level=1; q1.push_back(tk);}
            else q1.push_back(tk);
        }
    }
    log("[MLFQ] Done");
}

void Scheduler::runEDF(){
    log("[EDF] Starting");
    int t=0; auto q=tasks;
    while(!q.empty()){
        std::sort(q.begin(),q.end(),[](auto&a,auto&b){return a.deadline<b.deadline;});
        auto &tk=q[0];
        int run=std::min(tk.remaining_time,time_quantum), s=t,e=s+run;
        _timeline.push_back({tk.id,s,e});
        log("[EDF] T"+std::to_string(tk.id)+" dl="+std::to_string(tk.deadline));
        std::this_thread::sleep_for(std::chrono::milliseconds(run/10));
        t=e; tk.remaining_time-=run;
        if(tk.remaining_time<=0) q.erase(q.begin());
    }
    log("[EDF] Done");
}

void Scheduler::runCFS(){
    log("[CFS] Starting");
    int t=0; auto q=tasks;
    while(!q.empty()){
        std::sort(q.begin(),q.end(),[](auto&a,auto&b){return a.vruntime<b.vruntime;});
        auto &tk=q[0];
        int run=std::min(tk.remaining_time,time_quantum), s=t,e=s+run;
        _timeline.push_back({tk.id,s,e});
        log("[CFS] T"+std::to_string(tk.id));
        std::this_thread::sleep_for(std::chrono::milliseconds(run/10));
        t=e; tk.remaining_time-=run;
        tk.vruntime+=double(run)/tk.priority;
        if(tk.remaining_time<=0) q.erase(q.begin());
    }
    log("[CFS] Done");
}
