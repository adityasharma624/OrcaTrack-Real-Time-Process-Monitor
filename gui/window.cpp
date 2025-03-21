#include <GLFW/glfw3.h>
#include "window.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <format>
#include <iostream>
#include <codecvt>
#include <locale>
#include <chrono>
#include <thread>
#include <algorithm>

namespace {
    // Helper function to convert wide string to UTF-8
    std::string wstring_to_utf8(const std::wstring& wstr) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }

    // Helper function to format memory size
    std::string formatMemorySize(double mb) {
        if (mb >= 1024) {
            return std::format("{:.1f} GB", mb / 1024.0);
        }
        return std::format("{:.1f} MB", mb);
    }
}

Window::Window(const std::string& title, int width, int height, ProcessMonitor& monitor)
    : title(title)
    , width(width)
    , height(height)
    , monitor(monitor)
    , window(nullptr)
{
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

Window::~Window() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void Window::run() {
    using namespace std::chrono_literals;
    auto lastUpdateTime = std::chrono::steady_clock::now();
    const auto updateInterval = 1000ms; // Update every 1 second

    // Initialization phase
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Show initialization message
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)));
        ImGui::Begin("Initializing", nullptr, 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        
        ImGui::SetCursorPos(ImVec2(width/2 - 100, height/2 - 40));
        ImGui::Text("Initializing Process Monitor...");
        ImGui::SetCursorPos(ImVec2(width/2 - 150, height/2));
        ImGui::Text("Gathering initial system performance data...");
        
        ImGui::End();
        ImGui::Render();
        
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        // Take initial measurements
        monitor.update();
        std::this_thread::sleep_for(1000ms);
        monitor.update();
    }

    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::steady_clock::now();
        auto timeSinceLastUpdate = currentTime - lastUpdateTime;

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (timeSinceLastUpdate >= updateInterval) {
            monitor.update();
            lastUpdateTime = currentTime;
        }

        // Create the main window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)));
        ImGui::Begin("Process Monitor", nullptr, 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // System usage section
        ImGui::Text("System Usage");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Real-time system resource utilization");
            ImGui::BulletText("Updates every second");
            ImGui::BulletText("Monitors CPU and Memory usage");
            ImGui::BulletText("First few seconds may show higher values");
            ImGui::BulletText("while baseline measurements stabilize");
            ImGui::EndTooltip();
        }
        ImGui::Separator();

        float cpuUsage = static_cast<float>(monitor.getTotalCpuUsage());
        float memoryUsage = static_cast<float>(monitor.getTotalMemoryUsage());
        size_t totalMemory = monitor.getTotalMemoryAvailable();

        ImGui::ProgressBar(cpuUsage / 100.0f, ImVec2(-1, 0), 
            std::format("CPU Usage: {:.1f}%", cpuUsage).c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Total System CPU Usage: %.1f%%", cpuUsage);
            ImGui::Text("This includes:");
            ImGui::BulletText("All visible processes below");
            ImGui::BulletText("System processes & services");
            ImGui::BulletText("Background tasks & drivers");
            ImGui::BulletText("Kernel operations & interrupts");
            ImGui::EndTooltip();
        }
        ImGui::ProgressBar(memoryUsage / 100.0f, ImVec2(-1, 0), 
            (std::to_string(static_cast<int>(memoryUsage)) + "%% (" + 
             std::to_string(static_cast<int>(totalMemory / (1024.0 * 1024.0 * 1024.0))) + " GB available)").c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Memory Usage Statistics:");
            ImGui::BulletText("Used: %.1f%%", memoryUsage);
            ImGui::BulletText("Available: %d GB", static_cast<int>(totalMemory / (1024.0 * 1024.0 * 1024.0)));
            ImGui::BulletText("Includes cached files and standby memory");
            ImGui::EndTooltip();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Process list
        ImGui::Text("Processes");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Active Process List");
            ImGui::BulletText("Click column headers to sort");
            ImGui::BulletText("Shows processes using >0.01%% CPU or >1MB memory");
            ImGui::BulletText("Right-click process for more options");
            ImGui::EndTooltip();
        }
        ImGui::Separator();

        renderProcessTable();

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void Window::sortProcessList(std::vector<ProcessInfo>& processes) {
    if (sortState.columnIndex < 0) return;

    std::sort(processes.begin(), processes.end(),
        [this](const ProcessInfo& a, const ProcessInfo& b) {
            switch (sortState.columnIndex) {
                case 0: // Name
                    return sortState.ascending ? 
                        (wstring_to_utf8(a.name) < wstring_to_utf8(b.name)) :
                        (wstring_to_utf8(a.name) > wstring_to_utf8(b.name));
                case 1: // PID
                    return sortState.ascending ? (a.pid < b.pid) : (a.pid > b.pid);
                case 2: // CPU
                    return sortState.ascending ? (a.cpuUsage < b.cpuUsage) : (a.cpuUsage > b.cpuUsage);
                case 3: // Memory
                    return sortState.ascending ? (a.memoryUsage < b.memoryUsage) : (a.memoryUsage > b.memoryUsage);
                default:
                    return false;
            }
        });
}

