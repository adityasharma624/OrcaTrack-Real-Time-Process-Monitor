#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>
#include <memory>

struct ProcessInfo {
    unsigned long pid;
    std::string name;
    double cpuUsage;
    size_t memoryUsage;
    std::chrono::system_clock::time_point lastHighUsageTime;
    bool isHighUsage;
};

// Forward declaration
class ProcessMonitorImpl;

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();

    void update();
    std::vector<ProcessInfo> getProcessList() const;
    double getTotalCPUUsage() const;
    size_t getTotalMemoryUsage() const;
    size_t getTotalMemoryAvailable() const;
    void terminateProcess(unsigned long pid);
    
    // Alert settings
    void setUsageThreshold(double threshold) { usageThreshold = threshold; }
    void setAlertTimeout(std::chrono::seconds timeout) { alertTimeout = timeout; }
    std::vector<ProcessInfo> getHighUsageProcesses() const;

private:
    std::unique_ptr<ProcessMonitorImpl> pimpl;
    std::map<unsigned long, ProcessInfo> processes;
    mutable std::mutex processMutex;
    double usageThreshold;
    std::chrono::seconds alertTimeout;
    double totalCpuUsage = 0.0;
    
    void updateProcessInfo(ProcessInfo& info);
    void checkHighUsage(ProcessInfo& info);
}; 