# Orca Track

A real-time process monitoring tool for Windows that provides detailed CPU and memory usage tracking for all running processes.

## Features

### Process Monitoring
- Real-time monitoring of all system processes
- CPU usage tracking with percentage display
- Memory usage monitoring in MB/GB
- System-wide resource usage overview
- Process filtering and sorting capabilities
- Modern ImGui-based user interface

### Process Management
- Process termination capability
- Process priority control (Idle, Below Normal, Normal, Above Normal, High, Real-time)
- Process suspension and resumption
- Process path and elevation status information

### Resource Monitoring
- Total system CPU utilization tracking
- Total and available memory monitoring
- Per-process resource usage statistics
- High resource usage detection and alerts

### Alert System
- Configurable CPU and memory usage thresholds
- Consecutive high usage detection
- Customizable alert timeout settings
- Process-specific alert tracking

### Security Features
- Process privilege verification
- Elevated process detection
- Secure process management with proper access controls

## Building from Source

### Prerequisites

- Visual Studio 2022 with C++ desktop development workload
- CMake 3.15 or higher
- Git

### Dependencies (automatically handled by CMake)

- Dear ImGui
- GLFW
- Windows SDK

### Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/[your-username]/Real-Time-Process-Monitor.git
cd Real-Time-Process-Monitor
```

2. Create build directory and generate project files:
```bash
mkdir build
cd build
cmake ..
```

3. Build the project:
```bash
cmake --build . --config Debug
```

The executable will be created at `build/Debug/orca_track.exe`

## Project Structure

- `gui/` - User interface implementation using Dear ImGui
  - Window management and UI rendering
  - Process list display and interaction
- `monitor/` - Core process monitoring functionality
  - Process information gathering
  - Resource usage calculation
  - Process management operations
- `external/` - Third-party dependencies
- `src/` - Main application entry point
- `CMakeLists.txt` - Build configuration

## Usage

### Basic Operations
- View all running processes with their resource usage
- Sort processes by CPU usage, memory usage, or name
- Monitor system-wide resource utilization

### Process Management
- Right-click on processes to access management options
- Change process priorities
- Suspend/Resume processes
- Terminate processes (requires appropriate privileges)

### Alert Configuration
- Set custom thresholds for CPU and memory alerts
- Configure alert timeout periods
- Monitor processes exceeding resource thresholds

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to your branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Dear ImGui](https://github.com/ocornut/imgui) for the amazing GUI library
- [GLFW](https://www.glfw.org/) for window management
- All contributors who have helped with this project 