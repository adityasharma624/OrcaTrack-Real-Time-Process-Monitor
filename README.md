# Real-Time Process Monitor

A Python-based process monitoring application that provides real-time information about system processes, CPU usage, and memory consumption. Built with Tkinter for the GUI and psutil for system monitoring.

## Features

- **Real-time System Monitoring**
  - Total CPU usage and availability
  - System memory usage with available memory indicator
  - Auto-refreshing display (updates every 0.5 seconds)
  - Visual CPU and memory usage graphs

- **Process Information**
  - Process ID (PID)
  - Process name
  - CPU usage percentage (normalized across all cores)
  - Memory usage in MB
  - Process health status (Stable, CPU Intensive, Memory Intensive, etc.)
  - Auto-sorting by any column
  - Process filtering and search

- **Visualization**
  - Real-time CPU usage graphs
  - Historical CPU trends
  - Top processes CPU usage tracking
  - Dark theme for better visibility

- **User Interface**
  - Clean, modern dark interface
  - Tabbed interface for processes and visualizations
  - Interactive progress bars
  - Sortable columns with click headers
  - Scrollable process list with proper sizing
  - Column separators for better readability
  - Bold headers and proper alignment

## Requirements

- Python 3.6 or higher
- psutil library
- matplotlib library
- numpy library

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

The application will open with the following features:
- **Process Tab**: View and manage running processes
  - System information at the top (CPU and memory usage)
  - Search bar for filtering processes
  - Scrollable list of processes with detailed information
  - Process status indication for identifying problematic processes

- **Visualization Tab**: View real-time CPU usage graphs
  - Overall system CPU utilization over time
  - Top 5 process CPU usage tracking
  - Color-coded process identification

## Advanced Features

- **Process Status Monitoring**:
  - Automatic detection of CPU-intensive processes
  - Memory usage tracking and alerting
  - Status categorization (Stable, CPU Intensive, Memory Intensive, etc.)
  - Historical resource usage tracking

- **Performance Optimizations**:
  - Efficient UI updates to reduce overhead
  - Smart data collection to minimize system impact
  - Throttled updates for better responsiveness

## Notes

- The CPU usage shown for each process is normalized across all CPU cores
- System idle processes are filtered out for clarity
- Process status is determined by analyzing resource usage patterns over time
- Some process information might not be available due to system permissions
- The application requires appropriate system permissions to access process information

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is licensed under the MIT License - see the LICENSE file for details.