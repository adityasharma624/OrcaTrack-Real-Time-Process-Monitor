import sys
from PyQt6.QtWidgets import QApplication
from gui import Dashboard

if __name__ == "__main__":
    app = QApplication(sys.argv)
    dashboard = Dashboard()
    dashboard.show()
    sys.exit(app.exec())