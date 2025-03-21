#include "process_monitor.hpp"
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <algorithm>
#include <system_error>
#include <tlhelp32.h>
#include <sstream>
#include <iomanip>

// NT Status definitions
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

typedef LONG NTSTATUS;

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
    int numProcessors = 1;

    ProcessMonitorImpl() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        numProcessors = sysInfo.dwNumberOfProcessors;
    }
};

ProcessMonitor::ProcessMonitor() 
    : usageThreshold(80.0)
    , alertTimeout(std::chrono::seconds(300))
    , totalCpuUsage(0.0)
    , totalMemoryUsage(0.0)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;
    
    // Initialize system time tracking
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);
    lastIdleTime = idleTime;
    lastKernelTime = kernelTime;
    lastUserTime = userTime;
    GetSystemTimeAsFileTime(&lastUpdateTime);
}

ProcessMonitor::~ProcessMonitor() = default;

void ProcessMonitor::update() {
    std::lock_guard<std::mutex> lock(processMutex);
    processes.clear();

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(processEntry);

    if (Process32FirstW(snapshot, &processEntry)) {
        do {
            ProcessInfo info;
            info.pid = processEntry.th32ProcessID;
            info.name = processEntry.szExeFile;
            info.cpuUsage = 0.0;
            info.memoryUsage = 0.0;
            info.isHighUsage = false;
            
            updateProcessInfo(info);
            processes.push_back(info);
        } while (Process32NextW(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    updateTotalCpuUsage();
}

void ProcessMonitor::updateProcessInfo(ProcessInfo& info) {
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, info.pid);
    if (processHandle == nullptr) {
        return;
    }

    // Get process priority
    info.priority = GetPriorityClass(processHandle);
    if (info.priority == 0) {
        info.priority = NORMAL_PRIORITY_CLASS; // Default to normal if can't get priority
    }

    FILETIME createTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(processHandle, &createTime, &exitTime, &kernelTime, &userTime)) {
        info.lastKernelTime = kernelTime;
        info.lastUserTime = userTime;
        GetSystemTimeAsFileTime(&info.lastUpdateTime);

        // Convert FILETIME to ULARGE_INTEGER for calculations
        ULARGE_INTEGER kernelTimeValue, userTimeValue, systemTimeValue;
        kernelTimeValue.LowPart = kernelTime.dwLowDateTime;
        kernelTimeValue.HighPart = kernelTime.dwHighDateTime;
        userTimeValue.LowPart = userTime.dwLowDateTime;
        userTimeValue.HighPart = userTime.dwHighDateTime;
        systemTimeValue.LowPart = info.lastUpdateTime.dwLowDateTime;
        systemTimeValue.HighPart = info.lastUpdateTime.dwHighDateTime;

        // Get previous times for this process
        auto it = previousProcessTimes.find(info.pid);
        if (it != previousProcessTimes.end()) {
            info.prevKernelTime = it->second.kernelTime;
            info.prevUserTime = it->second.userTime;
            info.prevSystemTime = it->second.systemTime;
        } else {
            // First time seeing this process
            info.prevKernelTime = kernelTimeValue;
            info.prevUserTime = userTimeValue;
            info.prevSystemTime = systemTimeValue;
        }

        // Store current times for next update
        ProcessTimes times;
        times.kernelTime = kernelTimeValue;
        times.userTime = userTimeValue;
        times.systemTime = systemTimeValue;
        previousProcessTimes[info.pid] = times;

        info.cpuUsage = calculateCpuUsage(info);
    }

    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(processHandle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        info.memoryUsage = static_cast<double>(pmc.WorkingSetSize) / (1024 * 1024); // Convert to MB
    }

    // Check for high resource usage
    bool isHighCpu = info.cpuUsage > cpuAlertThreshold;
    bool isHighMemory = info.memoryUsage > memoryAlertThreshold;
    
    if (isHighCpu || isHighMemory) {
        info.highUsageCount++;
        if (info.highUsageCount >= alertTriggerCount && !info.alertTriggered) {
            info.isHighUsage = true;
            info.lastHighUsageTime = std::chrono::system_clock::now();
            info.alertTriggered = true;
        }
    } else {
        info.highUsageCount = 0;
        info.isHighUsage = false;
        info.alertTriggered = false;
    }

    CloseHandle(processHandle);
}

double ProcessMonitor::calculateCpuUsage(const ProcessInfo& info) {
    ULARGE_INTEGER currentSystemTime, currentUserTime, currentKernelTime;
    currentSystemTime.LowPart = info.lastUpdateTime.dwLowDateTime;
    currentSystemTime.HighPart = info.lastUpdateTime.dwHighDateTime;
    currentUserTime.LowPart = info.lastUserTime.dwLowDateTime;
    currentUserTime.HighPart = info.lastUserTime.dwHighDateTime;
    currentKernelTime.LowPart = info.lastKernelTime.dwLowDateTime;
    currentKernelTime.HighPart = info.lastKernelTime.dwHighDateTime;

    ULONGLONG userDiff = currentUserTime.QuadPart - info.prevUserTime.QuadPart;
    ULONGLONG kernelDiff = currentKernelTime.QuadPart - info.prevKernelTime.QuadPart;
    ULONGLONG totalDiff = userDiff + kernelDiff;
    ULONGLONG timeDiff = currentSystemTime.QuadPart - info.prevSystemTime.QuadPart;

    if (timeDiff == 0) return 0.0;

    // Convert from 100-nanosecond intervals to percentage
    // Divide by number of processors to get correct percentage
    double cpuUsage = (totalDiff * 100.0) / (timeDiff * numProcessors);
    
    // Clamp the value between 0 and 100 to match Task Manager's behavior
    return std::clamp(cpuUsage, 0.0, 100.0);
}

double ProcessMonitor::calculateMemoryUsage(const ProcessInfo& info) {
    return info.memoryUsage;
}

void ProcessMonitor::updateTotalCpuUsage() {
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);

    ULARGE_INTEGER currentIdle, currentKernel, currentUser;
    currentIdle.LowPart = idleTime.dwLowDateTime;
    currentIdle.HighPart = idleTime.dwHighDateTime;
    currentKernel.LowPart = kernelTime.dwLowDateTime;
    currentKernel.HighPart = kernelTime.dwHighDateTime;
    currentUser.LowPart = userTime.dwLowDateTime;
    currentUser.HighPart = userTime.dwHighDateTime;

    ULARGE_INTEGER lastIdle, lastKernel, lastUser;
    lastIdle.LowPart = lastIdleTime.dwLowDateTime;
    lastIdle.HighPart = lastIdleTime.dwHighDateTime;
    lastKernel.LowPart = lastKernelTime.dwLowDateTime;
    lastKernel.HighPart = lastKernelTime.dwHighDateTime;
    lastUser.LowPart = lastUserTime.dwLowDateTime;
    lastUser.HighPart = lastUserTime.dwHighDateTime;

    ULONGLONG idleDiff = currentIdle.QuadPart - lastIdle.QuadPart;
    ULONGLONG kernelDiff = currentKernel.QuadPart - lastKernel.QuadPart;
    ULONGLONG userDiff = currentUser.QuadPart - lastUser.QuadPart;
    ULONGLONG totalDiff = kernelDiff + userDiff;
    ULONGLONG activeDiff = totalDiff - idleDiff;

    if (totalDiff > 0) {
        // Calculate total CPU usage across all cores
        totalCpuUsage = (activeDiff * 100.0) / totalDiff;
    } else {
        totalCpuUsage = 0.0;
    }

    // Update stored times for next calculation
    lastIdleTime = idleTime;
    lastKernelTime = kernelTime;
    lastUserTime = userTime;

    // Update total memory usage
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        totalMemoryUsage = static_cast<double>(memInfo.dwMemoryLoad);
    }
}

