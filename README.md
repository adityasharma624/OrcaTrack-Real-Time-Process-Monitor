# OrcaTrack - Real-Time Process Monitor

A modern, feature-rich process monitoring tool built with C++ and ImGui. OrcaTrack provides real-time information about system processes, CPU usage, and memory consumption with a beautiful and intuitive interface.

## Features

- Real-time process monitoring with detailed information
- CPU and memory usage visualization
- Process management capabilities
- High resource usage alerts
- Customizable thresholds and alert settings
- Modern, responsive GUI with dark theme

## Prerequisites

- Windows 10 or later
- Visual Studio 2019/2022 with C++ desktop development workload
- CMake 3.15 or later
- Git

## Building from Source

1. Install required tools:
   ```powershell
   # Install Visual Studio 2022 Build Tools (if not already installed)
   # Download from: https://visualstudio.microsoft.com/visual-cpp-build-tools/
   # Select "Desktop development with C++"

   # Install CMake (if not already installed)
   winget install Kitware.CMake
   # or download from https://cmake.org/download/
   ```

2. Clone the repository and initialize submodules:
   ```powershell
   git clone https://github.com/yourusername/OrcaTrack.git
   cd OrcaTrack
   mkdir external
   cd external
   
   # Clone GLFW
   git clone https://github.com/glfw/glfw.git
   
   # Clone ImGui
   git clone https://github.com/ocornut/imgui.git
   cd imgui
   git checkout docking
   ```

3. Build the project:
   ```powershell
   cd ..
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

4. Run the application:
   ```powershell
   ./Release/orca_track.exe
   ```

## Usage

1. **Process List Tab**
   - View all running processes
   - Monitor CPU and memory usage per process
   - Terminate problematic processes

2. **Visualization Tab**
   - Real-time CPU usage graph
   - Real-time memory usage graph
   - Historical data visualization

3. **Settings Tab**
   - Adjust resource usage threshold
   - Configure alert timeout
   - Customize monitoring parameters

4. **Alerts**
   - Automatic notifications for high resource usage
   - Quick access to process termination
   - Configurable thresholds

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. 