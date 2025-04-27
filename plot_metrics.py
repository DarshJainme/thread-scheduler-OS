#!/usr/bin/env python3
import csv
import matplotlib.pyplot as plt

# Read data
algos = []
resp, tat, wait = [], [], []
with open('metrics.csv', newline='') as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        algos.append(row['Algorithm'])
        resp.append(float(row['Response']))
        tat.append(float(row['Turnaround']))
        wait.append(float(row['Waiting']))

# Positions for each group
x = range(len(algos))
width = 0.25

plt.figure(figsize=(10,6))
plt.bar([p - width for p in x], resp, width, label='Response')
plt.bar(x,               tat, width, label='Turnaround')
plt.bar([p + width for p in x], wait, width, label='Waiting')

plt.xticks(x, algos, rotation=45, ha='right')
plt.ylabel('Time (units)')
plt.title('Scheduler Algorithm Comparison')
plt.legend()
plt.tight_layout()
plt.savefig('metrics.png')   # so Qt can load it
# plt.show()  # if running interactively