const std::vector<ProcessInfo>& ProcessMonitor::getProcesses() const {
    return processes;
}

double ProcessMonitor::getTotalCpuUsage() const {
    return totalCpuUsage;
}

double ProcessMonitor::getTotalMemoryUsage() const {
    return totalMemoryUsage;
}

size_t ProcessMonitor::getTotalMemoryAvailable() const {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        // Return available physical memory in bytes
        return static_cast<size_t>(memInfo.ullAvailPhys);
    }
    return 0;
}

bool ProcessMonitor::adjustProcessPrivileges() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    CloseHandle(hToken);
    return result;
}

HANDLE ProcessMonitor::openProcessWithPrivileges(unsigned long pid, DWORD access) const {
    HANDLE hProcess = OpenProcess(access, FALSE, pid);
    if (!hProcess) {
        // Try to get debug privileges and retry
        const_cast<ProcessMonitor*>(this)->adjustProcessPrivileges();
        hProcess = OpenProcess(access, FALSE, pid);
    }
    return hProcess;
}

bool ProcessMonitor::hasProcessPrivileges() const {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return false;
    }

    PRIVILEGE_SET privs;
    privs.PrivilegeCount = 1;
    privs.Control = PRIVILEGE_SET_ALL_NECESSARY;
    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &privs.Privilege[0].Luid);

    BOOL hasPrivilege;
    bool result = PrivilegeCheck(hToken, &privs, &hasPrivilege);
    CloseHandle(hToken);
    return result && hasPrivilege;
}

