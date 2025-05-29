#include "crashDebug.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include "enumDefinitions.h"


#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")
#endif

// Global crash handler state
static bool g_crashHandlerInstalled = false;
static std::string g_crashLogPath = "crash_log.txt";

// Function to write crash information to file
void writeCrashLog(const std::string& crashInfo) {
    std::ofstream logFile(g_crashLogPath, std::ios::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        logFile << "\n=== CRASH REPORT ===\n";
        logFile << "Time: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n";
        logFile << crashInfo << "\n";
        logFile << "==================\n\n";
        logFile.close();
        
        std::cout << "Crash information written to: " << g_crashLogPath << std::endl;
    }
}

// Log crash events function implementation
void logCrashEvent(const std::string& event, const std::string& details) {
    std::ofstream logFile(g_crashLogPath, std::ios::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        logFile << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] "
                << event << ": " << details << std::endl;
        logFile.flush();
        logFile.close();
    }
}

#ifdef _WIN32
// Windows-specific crash handler
LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    std::stringstream crashInfo;
    crashInfo << "Exception Code: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << "\n";
    crashInfo << "Exception Address: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionAddress << "\n";
    
    // Get stack trace
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    
    SymInitialize(process, NULL, TRUE);
    
    CONTEXT* context = exceptionInfo->ContextRecord;
    STACKFRAME64 stackFrame = {};
    
#ifdef _M_X64
    stackFrame.AddrPC.Offset = context->Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context->Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context->Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#else
    stackFrame.AddrPC.Offset = context->Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context->Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context->Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
#endif
    
    crashInfo << "\nStack Trace:\n";
    
    for (int frame = 0; frame < 20; frame++) {
        BOOL result = StackWalk64(
            machineType,
            process,
            thread,
            &stackFrame,
            context,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        );
        
        if (!result || stackFrame.AddrPC.Offset == 0) {
            break;
        }
        
        crashInfo << "  Frame " << frame << ": 0x" << std::hex << stackFrame.AddrPC.Offset;
        
        // Try to get symbol information
        DWORD64 displacement = 0;
        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        
        if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, symbol)) {
            crashInfo << " " << symbol->Name;
        }
        
        // Try to get line information
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisplacement = 0;
        
        if (SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &lineDisplacement, &line)) {
            crashInfo << " (" << line.FileName << ":" << line.LineNumber << ")";
        }
        
        crashInfo << "\n";
    }
    
    SymCleanup(process);
    
    // Get memory information
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        crashInfo << "\nMemory Usage:\n";
        crashInfo << "  Working Set: " << (pmc.WorkingSetSize / 1024 / 1024) << " MB\n";
        crashInfo << "  Peak Working Set: " << (pmc.PeakWorkingSetSize / 1024 / 1024) << " MB\n";
        crashInfo << "  Page File Usage: " << (pmc.PagefileUsage / 1024 / 1024) << " MB\n";
    }
    
    writeCrashLog(crashInfo.str());
    
    // Show message box
    std::string message = "Game crashed! Details saved to " + g_crashLogPath + 
                         "\n\nException: 0x" + 
                         std::to_string(exceptionInfo->ExceptionRecord->ExceptionCode);
    MessageBoxA(NULL, message.c_str(), "Game Crash", MB_OK | MB_ICONERROR);
    
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void installCrashHandler() {
    if (g_crashHandlerInstalled) {
        return;
    }
    
#ifdef _WIN32
    SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
    
    // Enable stack walking
    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(process, NULL, TRUE);
#endif
    
    g_crashHandlerInstalled = true;
    std::cout << "Crash handler installed - crash logs will be saved to: " << g_crashLogPath << std::endl;
}

void setCrashLogPath(const std::string& path) {
    g_crashLogPath = path;
}

// Memory debugging functions
void logMemoryUsage(const std::string& location) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        std::cout << "[MEMORY] " << location << ": " 
                  << (pmc.WorkingSetSize / 1024 / 1024) << " MB working set, "
                  << (pmc.PagefileUsage / 1024 / 1024) << " MB page file" << std::endl;
    }
#endif
}

// Thread-safe assertion with detailed logging
void debugAssert(bool condition, const char* expression, const char* file, int line, const char* message) {
    if (!condition) {
        std::stringstream assertInfo;
        assertInfo << "ASSERTION FAILED!\n";
        assertInfo << "Expression: " << expression << "\n";
        assertInfo << "File: " << file << " (line " << line << ")\n";
        assertInfo << "Message: " << (message ? message : "No additional message") << "\n";
        
        std::cerr << assertInfo.str() << std::endl;
        writeCrashLog(assertInfo.str());
        
#ifdef _WIN32
        if (IsDebuggerPresent()) {
            DebugBreak();
        } else {
            // Trigger an access violation to generate a crash dump
            *(int*)0 = 0;
        }
#else
        abort();
#endif
    }
}

// Validate pointer before use
bool isValidPointer(void* ptr) {
    if (!ptr) return false;
    
#ifdef _WIN32
    // Use IsBadReadPtr Windows API function for MinGW compatibility
    return !IsBadReadPtr(ptr, sizeof(char));
#else
    // Basic null check for non-Windows platforms
    return ptr != nullptr;
#endif
}
