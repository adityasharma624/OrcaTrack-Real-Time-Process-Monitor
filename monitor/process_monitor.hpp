#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>
#include <memory>
#include <windows.h>

struct ProcessInfo {
    DWORD pid;
    std::wstring name;
    double cpuUsage;
    double memoryUsage;
    bool isHighUsage;
    std::chrono::system_clock::time_point lastHighUsageTime;
    bool alertTriggered;  // New field to track if alert has been shown
    int highUsageCount;   // New field to count consecutive high usage readings
    int priority;  // Current process priority
    
    // CPU usage tracking
    FILETIME lastKernelTime;
    FILETIME lastUserTime;
    FILETIME lastUpdateTime;
    ULARGE_INTEGER prevSystemTime;
    ULARGE_INTEGER prevUserTime;
    ULARGE_INTEGER prevKernelTime;
};

// Forward declaration
class ProcessMonitorImpl;

class ProcessMonitor {
public:
    // Process priority levels matching Windows priorities
    enum class Priority {
        Idle = IDLE_PRIORITY_CLASS,
        BelowNormal = BELOW_NORMAL_PRIORITY_CLASS,
        Normal = NORMAL_PRIORITY_CLASS,
        AboveNormal = ABOVE_NORMAL_PRIORITY_CLASS,
        High = HIGH_PRIORITY_CLASS,
        RealTime = REALTIME_PRIORITY_CLASS
    };

    ProcessMonitor();
    ~ProcessMonitor();

    void update();
    const std::vector<ProcessInfo>& getProcesses() const;
    double getTotalCpuUsage() const;
    double getTotalMemoryUsage() const;
    size_t getTotalMemoryAvailable() const;
    
    // Enhanced process management functions
    bool terminateProcess(unsigned long pid);
    bool setPriority(unsigned long pid, Priority priority);
    bool suspendProcess(unsigned long pid);
    bool resumeProcess(unsigned long pid);
    std::wstring getProcessPath(unsigned long pid);
    bool isProcessElevated(unsigned long pid);
    bool canModifyProcess(unsigned long pid);
    
    // Alert settings
    void setUsageThreshold(double threshold) { usageThreshold = threshold; }
    void setAlertTimeout(std::chrono::seconds timeout) { alertTimeout = timeout; }
    void setHighUsageThresholds(double cpu, double mem) { 
        cpuAlertThreshold = cpu; 
        memoryAlertThreshold = mem; 
    }
    void setAlertTriggerCount(int count) { alertTriggerCount = count; }
    std::vector<ProcessInfo> getHighUsageProcesses() const;

private:
    struct ProcessTimes {
        ULARGE_INTEGER kernelTime;
        ULARGE_INTEGER userTime;
        ULARGE_INTEGER systemTime;
    };

    std::unique_ptr<ProcessMonitorImpl> pimpl;
    std::vector<ProcessInfo> processes;
    mutable std::mutex processMutex;
    double usageThreshold;
    std::chrono::seconds alertTimeout;
    double totalCpuUsage;
    double totalMemoryUsage;
    int numProcessors;
    
    // New alert thresholds
    double cpuAlertThreshold = 90.0;    // Alert if CPU > 90%
    double memoryAlertThreshold = 1024.0; // Alert if Memory > 1GB
    int alertTriggerCount = 5;           // Alert after 5 consecutive high readings
    
    // System time tracking
    FILETIME lastIdleTime;
    FILETIME lastKernelTime;
    FILETIME lastUserTime;
    FILETIME lastUpdateTime;

    // Process time tracking
    std::map<unsigned long, ProcessTimes> previousProcessTimes;

    void updateProcessInfo(ProcessInfo& info);
    void checkHighUsage(ProcessInfo& info);
    double calculateCpuUsage(const ProcessInfo& info);
    double calculateMemoryUsage(const ProcessInfo& info);
    void updateTotalCpuUsage();

    // Helper functions for process management
    HANDLE openProcessWithPrivileges(unsigned long pid, DWORD access) const;
    bool adjustProcessPrivileges();
    bool hasProcessPrivileges() const;
}; 