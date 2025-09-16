from PyQt6.QtWidgets import QApplication, QLabel, QWidget, QVBoxLayout
import sys

def main():
    app = QApplication(sys.argv)
    window = QWidget()
    window.setWindowTitle("PyQt6 示例窗口")
    layout = QVBoxLayout()
    label = QLabel("Hello, PyQt6!")
    layout.addWidget(label)
    window.setLayout(layout)
    window.resize(300, 100)
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()