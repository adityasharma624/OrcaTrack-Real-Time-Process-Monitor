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
    , width(width * 2)
    , height(height + 50)
    , monitor(monitor)
    , showGroupSelector(false)
    , currentGroup(ProcessGroup::Default)
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
    // Get processes for current group
    auto processes = monitor.getProcessesByGroup(currentGroup);
    
    // Calculate total CPU usage from processes
    double totalProcessCpuUsage = 0.0;
    for (const auto& process : processes) {
        totalProcessCpuUsage += process.cpuUsage;
    }

    // Create table with sorting capabilities
    static ImGuiTableFlags flags = 
        ImGuiTableFlags_Resizable | 
        ImGuiTableFlags_Sortable | 
        ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_BordersOuter | 
        ImGuiTableFlags_BordersV |
        ImGuiTableFlags_ScrollY;

    // Add group selector button
    if (ImGui::Button("Select Group")) {
        showGroupSelector = !showGroupSelector;
    }
    ImGui::SameLine();
    ImGui::Text("Current Group: %s", getGroupName(currentGroup).c_str());

    if (showGroupSelector) {
        renderGroupSelector();
    }

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
        sortProcessList(processes);

        // Display table contents
        for (const auto& process : processes) {
            ImGui::TableNextRow();
            
            // Set row background color for high usage processes
            if (process.isHighUsage) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 100, 100, 100));
            }
            
            // Process Name
            ImGui::TableNextColumn();
            std::string label = wstring_to_utf8(process.name) + "##" + std::to_string(process.pid);
            bool selected = ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
            
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Process: %s", wstring_to_utf8(process.name).c_str());
                ImGui::Text("Path: %s", wstring_to_utf8(monitor.getProcessPath(process.pid)).c_str());
                ImGui::Text("Priority: %s", getPriorityString(process.priority).c_str());
                if (monitor.isProcessElevated(process.pid)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.0f, 1.0f), "Running with elevated privileges");
                }
                ImGui::EndTooltip();
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(("ProcessContextMenu_" + std::to_string(process.pid)).c_str());
            }
            
            // PID
            ImGui::TableNextColumn();
            ImGui::Text("%u", process.pid);
            
            // CPU Usage
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
            
            // Memory Usage
            ImGui::TableNextColumn();
            ImGui::Text("%s", formatMemorySize(process.memoryUsage).c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Memory: %s", formatMemorySize(process.memoryUsage).c_str());
                if (process.memoryUsage > 1024.0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                        "High memory usage detected!");
                }
                ImGui::EndTooltip();
            }

            // Context Menu
            if (ImGui::BeginPopup(("ProcessContextMenu_" + std::to_string(process.pid)).c_str())) {
                bool canModify = monitor.canModifyProcess(process.pid);
                
                ImGui::Text("%s (PID: %u)", wstring_to_utf8(process.name).c_str(), process.pid);
                ImGui::Separator();
                
                if (ImGui::MenuItem("Terminate Process", nullptr, false, canModify)) {
                    if (showConfirmationDialog("Terminate Process", 
                        std::format("Are you sure you want to terminate {}?", 
                        wstring_to_utf8(process.name)))) {
                        if (!monitor.terminateProcess(process.pid)) {
                            showErrorDialog("Failed to terminate process. Make sure you have sufficient privileges.");
                        }
                    }
                }

                if (ImGui::BeginMenu("Set Priority", canModify)) {
                    if (ImGui::MenuItem("Real Time", nullptr, process.priority == REALTIME_PRIORITY_CLASS)) {
                        monitor.setPriority(process.pid, ProcessMonitor::Priority::RealTime);
                    }
                    if (ImGui::MenuItem("High", nullptr, process.priority == HIGH_PRIORITY_CLASS)) {
                        monitor.setPriority(process.pid, ProcessMonitor::Priority::High);
                    }
                    if (ImGui::MenuItem("Above Normal", nullptr, process.priority == ABOVE_NORMAL_PRIORITY_CLASS)) {
                        monitor.setPriority(process.pid, ProcessMonitor::Priority::AboveNormal);
                    }
                    if (ImGui::MenuItem("Normal", nullptr, process.priority == NORMAL_PRIORITY_CLASS)) {
                        monitor.setPriority(process.pid, ProcessMonitor::Priority::Normal);
                    }
                    if (ImGui::MenuItem("Below Normal", nullptr, process.priority == BELOW_NORMAL_PRIORITY_CLASS)) {
                        monitor.setPriority(process.pid, ProcessMonitor::Priority::BelowNormal);
                    }
                    if (ImGui::MenuItem("Idle", nullptr, process.priority == IDLE_PRIORITY_CLASS)) {
                        monitor.setPriority(process.pid, ProcessMonitor::Priority::Idle);
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Suspend", nullptr, false, canModify)) {
                    if (!monitor.suspendProcess(process.pid)) {
                        showErrorDialog("Failed to suspend process. Make sure you have sufficient privileges.");
                    }
                }
                if (ImGui::MenuItem("Resume", nullptr, false, canModify)) {
                    if (!monitor.resumeProcess(process.pid)) {
                        showErrorDialog("Failed to resume process. Make sure you have sufficient privileges.");
                    }
                }

                if (!canModify) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                        "Insufficient privileges to modify this process");
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

void Window::renderGroupSelector() {
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Group Selector", &showGroupSelector);
    
    auto groupCounts = monitor.getProcessGroupCounts();

    // Add "Default" option at the top
    if (ImGui::Selectable("All Processes", currentGroup == ProcessGroup::Default)) {
        currentGroup = ProcessGroup::Default;
    }
    ImGui::SameLine();
    ImGui::Text("(%zu)", groupCounts[ProcessGroup::Default]);
    ImGui::Separator();

    // Add padding to ensure tab labels are fully visible
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    if (ImGui::BeginTabBar("GroupTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
        if (ImGui::BeginTabItem("Process Type")) {
            ImGui::Spacing();
            if (ImGui::Selectable("System Processes", currentGroup == ProcessGroup::SystemProcesses)) {
                currentGroup = ProcessGroup::SystemProcesses;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::SystemProcesses]);

            if (ImGui::Selectable("User Applications", currentGroup == ProcessGroup::UserApplications)) {
                currentGroup = ProcessGroup::UserApplications;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::UserApplications]);

            if (ImGui::Selectable("Background Services", currentGroup == ProcessGroup::BackgroundServices)) {
                currentGroup = ProcessGroup::BackgroundServices;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::BackgroundServices]);

            if (ImGui::Selectable("Windows Services", currentGroup == ProcessGroup::WindowsServices)) {
                currentGroup = ProcessGroup::WindowsServices;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::WindowsServices]);

            if (ImGui::Selectable("System Drivers", currentGroup == ProcessGroup::SystemDrivers)) {
                currentGroup = ProcessGroup::SystemDrivers;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::SystemDrivers]);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Resource Usage")) {
            ImGui::Spacing();
            if (ImGui::Selectable("High CPU Usage", currentGroup == ProcessGroup::HighCpuUsage)) {
                currentGroup = ProcessGroup::HighCpuUsage;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::HighCpuUsage]);

            if (ImGui::Selectable("High Memory Usage", currentGroup == ProcessGroup::HighMemoryUsage)) {
                currentGroup = ProcessGroup::HighMemoryUsage;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::HighMemoryUsage]);

            if (ImGui::Selectable("Low Resource Usage", currentGroup == ProcessGroup::LowResourceUsage)) {
                currentGroup = ProcessGroup::LowResourceUsage;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::LowResourceUsage]);

            if (ImGui::Selectable("Normal Resource Usage", currentGroup == ProcessGroup::NormalResourceUsage)) {
                currentGroup = ProcessGroup::NormalResourceUsage;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::NormalResourceUsage]);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Priority")) {
            ImGui::Spacing();
            if (ImGui::Selectable("Real-time Priority", currentGroup == ProcessGroup::RealTimePriority)) {
                currentGroup = ProcessGroup::RealTimePriority;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::RealTimePriority]);

            if (ImGui::Selectable("High Priority", currentGroup == ProcessGroup::HighPriority)) {
                currentGroup = ProcessGroup::HighPriority;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::HighPriority]);

            if (ImGui::Selectable("Above Normal", currentGroup == ProcessGroup::AboveNormalPriority)) {
                currentGroup = ProcessGroup::AboveNormalPriority;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::AboveNormalPriority]);

            if (ImGui::Selectable("Normal Priority", currentGroup == ProcessGroup::NormalPriority)) {
                currentGroup = ProcessGroup::NormalPriority;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::NormalPriority]);

            if (ImGui::Selectable("Below Normal", currentGroup == ProcessGroup::BelowNormalPriority)) {
                currentGroup = ProcessGroup::BelowNormalPriority;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::BelowNormalPriority]);

            if (ImGui::Selectable("Idle Priority", currentGroup == ProcessGroup::IdlePriority)) {
                currentGroup = ProcessGroup::IdlePriority;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::IdlePriority]);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Status")) {
            ImGui::Spacing();
            if (ImGui::Selectable("Running", currentGroup == ProcessGroup::Running)) {
                currentGroup = ProcessGroup::Running;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::Running]);

            if (ImGui::Selectable("Suspended", currentGroup == ProcessGroup::Suspended)) {
                currentGroup = ProcessGroup::Suspended;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::Suspended]);

            if (ImGui::Selectable("Elevated", currentGroup == ProcessGroup::Elevated)) {
                currentGroup = ProcessGroup::Elevated;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::Elevated]);

            if (ImGui::Selectable("System Protected", currentGroup == ProcessGroup::SystemProtected)) {
                currentGroup = ProcessGroup::SystemProtected;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::SystemProtected]);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Category")) {
            ImGui::Spacing();
            if (ImGui::Selectable("Microsoft Processes", currentGroup == ProcessGroup::MicrosoftProcesses)) {
                currentGroup = ProcessGroup::MicrosoftProcesses;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::MicrosoftProcesses]);

            if (ImGui::Selectable("Third-party Applications", currentGroup == ProcessGroup::ThirdPartyApplications)) {
                currentGroup = ProcessGroup::ThirdPartyApplications;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::ThirdPartyApplications]);

            if (ImGui::Selectable("Development Tools", currentGroup == ProcessGroup::DevelopmentTools)) {
                currentGroup = ProcessGroup::DevelopmentTools;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::DevelopmentTools]);

            if (ImGui::Selectable("System Services", currentGroup == ProcessGroup::SystemServices)) {
                currentGroup = ProcessGroup::SystemServices;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::SystemServices]);

            if (ImGui::Selectable("Background Tasks", currentGroup == ProcessGroup::BackgroundTasks)) {
                currentGroup = ProcessGroup::BackgroundTasks;
            }
            ImGui::SameLine();
            ImGui::Text("(%zu)", groupCounts[ProcessGroup::BackgroundTasks]);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::PopStyleVar();

    ImGui::End();
}

