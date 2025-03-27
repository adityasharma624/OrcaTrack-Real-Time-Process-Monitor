# Real-Time Process Monitor

A Python-based process monitoring application that provides real-time information about system processes, CPU usage, and memory consumption. Built with Tkinter for the GUI and psutil for system monitoring.

## Features

- **Real-time System Monitoring**
  - Total CPU usage and availability
  - System memory usage
  - Auto-refreshing display (updates every 0.5 seconds)

- **Process Information**
  - Process ID (PID)
  - Process name
  - CPU usage percentage (normalized across all cores)
  - Memory usage in MB
  - Auto-sorting by CPU usage
  - Filtered display (excludes system idle processes)

- **User Interface**
  - Clean, modern interface
  - Sortable columns
  - Scrollable process list
  - Column separators for better readability
  - Bold headers and proper alignment

## Requirements

- Python 3.6 or higher
- psutil library

## Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/Real-Time-Process-Monitor.git
   cd Real-Time-Process-Monitor
   ```

2. Install the required dependencies:
   ```bash
   pip install -r requirements.txt
   ```

## Usage

Run the program:
```bash
python process_monitor.py
```

The application will open in a window showing:
- System information at the top (CPU and memory usage)
- A scrollable list of processes below, sorted by CPU usage
- The list automatically updates every 0.5 seconds

## Notes

- The CPU usage shown for each process is normalized across all CPU cores
- System idle processes are filtered out for clarity
- Some process information might not be available due to system permissions
- The application requires appropriate system permissions to access process information

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is licensed under the MIT License - see the LICENSE file for details. 