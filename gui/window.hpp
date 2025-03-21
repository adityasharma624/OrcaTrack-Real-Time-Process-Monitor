#pragma once
#include <string>
#include <memory>
#include <GLFW/glfw3.h>
#include "../monitor/process_monitor.hpp"

class Window {
public:
    Window(const std::string& title, int width, int height, ProcessMonitor& monitor);
    ~Window();

    void run();

private:
    std::string title;
    int width;
    int height;
    ProcessMonitor& monitor;
    GLFWwindow* window;

    // Sorting state
    struct {
        int columnIndex = 2;  // CPU usage column
        bool ascending = false;  // Descending order
    } sortState;

    void renderProcessTable();
    void sortProcessList(std::vector<ProcessInfo>& processes);
    
    // Helper functions for process management
    std::string getPriorityString(int priority);
    bool showConfirmationDialog(const std::string& title, const std::string& message);
    void showErrorDialog(const std::string& message);
}; 