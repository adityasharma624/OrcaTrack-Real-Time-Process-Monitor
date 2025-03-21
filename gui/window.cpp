#include "window.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <format>

Window::Window(const std::string& title, int width, int height, ProcessMonitor& monitor)
    : processMonitor(monitor)
    , showProcessTab(true)
    , showVisualizationTab(true)
    , showSettingsTab(false)
    , cpuHistory(120)  // 2 minutes of history at 1 second update
    , memoryHistory(120) {
    
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    initializeImGui();
}

Window::~Window() {
    cleanup();
}

void Window::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Set up ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 3.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void Window::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

void Window::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        processMonitor.update();

        // Update history
        cpuHistory.addValue(static_cast<float>(processMonitor.getTotalCPUUsage()));
        float memoryUsagePercent = static_cast<float>(processMonitor.getTotalMemoryUsage()) / 
                                 static_cast<float>(processMonitor.getTotalMemoryAvailable()) * 100.0f;
        memoryHistory.addValue(memoryUsagePercent);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        render();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void Window::render() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("Process Monitor", nullptr, 
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

    if (ImGui::BeginTabBar("##Tabs")) {
        if (ImGui::BeginTabItem("Processes")) {
            renderProcessTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visualization")) {
            renderVisualizationTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings")) {
            renderSettingsTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    renderAlerts();
    ImGui::End();
}

void Window::renderProcessTab() {
    auto processes = processMonitor.getProcessList();
    
    ImGui::BeginChild("ProcessList");
    if (ImGui::BeginTable("##processes", 5, 
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | 
        ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | 
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | 
        ImGuiTableFlags_BordersV | ImGuiTableFlags_ScrollY)) {
        
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("CPU %", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();

        for (const auto& process : processes) {
            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(process.name.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%lu", process.pid);

            ImGui::TableNextColumn();
            ImGui::Text("%.1f%%", process.cpuUsage);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f MB", static_cast<double>(process.memoryUsage) / (1024.0 * 1024.0));

            ImGui::TableNextColumn();
            if (ImGui::Button(std::format("Terminate##{}", process.pid).c_str())) {
                processMonitor.terminateProcess(process.pid);
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void Window::renderVisualizationTab() {
    ImGui::BeginChild("Visualization");
    
    // CPU Usage Graph
    ImGui::Text("CPU Usage");
    ImGui::PlotLines("##cpu", cpuHistory.values.data(), 
        static_cast<int>(cpuHistory.values.size()), 0, nullptr, 
        0.0f, 100.0f, ImVec2(-1, 150));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Memory Usage Graph
    ImGui::Text("Memory Usage");
    ImGui::PlotLines("##memory", memoryHistory.values.data(), 
        static_cast<int>(memoryHistory.values.size()), 0, nullptr, 
        0.0f, 100.0f, ImVec2(-1, 150));

    ImGui::EndChild();
}

void Window::renderSettingsTab() {
    ImGui::BeginChild("Settings");
    
    static double threshold = 90.0;
    static int timeout = 30;

    if (ImGui::SliderDouble("Usage Threshold (%)", &threshold, 50.0, 100.0, "%.1f")) {
        processMonitor.setUsageThreshold(threshold);
    }

    if (ImGui::SliderInt("Alert Timeout (seconds)", &timeout, 10, 300)) {
        processMonitor.setAlertTimeout(std::chrono::seconds(timeout));
    }

    ImGui::EndChild();
}

void Window::renderAlerts() {
    auto highUsageProcesses = processMonitor.getHighUsageProcesses();
    if (!highUsageProcesses.empty()) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 300, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(280, 0));
        ImGui::Begin("Alerts", nullptr, 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "High Resource Usage Detected!");
        ImGui::Separator();

        for (const auto& process : highUsageProcesses) {
            ImGui::Text("%s (PID: %lu)", process.name.c_str(), process.pid);
            ImGui::Text("CPU: %.1f%%, Memory: %.1f MB", 
                process.cpuUsage,
                static_cast<double>(process.memoryUsage) / (1024.0 * 1024.0));
            if (ImGui::Button(std::format("Terminate##{}", process.pid).c_str())) {
                processMonitor.terminateProcess(process.pid);
            }
            ImGui::Separator();
        }

        ImGui::End();
    }
}

void Window::ChartData::addValue(float value) {
    if (values.size() >= maxPoints) {
        values.erase(values.begin());
    }
    values.push_back(value);
} 