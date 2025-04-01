import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QPushButton, QVBoxLayout, QWidget

class SchedulerGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.init_ui()

    def init_ui(self):
        self.setWindowTitle("User-Mode Thread Scheduler")
        self.setGeometry(100, 100, 800, 600)

        # Main Layout
        central_widget = QWidget(self)
        layout = QVBoxLayout(central_widget)

        # Start Scheduler Button
        self.start_button = QPushButton("Start Scheduling", self)
        self.start_button.clicked.connect(self.start_scheduler)
        layout.addWidget(self.start_button)

        # Set central widget
        self.setCentralWidget(central_widget)

    def start_scheduler(self):
        print("Scheduler Started!")  # Placeholder for now

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SchedulerGUI()
    window.show()
    sys.exit(app.exec_())