void Window::renderProcessTable() {
    // Get processes and filter them
    auto processes = monitor.getProcesses();
    std::vector<ProcessInfo> activeProcesses;
    activeProcesses.reserve(processes.size());
    
    // Calculate total CPU usage from processes
    double totalProcessCpuUsage = 0.0;
    for (const auto& process : processes) {
        if (process.cpuUsage > 0.01 || process.memoryUsage > 1.0) {
            activeProcesses.push_back(process);
            totalProcessCpuUsage += process.cpuUsage;
        }
    }

    // Create table with sorting capabilities
    static ImGuiTableFlags flags = 
        ImGuiTableFlags_Resizable | 
        ImGuiTableFlags_Sortable | 
        ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_BordersOuter | 
        ImGuiTableFlags_BordersV |
        ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("ProcessTable", 4, flags)) {
        // Setup columns
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("PID");
        ImGui::TableSetupColumn("CPU %");
        ImGui::TableSetupColumn("Memory");
        ImGui::TableHeadersRow();

        // Add tooltips for column headers
        if (ImGui::TableGetHoveredColumn() >= 0) {
            switch (ImGui::TableGetHoveredColumn()) {
                case 0:
                    ImGui::BeginTooltip();
                    ImGui::Text("Process Name");
                    ImGui::BulletText("The name of the executable");
                    ImGui::BulletText("Click to sort alphabetically");
                    ImGui::EndTooltip();
                    break;
                case 1:
                    ImGui::BeginTooltip();
                    ImGui::Text("Process ID (PID)");
                    ImGui::BulletText("Unique identifier for each process");
                    ImGui::BulletText("Click to sort numerically");
                    ImGui::EndTooltip();
                    break;
                case 2:
                    ImGui::BeginTooltip();
                    ImGui::Text("CPU Usage Percentage");
                    ImGui::BulletText("Current CPU utilization");
                    ImGui::BulletText("Click to sort by CPU usage");
                    ImGui::EndTooltip();
                    break;
                case 3:
                    ImGui::BeginTooltip();
                    ImGui::Text("Memory Usage");
                    ImGui::BulletText("Current memory consumption");
                    ImGui::BulletText("Shows in MB or GB");
                    ImGui::BulletText("Click to sort by memory usage");
                    ImGui::EndTooltip();
                    break;
            }
        }

        // Sort specs
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs* sort_spec = &sort_specs->Specs[0];
                sortState.columnIndex = sort_spec->ColumnIndex;
                sortState.ascending = sort_spec->SortDirection == ImGuiSortDirection_Ascending;
                sort_specs->SpecsDirty = false;
            }
        }

        // Apply current sort
        sortProcessList(activeProcesses);

        // Display table contents
        for (const auto& process : activeProcesses) {
            ImGui::TableNextRow();
            
            // Set row background color for high usage processes
            if (process.isHighUsage) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 100, 100, 100));
            }
            
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(wstring_to_utf8(process.name).c_str());
            if (process.isHighUsage) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), " (!)");;
            }
            
            ImGui::TableNextColumn();
            ImGui::Text("%u", process.pid);
            
            ImGui::TableNextColumn();
            ImGui::Text("%.1f%%", process.cpuUsage);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("CPU Usage: %.1f%%", process.cpuUsage);
                if (process.cpuUsage > 90.0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                        "High CPU usage detected!");
                }
                ImGui::EndTooltip();
            }
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", formatMemorySize(process.memoryUsage).c_str());

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Memory: %s", formatMemorySize(process.memoryUsage).c_str());
                if (process.memoryUsage > 1024.0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                        "High memory usage detected!");
                }
                ImGui::Text("Right-click for options");
                ImGui::EndTooltip();
            }

            if (ImGui::BeginPopupContextItem(std::format("process_context_{}", process.pid).c_str())) {
                if (ImGui::MenuItem("Terminate Process")) {
                    monitor.terminateProcess(process.pid);
                }
                ImGui::EndPopup();
            }
        }

        // Add footer with total CPU usage information
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("Total CPU Usage from Listed Processes: %.1f%%", totalProcessCpuUsage);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("System Total: %.1f%%", monitor.getTotalCpuUsage());
            ImGui::Text("Listed Processes: %.1f%%", totalProcessCpuUsage);
            ImGui::Text("Difference may be due to system processes\nand background tasks not shown.");
            ImGui::EndTooltip();
        }

        ImGui::EndTable();
    }
} 