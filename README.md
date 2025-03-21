# Orca Track

A real-time process monitoring tool for Windows that provides detailed CPU and memory usage tracking for all running processes.

## Features

- Real-time monitoring of all system processes
- CPU usage tracking with percentage display
- Memory usage monitoring in MB/GB
- System-wide resource usage overview
- Process filtering and sorting capabilities
- Modern ImGui-based user interface

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
- `monitor/` - Core process monitoring functionality
- `external/` - Third-party dependencies
- `CMakeLists.txt` - Build configuration

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to your branch
5. Create a Pull Request

## License

[Your chosen license]

## Version History

- v0.1.0-alpha: Initial release with basic process monitoring and resource tracking 