bool ProcessMonitor::terminateProcess(unsigned long pid) {
    HANDLE hProcess = openProcessWithPrivileges(pid, PROCESS_TERMINATE);
    if (!hProcess) {
        return false;
    }

    bool result = TerminateProcess(hProcess, 1) != 0;
    CloseHandle(hProcess);
    return result;
}

bool ProcessMonitor::setPriority(unsigned long pid, Priority priority) {
    HANDLE hProcess = openProcessWithPrivileges(pid, PROCESS_SET_INFORMATION);
    if (!hProcess) {
        return false;
    }

    bool result = SetPriorityClass(hProcess, static_cast<DWORD>(priority)) != 0;
    CloseHandle(hProcess);
    return result;
}

bool ProcessMonitor::suspendProcess(unsigned long pid) {
    HANDLE hProcess = openProcessWithPrivileges(pid, PROCESS_SUSPEND_RESUME);
    if (!hProcess) {
        return false;
    }

    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        CloseHandle(hProcess);
        return false;
    }

    using NtSuspendProcessFn = NTSTATUS(NTAPI*)(HANDLE ProcessHandle);
    NtSuspendProcessFn pfnNtSuspendProcess = nullptr;
    
    try {
        pfnNtSuspendProcess = reinterpret_cast<NtSuspendProcessFn>(
            GetProcAddress(hNtdll, "NtSuspendProcess"));
        
        if (!pfnNtSuspendProcess) {
            CloseHandle(hProcess);
            return false;
        }

        NTSTATUS status = pfnNtSuspendProcess(hProcess);
        CloseHandle(hProcess);
        return NT_SUCCESS(status);
    }
    catch (...) {
        CloseHandle(hProcess);
        return false;
    }
}

bool ProcessMonitor::resumeProcess(unsigned long pid) {
    HANDLE hProcess = openProcessWithPrivileges(pid, PROCESS_SUSPEND_RESUME);
    if (!hProcess) {
        return false;
    }

    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        CloseHandle(hProcess);
        return false;
    }

    using NtResumeProcessFn = NTSTATUS(NTAPI*)(HANDLE ProcessHandle);
    NtResumeProcessFn pfnNtResumeProcess = nullptr;
    
    try {
        pfnNtResumeProcess = reinterpret_cast<NtResumeProcessFn>(
            GetProcAddress(hNtdll, "NtResumeProcess"));
        
        if (!pfnNtResumeProcess) {
            CloseHandle(hProcess);
            return false;
        }

        NTSTATUS status = pfnNtResumeProcess(hProcess);
        CloseHandle(hProcess);
        return NT_SUCCESS(status);
    }
    catch (...) {
        CloseHandle(hProcess);
        return false;
    }
}

std::wstring ProcessMonitor::getProcessPath(unsigned long pid) {
    HANDLE hProcess = openProcessWithPrivileges(pid, PROCESS_QUERY_LIMITED_INFORMATION);
    if (!hProcess) {
        return L"";
    }

    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    bool result = QueryFullProcessImageNameW(hProcess, 0, path, &size) != 0;
    CloseHandle(hProcess);

    return result ? std::wstring(path) : L"";
}

bool ProcessMonitor::isProcessElevated(unsigned long pid) {
    HANDLE hProcess = openProcessWithPrivileges(pid, PROCESS_QUERY_LIMITED_INFORMATION);
    if (!hProcess) {
        return false;
    }

    HANDLE hToken;
    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        CloseHandle(hProcess);
        return false;
    }

    TOKEN_ELEVATION elevation;
    DWORD size;
    bool result = GetTokenInformation(hToken, TokenElevation, &elevation,
        sizeof(elevation), &size) != 0;

    CloseHandle(hToken);
    CloseHandle(hProcess);
    return result && elevation.TokenIsElevated;
}

bool ProcessMonitor::canModifyProcess(unsigned long pid) {
    HANDLE hProcess = openProcessWithPrivileges(pid, PROCESS_QUERY_LIMITED_INFORMATION);
    if (!hProcess) {
        return false;
    }

    CloseHandle(hProcess);
    return true;
}

std::vector<ProcessInfo> ProcessMonitor::getHighUsageProcesses() const {
    std::vector<ProcessInfo> highUsageProcesses;
    for (const auto& process : processes) {
        if (process.isHighUsage) {
            // Check if the alert timeout has passed
            auto now = std::chrono::system_clock::now();
            if (now - process.lastHighUsageTime < alertTimeout) {
                highUsageProcesses.push_back(process);
            }
        }
    }
    return highUsageProcesses;
} 