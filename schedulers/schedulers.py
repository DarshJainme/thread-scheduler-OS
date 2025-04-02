#!/usr/bin/env python3
import time

class Scheduler:
    def __init__(self, algorithm="RR", time_quantum=100, logger=None):
        """
        Parameters:
        - algorithm: "FCFS", "RR", or "PRIORITY"
        - time_quantum: time in milliseconds for each time slice (used in RR & Priority).
        - logger: Optional function to log text messages (defaults to print).
        
        Each task is defined with:
         - id: unique task identifier
         - priority: integer priority (higher = more urgent)
         - remaining_time: total CPU time needed by this task in ms
        """
        self.algorithm = algorithm.upper()
        self.time_quantum = time_quantum
        self.logger = logger or print  # fallback to print if no logger given

        # We'll store a "timeline" of (task_id, start_ms, end_ms) for building a Gantt chart later.
        self.timeline = []

        # Predefine different tasks & times for each algorithm for clarity.
        # You can modify these as desired.
        if self.algorithm == "FCFS":
            # Four tasks, same priority (ignored for FCFS), different CPU times
            self.tasks = [
                {'id': 1, 'priority': 1, 'remaining_time': 250},
                {'id': 2, 'priority': 1, 'remaining_time': 100},
                {'id': 3, 'priority': 1, 'remaining_time': 300},
                {'id': 4, 'priority': 1, 'remaining_time': 150},
            ]
        elif self.algorithm == "RR":
            # Four tasks, all same priority (irrelevant for RR), different CPU times
            self.tasks = [
                {'id': 1, 'priority': 1, 'remaining_time': 250},
                {'id': 2, 'priority': 1, 'remaining_time': 100},
                {'id': 3, 'priority': 1, 'remaining_time': 300},
                {'id': 4, 'priority': 1, 'remaining_time': 150},
            ]
        elif self.algorithm == "PRIORITY":
            # Four tasks, distinct priorities and CPU times
            self.tasks = [
                {'id': 1, 'priority': 15, 'remaining_time': 250},
                {'id': 2, 'priority': 5,  'remaining_time': 100},
                {'id': 3, 'priority': 20, 'remaining_time': 300},
                {'id': 4, 'priority': 10, 'remaining_time': 150},
            ]
        else:
            self.logger(f"Invalid algorithm: {self.algorithm}")
            self.tasks = []

    def run(self):
        """Run the selected scheduling algorithm and populate self.timeline with scheduling events."""
        if self.algorithm == "FCFS":
            self.run_fcfs()
        elif self.algorithm == "RR":
            self.run_rr()
        elif self.algorithm == "PRIORITY":
            self.run_priority()
        else:
            self.logger("Invalid algorithm specified.")

    def run_fcfs(self):
        """
        First-Come, First-Serve:
          - Run tasks strictly in the order they appear in self.tasks.
          - Each task runs to completion before the next starts.
        """
        self.logger("\n[FCFS] Starting FCFS scheduling.")
        current_time = 0
        for task in self.tasks:
            # The entire remaining_time is used in one continuous block.
            start_time = current_time
            run_duration = task['remaining_time']
            end_time = start_time + run_duration
            self.timeline.append((task['id'], start_time, end_time))
            
            self.logger(f"[FCFS] Task {task['id']} runs from {start_time} to {end_time} (needs {run_duration}ms).")
            
            # Simulate real passage of time (optional, for demonstration).
            time.sleep(run_duration / 1000.0 * 0.1)  # scale down actual sleep

            current_time = end_time  # move the clock
            self.logger(f"[FCFS] Task {task['id']} finished at time {current_time}ms.")

        self.logger("[FCFS] All tasks finished.\n")

    def run_rr(self):
        """
        Round Robin:
          - Cycle through tasks in a queue, giving each a time slice (time_quantum).
          - If a task still needs time after its slice, put it back in the queue.
          - Continue until all tasks are done.
        """
        self.logger("\n[RR] Starting Round Robin scheduling.")
        current_time = 0
        tasks = [dict(t) for t in self.tasks]  # copy tasks so we don't modify the original list

        cycle_count = 1
        while tasks:
            self.logger(f"[RR] -- Cycle {cycle_count} --")
            cycle_count += 1

            # We'll iterate over a snapshot of tasks. If tasks finish, remove them from 'tasks'.
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

                # Simulate partial passage of time
                time.sleep(run_duration / 1000.0 * 0.1)
                
                current_time = end_time

                # If finished, remove from tasks
                if task['remaining_time'] <= 0:
                    self.logger(f"[RR] Task {task['id']} finished at {current_time}ms.")
                    tasks.remove(task)

        self.logger("[RR] All tasks finished.\n")

    def run_priority(self):
        """
        Priority Scheduling:
          - Always pick the highest-priority task (largest priority value) for the next time slice.
          - Each selected task runs for up to time_quantum ms, then re-check priorities.
          - Continue until all tasks finish.
        """
        self.logger("\n[PRIORITY] Starting Priority scheduling.")
        current_time = 0
        tasks = [dict(t) for t in self.tasks]  # copy tasks

        while tasks:
            # Sort by descending priority
            tasks.sort(key=lambda x: -x['priority'])
            # The first task in sorted order is the highest priority
            task = tasks[0]

            start_time = current_time
            run_duration = min(self.time_quantum, task['remaining_time'])
            end_time = start_time + run_duration
            task['remaining_time'] -= run_duration
            self.timeline.append((task['id'], start_time, end_time))

            self.logger(f"[PRIORITY] Running Task {task['id']} from {start_time} to {end_time} (priority={task['priority']}).")
            
            time.sleep(run_duration / 1000.0 * 0.1)
            current_time = end_time

            # If finished, remove it
            if task['remaining_time'] <= 0:
                self.logger(f"[PRIORITY] Task {task['id']} finished at {current_time}ms.")
                tasks.remove(task)

        self.logger("[PRIORITY] All tasks finished.\n")

# For quick testing in console (optional):
if __name__ == '__main__':
    # Example: Priority
    s = Scheduler(algorithm="PRIORITY", time_quantum=100)
    s.run()
    print("Timeline:", s.timeline)
