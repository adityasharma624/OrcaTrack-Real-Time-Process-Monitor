import sys
from PyQt6.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QTableWidget, QTableWidgetItem
from PyQt6.QtCore import QTimer
from monitor import get_cpu_usage, get_memory_usage, get_processes

class Dashboard(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("OrcaTrack - Process Monitor")
        self.setGeometry(100, 100, 800, 600)

        # Layout
        self.layout = QVBoxLayout()
        
        # CPU and RAM Labels
        self.cpu_label = QLabel("CPU Usage: ")
        self.ram_label = QLabel("RAM Usage: ")
        self.layout.addWidget(self.cpu_label)
        self.layout.addWidget(self.ram_label)
        
        # Table for processes
        self.process_table = QTableWidget()
        self.process_table.setColumnCount(4)
        self.process_table.setHorizontalHeaderLabels(["PID", "Name", "CPU %", "Memory %"])
        self.layout.addWidget(self.process_table)
        
        self.setLayout(self.layout)
        
        # Timer for updating data
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_data)
        self.timer.start(5000)  # Update every 2 second
    
    def update_data(self):
        # Update CPU & RAM usage
        self.cpu_label.setText(f"CPU Usage: {get_cpu_usage()}%")
        memory = get_memory_usage()
        self.ram_label.setText(f"RAM Usage: {memory['percent']}%")
        
        # Update process table
        processes = get_processes()
        self.process_table.setRowCount(len(processes))
        for row, process in enumerate(processes):
            self.process_table.setItem(row, 0, QTableWidgetItem(str(process["pid"])))
            self.process_table.setItem(row, 1, QTableWidgetItem(process["name"]))
            self.process_table.setItem(row, 2, QTableWidgetItem(f"{process["cpu_percent"]:.2f}%"))
            self.process_table.setItem(row, 3, QTableWidgetItem(f"{process["memory_percent"]:.2f}%"))

if __name__ == "__main__":
    app = QApplication(sys.argv)
    dashboard = Dashboard()
    dashboard.show()
    sys.exit(app.exec())
