#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cstdio>
#include <csignal>

/**
 * Comprehensive crash debugging and memory safety utilities
 * This header provides tools to detect and handle various types of crashes
 */

// Crash debugging configuration
#define CRASH_DEBUG_ENABLED 1
#define CRASH_LOG_FILE "crash_debug.log"
#define MEMORY_DEBUG_ENABLED 1

// Memory bounds checking
template<typename T>
class SafeVector {
private:
    std::vector<T> data;
    
public:
    SafeVector() = default;
    SafeVector(size_t size) : data(size) {}
    SafeVector(size_t size, const T& value) : data(size, value) {}
    
    T& at(size_t index) {
        if (index >= data.size()) {
            std::cerr << "CRASH DEBUG: Vector bounds violation! Index " << index 
                      << " >= size " << data.size() << std::endl;
            logCrashEvent("Vector bounds violation", "Index: " + std::to_string(index) + 
                         ", Size: " + std::to_string(data.size()));
            throw std::out_of_range("Vector index out of bounds");
        }
        return data[index];
    }
    
    const T& at(size_t index) const {
        if (index >= data.size()) {
            std::cerr << "CRASH DEBUG: Vector bounds violation! Index " << index 
                      << " >= size " << data.size() << std::endl;
            logCrashEvent("Vector bounds violation (const)", "Index: " + std::to_string(index) + 
                         ", Size: " + std::to_string(data.size()));
            throw std::out_of_range("Vector index out of bounds");
        }
        return data[index];
    }
    
    T& operator[](size_t index) { return at(index); }
    const T& operator[](size_t index) const { return at(index); }
    
    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    void push_back(const T& value) { data.push_back(value); }
    void reserve(size_t capacity) { data.reserve(capacity); }
    void clear() { data.clear(); }
    
    typename std::vector<T>::iterator begin() { return data.begin(); }
    typename std::vector<T>::iterator end() { return data.end(); }
    typename std::vector<T>::const_iterator begin() const { return data.begin(); }
    typename std::vector<T>::const_iterator end() const { return data.end(); }
};

// Null pointer validation macros
#define VALIDATE_PTR(ptr, name) \
    do { \
        if ((ptr) == nullptr) { \
            std::cerr << "CRASH DEBUG: Null pointer detected: " << (name) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            logCrashEvent("Null pointer", std::string(name) + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
            return false; \
        } \
    } while(0)

#define VALIDATE_PTR_VOID(ptr, name) \
    do { \
        if ((ptr) == nullptr) { \
            std::cerr << "CRASH DEBUG: Null pointer detected: " << (name) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            logCrashEvent("Null pointer", std::string(name) + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
            return; \
        } \
    } while(0)

// Memory allocation tracking
class MemoryTracker {
private:
    static size_t totalAllocated;
    static size_t peakUsage;
    static size_t allocationCount;
    
public:
    static void recordAllocation(size_t size) {
        totalAllocated += size;
        allocationCount++;
        if (totalAllocated > peakUsage) {
            peakUsage = totalAllocated;
        }
        
        if (totalAllocated > 1024 * 1024 * 1024) { // 1GB threshold
            std::cerr << "CRASH DEBUG: High memory usage detected: " 
                      << (totalAllocated / (1024 * 1024)) << " MB" << std::endl;
            logCrashEvent("High memory usage", std::to_string(totalAllocated / (1024 * 1024)) + " MB");
        }
    }
    
    static void recordDeallocation(size_t size) {
        if (totalAllocated >= size) {
            totalAllocated -= size;
        } else {
            std::cerr << "CRASH DEBUG: Memory accounting error - negative allocation!" << std::endl;
            logCrashEvent("Memory accounting error", "Negative allocation detected");
        }
    }
    
    static void printStats() {
        std::cout << "Memory Statistics:" << std::endl;
        std::cout << "  Current usage: " << (totalAllocated / 1024) << " KB" << std::endl;
        std::cout << "  Peak usage: " << (peakUsage / 1024) << " KB" << std::endl;
        std::cout << "  Total allocations: " << allocationCount << std::endl;
    }
};

// Initialize static members
size_t MemoryTracker::totalAllocated = 0;
size_t MemoryTracker::peakUsage = 0;
size_t MemoryTracker::allocationCount = 0;

// Crash logging function
inline void logCrashEvent(const std::string& eventType, const std::string& details) {
    if (!CRASH_DEBUG_ENABLED) return;
    
    std::ofstream logFile(CRASH_LOG_FILE, std::ios::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        logFile << "[" << std::ctime(&time_t) << "] " 
                << eventType << ": " << details << std::endl;
        logFile.flush();
    }
}

// Signal handler for segmentation faults
inline void crashSignalHandler(int signal) {
    std::cerr << "CRASH DEBUG: Caught signal " << signal << std::endl;
    
    switch (signal) {
        case SIGSEGV:
            logCrashEvent("SIGSEGV", "Segmentation fault");
            std::cerr << "Segmentation fault detected!" << std::endl;
            break;
        case SIGABRT:
            logCrashEvent("SIGABRT", "Abort signal");
            std::cerr << "Abort signal detected!" << std::endl;
            break;
        case SIGFPE:
            logCrashEvent("SIGFPE", "Floating point exception");
            std::cerr << "Floating point exception detected!" << std::endl;
            break;
        default:
            logCrashEvent("Unknown signal", "Signal: " + std::to_string(signal));
            std::cerr << "Unknown signal detected: " << signal << std::endl;
            break;
    }
    
    // Print memory statistics before crashing
    MemoryTracker::printStats();
    
    // Re-raise the signal to get the default behavior (core dump, etc.)
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

// Initialize crash debugging
inline void initializeCrashDebugging() {
    if (!CRASH_DEBUG_ENABLED) return;
    
    // Set up signal handlers
    std::signal(SIGSEGV, crashSignalHandler);
    std::signal(SIGABRT, crashSignalHandler);
    std::signal(SIGFPE, crashSignalHandler);
    
    // Clear previous log
    std::ofstream logFile(CRASH_LOG_FILE);
    logFile << "=== Crash Debug Session Started ===" << std::endl;
    logFile.close();
    
    logCrashEvent("Session started", "Crash debugging initialized");
    std::cout << "Crash debugging enabled - logging to " << CRASH_LOG_FILE << std::endl;
}

// Bounds checking for array/vector access
template<typename Container>
inline bool validateIndex(const Container& container, size_t index, const std::string& containerName) {
    if (index >= container.size()) {
        std::cerr << "CRASH DEBUG: Index " << index << " out of bounds for " 
                  << containerName << " (size: " << container.size() << ")" << std::endl;
        logCrashEvent("Index out of bounds", containerName + " index: " + std::to_string(index) + 
                     ", size: " + std::to_string(container.size()));
        return false;
    }
    return true;
}

// OpenGL error checking
inline void checkOpenGLError(const std::string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "CRASH DEBUG: OpenGL error in " << operation << ": " << error << std::endl;
        logCrashEvent("OpenGL error", operation + " - Error code: " + std::to_string(error));
    }
}

// Thread safety validation
inline void validateThreadSafety(const std::string& functionName, bool isMainThread = false) {
    static std::thread::id mainThreadId = std::this_thread::get_id();
    std::thread::id currentThreadId = std::this_thread::get_id();
    
    if (isMainThread && currentThreadId != mainThreadId) {
        std::cerr << "CRASH DEBUG: Function " << functionName 
                  << " called from wrong thread!" << std::endl;
        logCrashEvent("Thread safety violation", functionName + " called from wrong thread");
    }
}
