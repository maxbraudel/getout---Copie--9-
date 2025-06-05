#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <iostream>

class PerformanceProfiler {
public:
    static PerformanceProfiler& getInstance() {
        static PerformanceProfiler instance;
        return instance;
    }
    
    struct Timer {
        std::chrono::high_resolution_clock::time_point start;
        std::string name;
        
        Timer(const std::string& timerName) : name(timerName) {
            start = std::chrono::high_resolution_clock::now();
        }
        
        ~Timer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            PerformanceProfiler::getInstance().addSample(name, duration.count());
        }
    };
    
    void addSample(const std::string& name, int64_t microseconds) {
        samples[name].push_back(microseconds);
        
        // Keep only the last 60 samples for each timer (about 1 second at 60Hz)
        if (samples[name].size() > 60) {
            samples[name].erase(samples[name].begin());
        }
    }
    
    void printReport() {
        std::cout << "\n=== PERFORMANCE REPORT ===" << std::endl;
        for (const auto& [name, timings] : samples) {
            if (timings.empty()) continue;
            
            int64_t total = 0;
            int64_t maxTime = 0;
            for (int64_t time : timings) {
                total += time;
                maxTime = std::max(maxTime, time);
            }
            
            double averageMs = (total / static_cast<double>(timings.size())) / 1000.0;
            double maxMs = maxTime / 1000.0;
            
            std::cout << name << ": avg=" << averageMs << "ms, max=" << maxMs << "ms" << std::endl;
        }
        std::cout << "========================\n" << std::endl;
    }
    
    void reset() {
        samples.clear();
    }

private:
    std::unordered_map<std::string, std::vector<int64_t>> samples;
};

// Convenient macro for creating scoped timers
#define PROFILE_SCOPE(name) PerformanceProfiler::Timer timer(name)
