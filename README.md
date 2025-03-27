# Process Monitor

A simple process monitoring application built with Python and Tkinter that displays real-time system and process information.

## Features

- Real-time CPU and memory usage monitoring
- List of all running processes
- Process information including:
  - Process ID (PID)
  - Process name
  - CPU usage percentage
  - Memory usage in MB
- Auto-sorting by CPU usage
- Auto-refresh every second

## Requirements

- Python 3.6 or higher
- psutil library

## Installation

1. Clone this repository or download the files
2. Install the required dependencies:
   ```
   pip install -r requirements.txt
   ```

## Usage

Run the program:
```
python process_monitor.py
```

The application will open in a window showing:
- System information (CPU and memory usage) at the top
- A scrollable list of processes below, sorted by CPU usage
- The list automatically updates every second

## Note

Some process information might not be available due to system permissions. This is normal and the application will skip those processes. 