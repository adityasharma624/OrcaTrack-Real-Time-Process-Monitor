#pragma once
#include "../monitor/process_monitor.hpp"
#include <string>
#include <vector>

struct GLFWwindow;

class Window {
public:
    Window(const std::string& title, int width, int height, ProcessMonitor& monitor);
    ~Window();

    void run();

private:
    struct ChartData {
        std::vector<float> values;
        size_t maxPoints;

        explicit ChartData(size_t max = 100) : maxPoints(max) {}
        void addValue(float value);
    };

    GLFWwindow* window;
    ProcessMonitor& processMonitor;
    bool showProcessTab;
    bool showVisualizationTab;
    bool showSettingsTab;
    ChartData cpuHistory;
    ChartData memoryHistory;

    void initializeImGui();
    void cleanup();
    void render();
    void renderProcessTab();
    void renderVisualizationTab();
    void renderSettingsTab();
    void renderAlerts();
}; 