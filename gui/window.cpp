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
        ImGui::Separator();

        float cpuUsage = static_cast<float>(monitor.getTotalCpuUsage());
        float memoryUsage = static_cast<float>(monitor.getTotalMemoryUsage());
        size_t totalMemory = monitor.getTotalMemoryAvailable();

        ImGui::ProgressBar(cpuUsage / 100.0f, ImVec2(-1, 0), 
            std::format("CPU Usage: {:.1f}%", cpuUsage).c_str());
        ImGui::ProgressBar(memoryUsage / 100.0f, ImVec2(-1, 0), 
            (std::to_string(static_cast<int>(memoryUsage)) + "%% (" + 
             std::to_string(static_cast<int>(totalMemory / (1024.0 * 1024.0 * 1024.0))) + " GB available)").c_str());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Process list
        ImGui::Text("Processes");
        ImGui::Separator();

        // Table headers
        ImGui::Columns(4, "ProcessColumns");
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("PID"); ImGui::NextColumn();
        ImGui::Text("CPU %"); ImGui::NextColumn();
        ImGui::Text("Memory"); ImGui::NextColumn();
        ImGui::Separator();

        // Process entries
        const auto& processes = monitor.getProcesses();
        for (const auto& process : processes) {
            if (process.cpuUsage > 0.01 || process.memoryUsage > 1.0) { // Filter out inactive processes
                ImGui::Text("%s", wstring_to_utf8(process.name).c_str()); ImGui::NextColumn();
                ImGui::Text("%u", process.pid); ImGui::NextColumn();
                ImGui::Text("%.1f%%", process.cpuUsage); ImGui::NextColumn();
                ImGui::Text("%s", formatMemorySize(process.memoryUsage).c_str()); ImGui::NextColumn();

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
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
        }
        ImGui::Columns(1);

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