std::string Window::getGroupName(ProcessGroup group) const {
    switch (group) {
        case ProcessGroup::Default: return "All Processes";
        case ProcessGroup::SystemProcesses: return "System Processes";
        case ProcessGroup::UserApplications: return "User Applications";
        case ProcessGroup::BackgroundServices: return "Background Services";
        case ProcessGroup::WindowsServices: return "Windows Services";
        case ProcessGroup::SystemDrivers: return "System Drivers";
        case ProcessGroup::HighCpuUsage: return "High CPU Usage";
        case ProcessGroup::HighMemoryUsage: return "High Memory Usage";
        case ProcessGroup::LowResourceUsage: return "Low Resource Usage";
        case ProcessGroup::NormalResourceUsage: return "Normal Resource Usage";
        case ProcessGroup::RealTimePriority: return "Real-time Priority";
        case ProcessGroup::HighPriority: return "High Priority";
        case ProcessGroup::AboveNormalPriority: return "Above Normal Priority";
        case ProcessGroup::NormalPriority: return "Normal Priority";
        case ProcessGroup::BelowNormalPriority: return "Below Normal Priority";
        case ProcessGroup::IdlePriority: return "Idle Priority";
        case ProcessGroup::Running: return "Running";
        case ProcessGroup::Suspended: return "Suspended";
        case ProcessGroup::Elevated: return "Elevated";
        case ProcessGroup::SystemProtected: return "System Protected";
        case ProcessGroup::MicrosoftProcesses: return "Microsoft Processes";
        case ProcessGroup::ThirdPartyApplications: return "Third-party Applications";
        case ProcessGroup::DevelopmentTools: return "Development Tools";
        case ProcessGroup::SystemServices: return "System Services";
        case ProcessGroup::BackgroundTasks: return "Background Tasks";
        default: return "Unknown Group";
    }
}

std::string Window::getPriorityString(int priority) {
    switch (priority) {
        case REALTIME_PRIORITY_CLASS: return "Real Time";
        case HIGH_PRIORITY_CLASS: return "High";
        case ABOVE_NORMAL_PRIORITY_CLASS: return "Above Normal";
        case NORMAL_PRIORITY_CLASS: return "Normal";
        case BELOW_NORMAL_PRIORITY_CLASS: return "Below Normal";
        case IDLE_PRIORITY_CLASS: return "Idle";
        default: return "Unknown";
    }
}

bool Window::showConfirmationDialog(const std::string& title, const std::string& message) {
    bool confirmed = false;
    ImGui::OpenPopup(title.c_str());
    
    if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", message.c_str());
        ImGui::Separator();
        
        if (ImGui::Button("Yes", ImVec2(120, 0))) {
            confirmed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    return confirmed;
}

void Window::showErrorDialog(const std::string& message) {
    ImGui::OpenPopup("Error");
    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", message.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
} 