#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <csignal>
#include <vector>
#include <stdexcept>

/**
 * Comprehensive crash debugging and memory safety utilities
 * This header provides tools to detect and handle various types of crashes
 */

// Install crash handler to capture detailed crash information
void installCrashHandler();

// Set the path where crash logs will be saved
void setCrashLogPath(const std::string& path);

// Log current memory usage with a location identifier
void logMemoryUsage(const std::string& location);

// Thread-safe assertion with detailed logging
void debugAssert(bool condition, const char* expression, const char* file, int line, const char* message = nullptr);

// Validate if a pointer is safe to use
bool isValidPointer(void* ptr);

// Log crash events
void logCrashEvent(const std::string& event, const std::string& details);

// Convenience macros for debugging
#ifdef _DEBUG
    #define DEBUG_ASSERT(expr, msg) debugAssert((expr), #expr, __FILE__, __LINE__, msg)
    #define DEBUG_LOG_MEMORY(location) logMemoryUsage(location)
    #define DEBUG_VALIDATE_PTR(ptr) debugAssert(isValidPointer(ptr), #ptr " is invalid", __FILE__, __LINE__)
#else
    #define DEBUG_ASSERT(expr, msg) ((void)0)
    #define DEBUG_LOG_MEMORY(location) ((void)0)
    #define DEBUG_VALIDATE_PTR(ptr) ((void)0)
#endif

// Always active macros for release builds
#define RELEASE_ASSERT(expr, msg) debugAssert((expr), #expr, __FILE__, __LINE__, msg)
#define VALIDATE_PTR(ptr) isValidPointer(ptr)

// Safe vector template with bounds checking
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
