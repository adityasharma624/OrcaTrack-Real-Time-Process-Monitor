#include "../gui/window.hpp"
#include "../monitor/process_monitor.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    try {
        ProcessMonitor monitor;
        Window window("OrcaTrack - Process Monitor", 1280, 720, monitor);
        window.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
} 