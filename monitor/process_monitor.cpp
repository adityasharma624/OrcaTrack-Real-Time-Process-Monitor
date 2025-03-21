#include "process_monitor.hpp"
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <algorithm>
#include <system_error>
#include <tlhelp32.h>

class PDHQuery {
public:
    PDHQuery() {
        if (PdhOpenQuery(nullptr, 0, &query) != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to open PDH query");
        }
    }

    ~PDHQuery() {
        if (query) PdhCloseQuery(query);
    }

    void addCounter(const std::wstring& counterPath, PDH_HCOUNTER* counter) {
        if (PdhAddCounterW(query, counterPath.c_str(), 0, counter) != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to add counter");
        }
    }

    void collect() {
        if (PdhCollectQueryData(query) != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to collect query data");
        }
    }

private:
    PDH_HQUERY query = nullptr;
};

class ProcessMonitorImpl {
public:
    PDHQuery pdhQuery;
    PDH_HCOUNTER cpuCounter = nullptr;
    ULARGE_INTEGER lastCPU = {0};
    ULARGE_INTEGER lastSysCPU = {0};
    ULARGE_INTEGER lastUserCPU = {0};
    int numProcessors = 1;

    ProcessMonitorImpl() {
        // Initialize CPU counter
        pdhQuery.addCounter(L"\\Processor(_Total)\\% Processor Time", &cpuCounter);
        
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        numProcessors = sysInfo.dwNumberOfProcessors;
    }
};

ProcessMonitor::ProcessMonitor() 
    : usageThreshold(90.0)
    , alertTimeout(std::chrono::seconds(30)) {
    pimpl = std::make_unique<ProcessMonitorImpl>();
}

ProcessMonitor::~ProcessMonitor() = default;

void ProcessMonitor::update() {
    std::lock_guard<std::mutex> lock(processMutex);
    
    // Get process list
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        throw std::system_error(GetLastError(), std::system_category(), "Failed to create process snapshot");
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);

    std::map<unsigned long, ProcessInfo> newProcesses;
    
    if (Process32FirstW(snapshot, &pe32)) {
        do {
            ProcessInfo info;
            info.pid = pe32.th32ProcessID;
            info.name = std::string(pe32.szExeFile, pe32.szExeFile + wcslen(pe32.szExeFile));
            
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, info.pid);
            if (hProcess) {
                // Get memory info
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    info.memoryUsage = pmc.WorkingSetSize;
                }

                // Get CPU usage
                FILETIME createTime, exitTime, kernelTime, userTime;
                if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                    ULARGE_INTEGER kernelTimeValue, userTimeValue;
                    kernelTimeValue.LowPart = kernelTime.dwLowDateTime;
                    kernelTimeValue.HighPart = kernelTime.dwHighDateTime;
                    userTimeValue.LowPart = userTime.dwLowDateTime;
                    userTimeValue.HighPart = userTime.dwHighDateTime;

                    auto totalTime = (kernelTimeValue.QuadPart + userTimeValue.QuadPart) / 10000000.0;
                    info.cpuUsage = totalTime / pimpl->numProcessors;
                }

                CloseHandle(hProcess);
            }

            // Preserve high usage status and time if process existed before
            auto it = processes.find(info.pid);
            if (it != processes.end()) {
                info.isHighUsage = it->second.isHighUsage;
                info.lastHighUsageTime = it->second.lastHighUsageTime;
            }

            checkHighUsage(info);
            newProcesses[info.pid] = info;
        } while (Process32NextW(snapshot, &pe32));
    }

    CloseHandle(snapshot);
    processes = std::move(newProcesses);

    // Update total CPU usage
    pimpl->pdhQuery.collect();
    PDH_FMT_COUNTERVALUE counterVal;
    PdhGetFormattedCounterValue(pimpl->cpuCounter, PDH_FMT_DOUBLE, nullptr, &counterVal);
    totalCpuUsage = counterVal.doubleValue;
}

std::vector<ProcessInfo> ProcessMonitor::getProcessList() const {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<ProcessInfo> result;
    result.reserve(processes.size());
    for (const auto& pair : processes) {
        result.push_back(pair.second);
    }
    return result;
}

double ProcessMonitor::getTotalCPUUsage() const {
    return totalCpuUsage;
}

size_t ProcessMonitor::getTotalMemoryUsage() const {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullTotalPhys - memInfo.ullAvailPhys;
}

size_t ProcessMonitor::getTotalMemoryAvailable() const {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullTotalPhys;
}

void ProcessMonitor::terminateProcess(unsigned long pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        TerminateProcess(hProcess, 1);
        CloseHandle(hProcess);
    }
}

std::vector<ProcessInfo> ProcessMonitor::getHighUsageProcesses() const {
    std::lock_guard<std::mutex> lock(processMutex);
    std::vector<ProcessInfo> result;
    for (const auto& pair : processes) {
        if (pair.second.isHighUsage) {
            result.push_back(pair.second);
        }
    }
    return result;
}

void ProcessMonitor::checkHighUsage(ProcessInfo& info) {
    auto now = std::chrono::system_clock::now();
    
    if (info.cpuUsage > usageThreshold || 
        (info.memoryUsage > getTotalMemoryAvailable() * usageThreshold / 100.0)) {
        if (!info.isHighUsage) {
            info.isHighUsage = true;
            info.lastHighUsageTime = now;
        }
    } else {
        info.isHighUsage = false;
    }
} 