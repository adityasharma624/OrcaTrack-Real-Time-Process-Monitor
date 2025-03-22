#include "../gui/window.hpp"
#include "../monitor/process_monitor.hpp"
#include <iostream>
#include <stdexcept>
#include <windows.h>
#include <shellapi.h>

bool isElevated() {
    BOOL elevated = FALSE;
    HANDLE hToken = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            elevated = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return elevated;
}

int main(int argc, char* argv[]) {
    try {
        ProcessMonitor monitor;
        Window window("OrcaTrack - Process Monitor", 1280, 720, monitor);
        window.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
} 