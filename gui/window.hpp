#pragma once
#include "../monitor/process_monitor.hpp"
#include <string>
#include <memory>
#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_map>
#include <imgui.h>

struct GLFWwindow;

class Window {
public:
    Window(const std::string& title, int width, int height, ProcessMonitor& monitor);
    ~Window();

    void run();
    void toggleGroupSelector();
    void setCurrentGroup(ProcessGroup group);

private:
    void handleEvents();
    void update();
    void render();
    void renderProcessTable();
    void renderGroupSelector();
    void renderCategoryCounts();
    void renderHeader();
    void renderFooter();

    std::string title;
    int width;
    int height;
    ProcessMonitor& monitor;
    bool showGroupSelector;
    ProcessGroup currentGroup;
    GLFWwindow* window;

    // Sorting state
    struct {
        int columnIndex = -1;
        bool ascending = true;
    } sortState;

    std::string getGroupName(ProcessGroup group) const;
    void sortProcessList(std::vector<ProcessInfo>& processes);
    std::string getPriorityString(int priority);
    bool showConfirmationDialog(const std::string& title, const std::string& message);
    void showErrorDialog(const std::string& message);
}; 