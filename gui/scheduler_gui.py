import sys
import os
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QPushButton, QVBoxLayout, QWidget,
    QTextEdit, QTabWidget, QHBoxLayout, QGraphicsView, QGraphicsScene, QGraphicsRectItem, QLabel
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
        # Add more if you have more task IDs
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

        # Label at the top
        title = QLabel(f"{self.algorithm_name} Gantt Chart")
        title_item = self.scene.addWidget(title)
        title_item.setPos(0, 0)

        # A small vertical offset below the title
        offset_y = 30

        # We'll scale time so that 1ms = 1 pixel (change if needed).
        time_scale = 1.0

        # The height of each task bar
        bar_height = 20

        # We'll track the "lanes" by task_id, so each task_id has its own Y offset.
        # But for a simpler chart, we can just stack them in chronological order.
        # However, a typical Gantt chart places each task on its own row.
        # Let's do that. We'll need to track the row for each task_id.
        # In this example, we have up to 4 tasks, so let's just do a small mapping:
        task_rows = {}
        next_row = 0

        for (task_id, start_ms, end_ms) in self.timeline:
            if task_id not in task_rows:
                task_rows[task_id] = next_row
                next_row += 1

        # Draw each event
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

            # Optionally add text on the rectangle
            text_label = f"T{task_id}"
            text_item = self.scene.addSimpleText(text_label)
            text_item.setPos(left + width/2 - 5, top)  # center-ish

        # Adjust the scene rect so everything is visible
        max_time = max(e[2] for e in self.timeline) if self.timeline else 0
        self.scene.setSceneRect(0, 0, max_time * time_scale + 100, offset_y + (next_row * (bar_height+10) + 50))

class SchedulerThread(QThread):
    log_signal = pyqtSignal(str)            # For textual logs
    gantt_signal = pyqtSignal(list, str)    # For timeline data

    def __init__(self, algorithm="RR", time_quantum=100, parent=None):
        super().__init__(parent)
        self.algorithm = algorithm
        self.time_quantum = time_quantum

    def run(self):
        # Create a Scheduler with a custom logger callback that emits log_signal.
        def logger(msg):
            self.log_signal.emit(msg)

        sched = Scheduler(
            algorithm=self.algorithm,
            time_quantum=self.time_quantum,
            logger=logger
        )
        sched.run()

        # After finishing, emit the timeline so the GUI can draw the Gantt chart.
        self.gantt_signal.emit(sched.timeline, self.algorithm)

class SchedulerGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.fcfs_thread = None
        self.rr_thread = None
        self.priority_thread = None

        self.init_ui()

    def init_ui(self):
        self.setWindowTitle("Scheduler Comparison GUI")
        self.setGeometry(100, 100, 1000, 600)
        
        # Main layout
        central_widget = QWidget(self)
        main_layout = QVBoxLayout(central_widget)
        
        # Button layout at the top
        button_layout = QHBoxLayout()
        self.run_all_button = QPushButton("Run All Schedulers", self)
        self.run_all_button.clicked.connect(self.run_all_schedulers)
        button_layout.addWidget(self.run_all_button)
        main_layout.addLayout(button_layout)
        
        # Tab widget for separate scheduler outputs
        self.tab_widget = QTabWidget(self)

        # 1) FCFS Tab
        self.fcfs_text = QTextEdit(self)
        self.fcfs_text.setReadOnly(True)
        fcfs_log_tab_index = self.tab_widget.addTab(self.fcfs_text, "FCFS Log")

        # 2) RR Tab
        self.rr_text = QTextEdit(self)
        self.rr_text.setReadOnly(True)
        rr_log_tab_index = self.tab_widget.addTab(self.rr_text, "RR Log")

        # 3) Priority Tab
        self.priority_text = QTextEdit(self)
        self.priority_text.setReadOnly(True)
        priority_log_tab_index = self.tab_widget.addTab(self.priority_text, "Priority Log")

        # 4) FCFS Gantt Chart
        self.fcfs_gantt = GanttChartWidget()
        fcfs_gantt_tab_index = self.tab_widget.addTab(self.fcfs_gantt, "FCFS Gantt")

        # 5) RR Gantt Chart
        self.rr_gantt = GanttChartWidget()
        rr_gantt_tab_index = self.tab_widget.addTab(self.rr_gantt, "RR Gantt")

        # 6) Priority Gantt Chart
        self.priority_gantt = GanttChartWidget()
        priority_gantt_tab_index = self.tab_widget.addTab(self.priority_gantt, "Priority Gantt")

        main_layout.addWidget(self.tab_widget)
        self.setCentralWidget(central_widget)

    def run_all_schedulers(self):
        # Clear previous logs
        self.fcfs_text.clear()
        self.rr_text.clear()
        self.priority_text.clear()

        # Clear Gantt charts
        self.fcfs_gantt.set_timeline([], "FCFS")
        self.rr_gantt.set_timeline([], "RR")
        self.priority_gantt.set_timeline([], "PRIORITY")
        
        # Create scheduler threads for each algorithm
        self.fcfs_thread = SchedulerThread(algorithm="FCFS", time_quantum=100)
        self.rr_thread = SchedulerThread(algorithm="RR", time_quantum=100)
        self.priority_thread = SchedulerThread(algorithm="PRIORITY", time_quantum=100)
        
        # Connect each thread's signals to the appropriate text edit & gantt chart
        self.fcfs_thread.log_signal.connect(self.fcfs_text.append)
        self.fcfs_thread.gantt_signal.connect(self.update_fcfs_gantt)

        self.rr_thread.log_signal.connect(self.rr_text.append)
        self.rr_thread.gantt_signal.connect(self.update_rr_gantt)

        self.priority_thread.log_signal.connect(self.priority_text.append)
        self.priority_thread.gantt_signal.connect(self.update_priority_gantt)
        
        # Start all threads concurrently
        self.fcfs_thread.start()
        self.rr_thread.start()
        self.priority_thread.start()

    def update_fcfs_gantt(self, timeline, algorithm):
        self.fcfs_gantt.set_timeline(timeline, algorithm)

    def update_rr_gantt(self, timeline, algorithm):
        self.rr_gantt.set_timeline(timeline, algorithm)

    def update_priority_gantt(self, timeline, algorithm):
        self.priority_gantt.set_timeline(timeline, algorithm)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = SchedulerGUI()
    window.show()
    sys.exit(app.exec_())
