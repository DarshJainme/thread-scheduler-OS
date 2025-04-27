# Thread Scheduler

## Overview
This script is a simulation of various **CPU thread scheduling algorithms** implemented in Python. It allows users to run different scheduling strategies and observe how threads are scheduled over time. The script supports multiple algorithms, including First-Come, First-Served (FCFS), Round Robin (RR), Priority Scheduling, Shortest Job First (SJF), Multi-Level Queue (MLQ), Multi-Level Feedback Queue (MLFQ), Earliest Deadline First (EDF), and Completely Fair Scheduler (CFS).

Each scheduling algorithm has unique characteristics that determine how threads are selected and executed. This simulation helps understand the advantages and drawbacks of different scheduling policies in an operating system, particularly in **multi-threaded environments**.

Report Regarding our project is also present here.

## Supported Scheduling Algorithms

### 1. First-Come, First-Served (FCFS) - **Type: Non-Preemptive**
**Characteristics:**
- Threads are executed in the order they arrive in the ready queue.
- Once a thread starts execution, it **cannot be interrupted** until completion.
- **Simple and easy to implement**, making it ideal for batch systems.
- Can lead to the **convoy effect**, where short threads wait for long ones to finish.

### 2. Round Robin (RR) - **Type: Preemptive**
**Characteristics:**
- Each thread receives a **fixed time slice (quantum)** before being preempted.
- Ensures **fair CPU time distribution** among all threads.
- Performance depends on the choice of **time quantum**:
  - Too **small** → excessive context switching (overhead).
  - Too **large** → behaves like FCFS.
- Suitable for **interactive systems**.

### 3. Priority Scheduling - **Type: Preemptive / Non-Preemptive**
**Characteristics:**
- Threads are assigned **priorities**, with higher-priority threads executing first.
- Can be:
  - **Preemptive**: A higher-priority thread **interrupts** a lower-priority one.
  - **Non-Preemptive**: A thread runs until completion or blocking.
- Can cause **starvation** for low-priority threads (solved using **aging**).
- Used in **real-time and critical systems** where priority matters.

### 4. Shortest Job First (SJF) - **Type: Non-Preemptive**
**Characteristics:**
- Threads are scheduled based on their **execution time** (shortest job first).
- **Optimizes** average waiting time but requires knowledge of **execution time in advance**.
- **Starvation possible**: Longer jobs may **never get scheduled** if shorter ones keep arriving.
- Best suited for **batch processing** environments.

### 5. Multi-Level Queue (MLQ) - **Type: Preemptive / Non-Preemptive**
**Characteristics:**
- Threads are categorized into **different queues** based on priority or thread type (e.g., **system, interactive, batch**).
- Each queue has its own scheduling algorithm.
- **Fixed priority** between queues, meaning lower-priority queues may experience **starvation**.
- Suitable for **systems with distinct scheduling needs**.

### 6. Multi-Level Feedback Queue (MLFQ) - **Type: Preemptive**
**Characteristics:**
- **Improves MLQ** by allowing **dynamic movement** between queues.
- If a thread uses too much CPU time, it is **moved to a lower-priority queue**.
- If a thread waits too long, it may be **promoted** to a higher-priority queue.
- Balances **fairness and responsiveness** but requires careful tuning.
- Used in **modern OS schedulers** like Linux.

### 7. Earliest Deadline First (EDF) - **Type: Preemptive**
**Characteristics:**
- A **real-time scheduling algorithm** where the thread **with the closest deadline** runs first.
- Ensures **time-critical tasks** meet deadlines.
- Requires threads to have **defined deadlines**.
- Common in **embedded and real-time systems**.

### 8. Completely Fair Scheduler (CFS) - **Type: Preemptive**
**Characteristics:**
- Ensures **fair CPU distribution** among all threads.
- Uses **virtual runtime (vruntime)** to track CPU time usage.
- Threads with **low vruntime** get scheduled first.
- Provides a balance between **throughput and responsiveness**.
- Implemented in **Linux kernel**.

## How This Script Works
1. The user selects a scheduling algorithm when initializing the `Scheduler` class.
2. The script generates a **set of threads** with attributes like arrival time, burst time, and priority.
3. The selected algorithm processes the **ready queue** and schedules threads accordingly.
4. The scheduler maintains a **timeline of execution**, tracking:
   - When each thread starts and finishes.
   - Context switches for **preemptive algorithms**.
   - Starvation and aging effects (if applicable).
5. The script logs execution details for analysis and visualization.
6. Users can modify parameters like **quantum size, priority levels, and aging rules** to experiment with different behaviors.

## Running the Script
To execute the script with a specific scheduling algorithm, run:
```bash
python scheduler.py
```
By default, it runs the **Completely Fair Scheduler (CFS)**. You can change the algorithm by modifying the `Scheduler` initialization:
```python
s = Scheduler(algorithm="RR", time_quantum=100)
s.run()
```
