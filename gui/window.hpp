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
}; 