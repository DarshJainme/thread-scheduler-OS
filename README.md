# Thread Scheduler
Created by:
* Kartik Sarda: 23114047
* Nitin Agiwal: 23114074
* Darsh Jain: 23114023
* Shyam Agarwal: 23116089
* Krishna Pahariya: 23113089

## Overview
Efficient CPU scheduling is critical for both operating systems and user-level runtimes, as it directly affects application throughput, latency, and overall fairness. In this project, we present a flexible User–Mode Thread Scheduler with Dynamic Feedback that not only implements a novel adaptive scheduling framework but also exposes eight classical scheduling policies as selectable options:
* First–Come, First–Served (FCFS)
* Round Robin (RR)
* Priority Scheduling (with and without Aging)
* Shortest Job First (SJF)
* Multi–Level Queue (MLQ)
* Multi–Level Feedback Queue (MLFQ)
* Earliest Deadline First (EDF)
* Completely Fair Scheduler (CFS)

Users of our library can choose at runtime which of these eight policies to apply to their user-level threads, allowing easy comparison and integration into diverse application scenarios. Beyond offering these standard algorithms, our scheduler continuously monitors per-thread metrics—CPU utilization, latency to first run, and waiting times—and dynamically adjusts both time quanta and priorities during execution.


## Running the project
* Install Qt6 community (open-source) edition.
* Click on create new project.
* Select `CMakeLists.txt` from the file browser window.
* Run the project.

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

```
