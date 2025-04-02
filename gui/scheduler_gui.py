import sys
import os
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QPushButton, QVBoxLayout, QWidget,
    QTextEdit, QTabWidget, QHBoxLayout, QGraphicsView, QGraphicsScene, QGraphicsRectItem, QLabel, QSplitter
)
from PyQt5.QtCore import pyqtSignal, QThread, Qt, QRectF
from PyQt5.QtGui import QBrush, QPen, QColor

# Adjust path to locate schedulers.py
current_dir = os.path.dirname(os.path.abspath(__file__))
schedulers_dir = os.path.abspath(os.path.join(current_dir, '..', 'schedulers'))
sys.path.insert(0, schedulers_dir)

from schedulers import Scheduler

class GanttChartWidget(QGraphicsView):
    """
    A custom widget that draws a Gantt chart in a QGraphicsScene.
    Call set_timeline(timeline, algorithm_name) to update.
    """
    TASK_COLORS = {
        1: QColor("#ff9999"),  # red-ish
        2: QColor("#99ff99"),  # green-ish
        3: QColor("#9999ff"),  # blue-ish
        4: QColor("#ffff99"),  # yellow-ish
    }

    def __init__(self, parent=None):
        super().__init__(parent)
        self.scene = QGraphicsScene(self)
        self.setScene(self.scene)
        self.timeline = []
        self.algorithm_name = ""

    def set_timeline(self, timeline, algorithm_name):
        """
        timeline: list of (task_id, start_ms, end_ms)
        algorithm_name: string to label the chart
        """
        self.timeline = timeline
        self.algorithm_name = algorithm_name
        self.draw_chart()

    def draw_chart(self):
        self.scene.clear()
        if not self.timeline:
            return

        # Title at the top.
        title = QLabel(f"{self.algorithm_name} Gantt Chart")
        title_item = self.scene.addWidget(title)
        title_item.setPos(0, 0)

        offset_y = 30
        time_scale = 1.0  # 1ms = 1 pixel
        bar_height = 20

        # Assign each task its own row.
        task_rows = {}
        next_row = 0
        for (task_id, start_ms, end_ms) in self.timeline:
            if task_id not in task_rows:
                task_rows[task_id] = next_row
                next_row += 1

        # Draw each scheduled segment.
        for (task_id, start_ms, end_ms) in self.timeline:
            row = task_rows[task_id]
            top = offset_y + row * (bar_height + 10)
            left = start_ms * time_scale
            width = (end_ms - start_ms) * time_scale

            color = self.TASK_COLORS.get(task_id, QColor("#cccccc"))
            rect_item = self.scene.addRect(
                QRectF(left, top, width, bar_height),
                QPen(Qt.black),
                QBrush(color)
            )
            text_label = f"T{task_id}"
            text_item = self.scene.addSimpleText(text_label)
            text_item.setPos(left + width/2 - 5, top)

        max_time = max(e[2] for e in self.timeline) if self.timeline else 0
        self.scene.setSceneRect(0, 0, max_time * time_scale + 100, offset_y + (next_row * (bar_height + 10) + 50))


class QueueStateWidget(QTextEdit):
    """
    A widget to display queue state changes for MLQ and MLFQ.
    """
    def __init__(self, title, parent=None):
        super().__init__(parent)
        self.setReadOnly(True)
        self.setStyleSheet("background-color: #f0f0f0;")
        self.append(f"--- {title} Queue State ---\n")

    def update_state(self, msg):
        self.append(msg)


class SchedulerThread(QThread):
    log_signal = pyqtSignal(str)         # For textual logs
    gantt_signal = pyqtSignal(list, str)   # For timeline data
    queue_signal = pyqtSignal(str)         # For queue state updates (MLQ, MLFQ)

    def __init__(self, algorithm="RR", time_quantum=100, parent=None):
        super().__init__(parent)
        self.algorithm = algorithm.upper()
        self.time_quantum = time_quantum

    def run(self):
        # Create a Scheduler with a custom logger that emits log_signal.
        def logger(msg):
            self.log_signal.emit(msg)
            # For MLQ and MLFQ, emit extra signal for key queue state changes.
            if self.algorithm in ["MLQ", "MLFQ"]:
                if ("priority queue" in msg) or ("demoted" in msg):
                    self.queue_signal.emit(msg)
        sched = Scheduler(
            algorithm=self.algorithm,
            time_quantum=self.time_quantum,
            logger=logger
        )
        sched.run()
        self.gantt_signal.emit(sched.timeline, self.algorithm)


class SchedulerGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        # Threads for each scheduler algorithm.
        self.fcfs_thread = None
        self.rr_thread = None
        self.priority_thread = None
        self.sjf_thread = None
        self.mlq_thread = None
        self.mlfq_thread = None
        self.edf_thread = None
        self.cfs_thread = None

        self.init_ui()

    def init_ui(self):
        self.setWindowTitle("Scheduler Comparison GUI")
        self.setGeometry(100, 100, 1400, 900)
        
        central_widget = QWidget(self)
        main_layout = QVBoxLayout(central_widget)
        
        button_layout = QHBoxLayout()
        self.run_all_button = QPushButton("Run All Schedulers", self)
        self.run_all_button.clicked.connect(self.run_all_schedulers)
        button_layout.addWidget(self.run_all_button)
        main_layout.addLayout(button_layout)
        
        self.tab_widget = QTabWidget(self)

        # Log Tabs
        self.fcfs_text = QTextEdit(self); self.fcfs_text.setReadOnly(True)
        self.rr_text = QTextEdit(self); self.rr_text.setReadOnly(True)
        self.priority_text = QTextEdit(self); self.priority_text.setReadOnly(True)
        self.sjf_text = QTextEdit(self); self.sjf_text.setReadOnly(True)
        self.mlq_text = QTextEdit(self); self.mlq_text.setReadOnly(True)
        self.mlfq_text = QTextEdit(self); self.mlfq_text.setReadOnly(True)
        self.edf_text = QTextEdit(self); self.edf_text.setReadOnly(True)
        self.cfs_text = QTextEdit(self); self.cfs_text.setReadOnly(True)

        self.tab_widget.addTab(self.fcfs_text, "FCFS Log")
        self.tab_widget.addTab(self.rr_text, "RR Log")
        self.tab_widget.addTab(self.priority_text, "Priority Log")
        self.tab_widget.addTab(self.sjf_text, "SJF Log")
        self.tab_widget.addTab(self.mlq_text, "MLQ Log")
        self.tab_widget.addTab(self.mlfq_text, "MLFQ Log")
        self.tab_widget.addTab(self.edf_text, "EDF Log")
        self.tab_widget.addTab(self.cfs_text, "CFS Log")

        # Gantt Chart Tabs (for FCFS, RR, Priority, SJF, EDF, CFS)
        self.fcfs_gantt = GanttChartWidget()
        self.rr_gantt = GanttChartWidget()
        self.priority_gantt = GanttChartWidget()
        self.sjf_gantt = GanttChartWidget()
        self.edf_gantt = GanttChartWidget()
        self.cfs_gantt = GanttChartWidget()

        # For MLQ and MLFQ, we create a combined widget with both Gantt chart and Queue State view.
        self.mlq_gantt = GanttChartWidget()
        self.mlq_queue = QueueStateWidget("MLQ")
        self.mlfq_gantt = GanttChartWidget()
        self.mlfq_queue = QueueStateWidget("MLFQ")

        # Create container widgets for MLQ and MLFQ visualizations.
        self.mlq_container = QWidget()
        mlq_layout = QHBoxLayout(self.mlq_container)
        mlq_layout.addWidget(self.mlq_gantt)
        mlq_layout.addWidget(self.mlq_queue)

        self.mlfq_container = QWidget()
        mlfq_layout = QHBoxLayout(self.mlfq_container)
        mlfq_layout.addWidget(self.mlfq_gantt)
        mlfq_layout.addWidget(self.mlfq_queue)

        # Add tabs for Gantt charts.
        self.tab_widget.addTab(self.fcfs_gantt, "FCFS Gantt")
        self.tab_widget.addTab(self.rr_gantt, "RR Gantt")
        self.tab_widget.addTab(self.priority_gantt, "Priority Gantt")
        self.tab_widget.addTab(self.sjf_gantt, "SJF Gantt")
        self.tab_widget.addTab(self.mlq_container, "MLQ Visual")
        self.tab_widget.addTab(self.mlfq_container, "MLFQ Visual")
        self.tab_widget.addTab(self.edf_gantt, "EDF Gantt")
        self.tab_widget.addTab(self.cfs_gantt, "CFS Gantt")

        main_layout.addWidget(self.tab_widget)
        self.setCentralWidget(central_widget)

    def run_all_schedulers(self):
        # Clear previous logs and charts.
        for widget in [self.fcfs_text, self.rr_text, self.priority_text, self.sjf_text,
                       self.mlq_text, self.mlfq_text, self.edf_text, self.cfs_text]:
            widget.clear()
        for chart in [self.fcfs_gantt, self.rr_gantt, self.priority_gantt,
                      self.sjf_gantt, self.mlq_gantt, self.mlfq_gantt, self.edf_gantt, self.cfs_gantt]:
            chart.set_timeline([], chart.__class__.__name__)
        # Also clear queue state widgets.
        self.mlq_queue.clear()
        self.mlq_queue.append("--- MLQ Queue State ---\n")
        self.mlfq_queue.clear()
        self.mlfq_queue.append("--- MLFQ Queue State ---\n")

        # Create threads for each algorithm.
        self.fcfs_thread = SchedulerThread(algorithm="FCFS", time_quantum=100)
        self.rr_thread = SchedulerThread(algorithm="RR", time_quantum=100)
        self.priority_thread = SchedulerThread(algorithm="PRIORITY", time_quantum=100)
        self.sjf_thread = SchedulerThread(algorithm="SJF", time_quantum=100)
        self.mlq_thread = SchedulerThread(algorithm="MLQ", time_quantum=100)
        self.mlfq_thread = SchedulerThread(algorithm="MLFQ", time_quantum=100)
        self.edf_thread = SchedulerThread(algorithm="EDF", time_quantum=100)
        self.cfs_thread = SchedulerThread(algorithm="CFS", time_quantum=100)
        
        # Connect signals to log displays and Gantt updates.
        self.fcfs_thread.log_signal.connect(self.fcfs_text.append)
        self.fcfs_thread.gantt_signal.connect(self.update_fcfs_gantt)

        self.rr_thread.log_signal.connect(self.rr_text.append)
        self.rr_thread.gantt_signal.connect(self.update_rr_gantt)

        self.priority_thread.log_signal.connect(self.priority_text.append)
        self.priority_thread.gantt_signal.connect(self.update_priority_gantt)

        self.sjf_thread.log_signal.connect(self.sjf_text.append)
        self.sjf_thread.gantt_signal.connect(self.update_sjf_gantt)

        self.mlq_thread.log_signal.connect(self.mlq_text.append)
        self.mlq_thread.gantt_signal.connect(self.update_mlq_gantt)
        self.mlq_thread.queue_signal.connect(self.update_mlq_queue_state)

        self.mlfq_thread.log_signal.connect(self.mlfq_text.append)
        self.mlfq_thread.gantt_signal.connect(self.update_mlfq_gantt)
        self.mlfq_thread.queue_signal.connect(self.update_mlfq_queue_state)

        self.edf_thread.log_signal.connect(self.edf_text.append)
        self.edf_thread.gantt_signal.connect(self.update_edf_gantt)

        self.cfs_thread.log_signal.connect(self.cfs_text.append)
        self.cfs_thread.gantt_signal.connect(self.update_cfs_gantt)
        
        # Start all threads concurrently.
        self.fcfs_thread.start()
        self.rr_thread.start()
        self.priority_thread.start()
        self.sjf_thread.start()
        self.mlq_thread.start()
        self.mlfq_thread.start()
        self.edf_thread.start()
        self.cfs_thread.start()

    def update_fcfs_gantt(self, timeline, algorithm):
        self.fcfs_gantt.set_timeline(timeline, algorithm)

    def update_rr_gantt(self, timeline, algorithm):
        self.rr_gantt.set_timeline(timeline, algorithm)

    def update_priority_gantt(self, timeline, algorithm):
        self.priority_gantt.set_timeline(timeline, algorithm)

    def update_sjf_gantt(self, timeline, algorithm):
        self.sjf_gantt.set_timeline(timeline, algorithm)

    def update_mlq_gantt(self, timeline, algorithm):
        self.mlq_gantt.set_timeline(timeline, algorithm)

    def update_mlfq_gantt(self, timeline, algorithm):
        self.mlfq_gantt.set_timeline(timeline, algorithm)

    def update_edf_gantt(self, timeline, algorithm):
        self.edf_gantt.set_timeline(timeline, algorithm)

    def update_cfs_gantt(self, timeline, algorithm):
        self.cfs_gantt.set_timeline(timeline, algorithm)

    def update_mlq_queue_state(self, msg):
        self.mlq_queue.update_state(msg)

    def update_mlfq_queue_state(self, msg):
        self.mlfq_queue.update_state(msg)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = SchedulerGUI()
    window.show()
    sys.exit(app.exec_())
