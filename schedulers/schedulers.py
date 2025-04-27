#!/usr/bin/env python3
import time

class Scheduler:
    def __init__(self, algorithm="RR", time_quantum=100, logger=None):
        """
        Parameters:
        - algorithm: "FCFS", "RR", "PRIORITY", "SJF", "MLQ", "MLFQ", "EDF", or "CFS"
        - time_quantum: time in milliseconds for each time slice (used in RR, Priority, etc).
        - logger: Optional function to log text messages (defaults to print).

        Each task is defined with:
         - id: unique task identifier
         - priority: integer priority (higher = more weight) [if applicable]
         - remaining_time: total CPU time needed by this task in ms
         - deadline: (for EDF) deadline in ms
         - level: (for MLFQ) the current queue level (0 is highest)
         - vruntime: (for CFS) the virtual runtime, used to ensure fairness
        """
        self.algorithm = algorithm.upper()
        self.time_quantum = time_quantum
        self.logger = logger or print  # fallback to print if no logger given

        # Timeline: list of (task_id, start_ms, end_ms)
        self.timeline = []

        # Define tasks based on algorithm.
        if self.algorithm in ["FCFS", "RR"]:
            self.tasks = [
                {'id': 1, 'priority': 1, 'remaining_time': 250},
                {'id': 2, 'priority': 1, 'remaining_time': 100},
                {'id': 3, 'priority': 1, 'remaining_time': 300},
                {'id': 4, 'priority': 1, 'remaining_time': 150},
            ]
        elif self.algorithm == "PRIORITY":
            self.tasks = [
                {'id': 1, 'priority': 15, 'remaining_time': 250},
                {'id': 2, 'priority': 5,  'remaining_time': 100},
                {'id': 3, 'priority': 20, 'remaining_time': 300},
                {'id': 4, 'priority': 10, 'remaining_time': 150},
            ]
        elif self.algorithm == "SJF":
            # For SJF, we assume all tasks are available at t=0.
            self.tasks = [
                {'id': 1, 'priority': 1, 'remaining_time': 250},
                {'id': 2, 'priority': 1, 'remaining_time': 100},
                {'id': 3, 'priority': 1, 'remaining_time': 300},
                {'id': 4, 'priority': 1, 'remaining_time': 150},
            ]
        elif self.algorithm == "MLQ":
            # Multilevel Queue: we assign tasks into two fixed queues.
            # Here, tasks with even IDs (2,4) are high priority; odd IDs (1,3) are low.
            self.tasks = [
                {'id': 1, 'priority': 1, 'remaining_time': 250},  # low
                {'id': 2, 'priority': 2, 'remaining_time': 100},  # high
                {'id': 3, 'priority': 1, 'remaining_time': 300},  # low
                {'id': 4, 'priority': 2, 'remaining_time': 150},  # high
            ]
        elif self.algorithm == "MLFQ":
            # Multilevel Feedback Queue:
            # Each task starts at level 0; if it uses up its quantum, it is demoted.
            self.tasks = [
                {'id': 1, 'priority': 1, 'remaining_time': 250, 'level': 0},
                {'id': 2, 'priority': 1, 'remaining_time': 100, 'level': 0},
                {'id': 3, 'priority': 1, 'remaining_time': 300, 'level': 0},
                {'id': 4, 'priority': 1, 'remaining_time': 150, 'level': 0},
            ]
        elif self.algorithm == "EDF":
            # Earliest Deadline First: tasks with deadlines.
            self.tasks = [
                {'id': 1, 'priority': 1, 'remaining_time': 250, 'deadline': 400},
                {'id': 2, 'priority': 1, 'remaining_time': 100, 'deadline': 200},
                {'id': 3, 'priority': 1, 'remaining_time': 300, 'deadline': 500},
                {'id': 4, 'priority': 1, 'remaining_time': 150, 'deadline': 300},
            ]
        elif self.algorithm == "CFS":
            # Completely Fair Scheduling:
            # Each task starts with a virtual runtime (vruntime) of 0.
            self.tasks = [
                {'id': 1, 'priority': 1, 'remaining_time': 250, 'vruntime': 0},
                {'id': 2, 'priority': 1, 'remaining_time': 100, 'vruntime': 0},
                {'id': 3, 'priority': 1, 'remaining_time': 300, 'vruntime': 0},
                {'id': 4, 'priority': 1, 'remaining_time': 150, 'vruntime': 0},
            ]
        else:
            self.logger(f"Invalid algorithm: {self.algorithm}")
            self.tasks = []

    def run(self):
        if self.algorithm == "FCFS":
            self.run_fcfs()
        elif self.algorithm == "RR":
            self.run_rr()
        elif self.algorithm == "PRIORITY":
            self.run_priority()
        elif self.algorithm == "SJF":
            self.run_sjf()
        elif self.algorithm == "MLQ":
            self.run_mlq()
        elif self.algorithm == "MLFQ":
            self.run_mlfq()
        elif self.algorithm == "EDF":
            self.run_edf()
        elif self.algorithm == "CFS":
            self.run_cfs()
        else:
            self.logger("Invalid algorithm specified.")

    def run_fcfs(self):
        self.logger("\n[FCFS] Starting FCFS scheduling.")
        current_time = 0
        for task in self.tasks:
            start_time = current_time
            run_duration = task['remaining_time']
            end_time = start_time + run_duration
            self.timeline.append((task['id'], start_time, end_time))
            self.logger(f"[FCFS] Task {task['id']} runs from {start_time} to {end_time} (needs {run_duration}ms).")
            time.sleep(run_duration / 1000.0 * 0.1)
            current_time = end_time
            self.logger(f"[FCFS] Task {task['id']} finished at time {current_time}ms.")
        self.logger("[FCFS] All tasks finished.\n")

    def run_rr(self):
        self.logger("\n[RR] Starting Round Robin scheduling.")
        current_time = 0
        tasks = [dict(t) for t in self.tasks]
        cycle_count = 1
        while tasks:
            self.logger(f"[RR] -- Cycle {cycle_count} --")
            cycle_count += 1
            for task in tasks.copy():
                if task['remaining_time'] <= 0:
                    tasks.remove(task)
                    continue
                start_time = current_time
                run_duration = min(self.time_quantum, task['remaining_time'])
                end_time = start_time + run_duration
                task['remaining_time'] -= run_duration
                self.timeline.append((task['id'], start_time, end_time))
                self.logger(f"[RR] Running Task {task['id']}: {start_time} -> {end_time}, remaining {task['remaining_time']}ms.")
                time.sleep(run_duration / 1000.0 * 0.1)
                current_time = end_time
                if task['remaining_time'] <= 0:
                    self.logger(f"[RR] Task {task['id']} finished at {current_time}ms.")
                    tasks.remove(task)
        self.logger("[RR] All tasks finished.\n")

    def run_priority(self):
        self.logger("\n[PRIORITY] Starting Priority scheduling with dynamic feedback and aging.")
        current_time = 0
        tasks = [dict(t) for t in self.tasks]
        FEEDBACK_FACTOR = 50
        AGING_INCREMENT = 1  # Boost priority of waiting tasks

        while tasks:
            # Sort by current priority (higher = better)
            tasks.sort(key=lambda x: -x['priority'])

            # Pick highest-priority task to run
            task = tasks[0]
            start_time = current_time
            run_duration = min(self.time_quantum, task['remaining_time'])
            end_time = start_time + run_duration
            task['remaining_time'] -= run_duration
            self.timeline.append((task['id'], start_time, end_time))
            self.logger(f"[PRIORITY] Running Task {task['id']} from {start_time} to {end_time} (priority={task['priority']}).")
            time.sleep(run_duration / 1000.0 * 0.1)
            current_time = end_time

            # Dynamic feedback: decay priority of running task
            cpu_time_used = self.time_quantum - task['remaining_time']
            new_priority = task['priority'] - (cpu_time_used // FEEDBACK_FACTOR)
            if new_priority < 1:
                new_priority = 1
            self.logger(f"[PRIORITY] Adjusted priority of Task {task['id']} from {task['priority']} to {new_priority}")
            task['priority'] = new_priority

            # Aging: increase priority of all waiting tasks
            for i in range(1, len(tasks)):
                waiting_task = tasks[i]
                waiting_task['priority'] += AGING_INCREMENT
                self.logger(f"[PRIORITY] Aging: Increased priority of Task {waiting_task['id']} to {waiting_task['priority']}")

            if task['remaining_time'] <= 0:
                self.logger(f"[PRIORITY] Task {task['id']} finished at {current_time}ms.")
                tasks.remove(task)

        self.logger("[PRIORITY] All tasks finished.\n")



    def run_sjf(self):
        self.logger("\n[SJF] Starting Shortest Job First scheduling (non-preemptive).")
        # Non-preemptive SJF: sort tasks by job length (remaining_time)
        tasks = sorted(self.tasks, key=lambda x: x['remaining_time'])
        current_time = 0
        for task in tasks:
            start_time = current_time
            run_duration = task['remaining_time']
            end_time = start_time + run_duration
            self.timeline.append((task['id'], start_time, end_time))
            self.logger(f"[SJF] Task {task['id']} (job length {run_duration}ms) runs from {start_time} to {end_time}.")
            time.sleep(run_duration / 1000.0 * 0.1)
            current_time = end_time
            self.logger(f"[SJF] Task {task['id']} finished at {current_time}ms.")
        self.logger("[SJF] All tasks finished.\n")

    def run_mlq(self):
        self.logger("\n[MLQ] Starting Multilevel Queue scheduling.")
        current_time = 0
        # Divide tasks into two queues: high (priority 2) and low (priority 1)
        high_queue = [t for t in self.tasks if t['priority'] == 2]
        low_queue = [t for t in self.tasks if t['priority'] == 1]
        for queue_name, queue in [("High", high_queue), ("Low", low_queue)]:
            self.logger(f"[MLQ] Running {queue_name} priority queue.")
            for task in queue:
                start_time = current_time
                run_duration = task['remaining_time']
                end_time = start_time + run_duration
                self.timeline.append((task['id'], start_time, end_time))
                self.logger(f"[MLQ] Task {task['id']} from {start_time} to {end_time} (Queue: {queue_name}).")
                time.sleep(run_duration / 1000.0 * 0.1)
                current_time = end_time
                self.logger(f"[MLQ] Task {task['id']} finished at {current_time}ms.")
        self.logger("[MLQ] All tasks finished.\n")

    def run_mlfq(self):
        self.logger("\n[MLFQ] Starting Multilevel Feedback Queue scheduling.")
        current_time = 0
        # Two levels: level 0 (quantum = self.time_quantum) and level 1 (quantum = 2*self.time_quantum)
        level0_quantum = self.time_quantum
        level1_quantum = 2 * self.time_quantum
        tasks = [dict(t) for t in self.tasks]  # tasks include 'level' key
        queue0 = tasks[:]  # All tasks start at level 0
        queue1 = []
        cycle = 1
        while queue0 or queue1:
            self.logger(f"[MLFQ] -- Cycle {cycle} --")
            cycle += 1
            if queue0:
                task = queue0.pop(0)
                quantum = level0_quantum
            elif queue1:
                task = queue1.pop(0)
                quantum = level1_quantum
            else:
                break

            start_time = current_time
            run_duration = min(quantum, task['remaining_time'])
            end_time = start_time + run_duration
            task['remaining_time'] -= run_duration
            self.timeline.append((task['id'], start_time, end_time))
            self.logger(f"[MLFQ] Task {task['id']} at level {task.get('level', 0)} runs from {start_time} to {end_time}, remaining {task['remaining_time']}ms.")
            time.sleep(run_duration / 1000.0 * 0.1)
            current_time = end_time
            if task['remaining_time'] > 0:
                if task.get('level', 0) == 0:
                    task['level'] = 1
                    queue1.append(task)
                    self.logger(f"[MLFQ] Task {task['id']} demoted to level 1.")
                else:
                    queue1.append(task)
            else:
                self.logger(f"[MLFQ] Task {task['id']} finished at {current_time}ms.")
        self.logger("[MLFQ] All tasks finished.\n")

    def run_edf(self):
        self.logger("\n[EDF] Starting Earliest Deadline First scheduling (preemptive).")
        current_time = 0
        tasks = [dict(t) for t in self.tasks]
        while tasks:
            tasks.sort(key=lambda x: x['deadline'])
            task = tasks[0]
            start_time = current_time
            run_duration = min(self.time_quantum, task['remaining_time'])
            end_time = start_time + run_duration
            task['remaining_time'] -= run_duration
            self.timeline.append((task['id'], start_time, end_time))
            self.logger(f"[EDF] Running Task {task['id']} (deadline {task['deadline']}) from {start_time} to {end_time}, remaining {task['remaining_time']}ms.")
            time.sleep(run_duration / 1000.0 * 0.1)
            current_time = end_time
            if task['remaining_time'] <= 0:
                self.logger(f"[EDF] Task {task['id']} finished at {current_time}ms.")
                tasks.remove(task)
        self.logger("[EDF] All tasks finished.\n")

    def run_cfs(self):
        self.logger("\n[CFS] Starting Completely Fair Scheduling.")
        current_time = 0
        tasks = [dict(t) for t in self.tasks]  # Each task has a 'vruntime' field.
        while tasks:
            # Select the task with the smallest virtual runtime.
            tasks.sort(key=lambda x: x['vruntime'])
            task = tasks[0]
            slice_time = min(self.time_quantum, task['remaining_time'])
            start_time = current_time
            end_time = start_time + slice_time
            self.timeline.append((task['id'], start_time, end_time))
            self.logger(f"[CFS] Running Task {task['id']} from {start_time} to {end_time}, remaining {task['remaining_time'] - slice_time}ms.")
            time.sleep(slice_time / 1000.0 * 0.1)
            current_time = end_time
            task['remaining_time'] -= slice_time
            # Increment virtual runtime inversely proportional to the task's priority (weight).
            task['vruntime'] += slice_time / task['priority']
            if task['remaining_time'] <= 0:
                self.logger(f"[CFS] Task {task['id']} finished at {current_time}ms.")
                tasks.remove(task)
        self.logger("[CFS] All tasks finished.\n")

# For quick testing in console:
if __name__ == '__main__':
    # Example: Run CFS simulation
    s = Scheduler(algorithm="CFS", time_quantum=100)
    s.run()
    print("Timeline:", s.timeline)
