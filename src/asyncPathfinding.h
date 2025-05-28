#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <future>
#include <taskflow.hpp>
#include "entities.h"

// Forward declarations
class Map;

// Async pathfinding request structure (different from pathfinding.h)
struct AsyncPathfindingRequest {
    uint32_t requestId;
    std::string entityId;
    std::string instanceName;  // Instance name of the entity
    float startX, startY;
    float endX, endY;
    EntityConfiguration config;
    WalkType walkType = WalkType::NORMAL;
    std::chrono::steady_clock::time_point timestamp;    
    AsyncPathfindingRequest() = default;
};

// Async pathfinding result structure
struct AsyncPathfindingResult {
    uint32_t requestId;
    std::string entityId;
    std::string instanceName;  // Instance name of the entity
    std::vector<std::pair<float, float>> path;
    bool success;
    bool completed = false;
    bool failed = false;
    std::string errorMessage;
    WalkType walkType = WalkType::NORMAL;
    float targetX;
    float targetY;
    float computationTimeMs;
    
    AsyncPathfindingResult() = default;
};

// Async pathfinding manager using Taskflow efficiently
class AsyncEntityPathfinder {
public:
    AsyncEntityPathfinder(size_t numThreads = 4);
    ~AsyncEntityPathfinder();
    
    // Initialize with game map reference (MUST be called before start)
    void initialize(const Map* gameMap);
    
    // Start the async pathfinding system
    void start();
    
    // Stop the async pathfinding system
    void stop();
    
    // Request pathfinding (non-blocking) - creates individual Taskflow task
    uint32_t requestPathfinding(const std::string& entityId, 
                               float startX, float startY, 
                               float endX, float endY,
                               const EntityConfiguration& config,
                               WalkType walkType = WalkType::NORMAL);
    
    // Cancel pathfinding request for a specific entity
    bool cancelPathfindingRequest(const std::string& entityId);
    
    // Get completed pathfinding results (non-blocking)
    std::vector<AsyncPathfindingResult> getCompletedResults();
    
    // Check if entity has an active pathfinding request
    bool hasActiveRequest(const std::string& entityId) const;
    
    // Get queue statistics
    size_t getActiveRequestsCount() const;
    size_t getCompletedResultsCount() const;

private:
    // Taskflow executor for efficient task management
    tf::Executor executor;
    
    // Thread synchronization
    mutable std::mutex resultQueueMutex;
    mutable std::mutex activeRequestsMutex;
    mutable std::mutex activeTasksMutex;  // New mutex for task lifetime management
    mutable std::mutex stateMutex;
    mutable std::mutex gameMapMutex;
    
    // Result queue - completed pathfinding results
    std::queue<AsyncPathfindingResult> resultQueue;
    
    // Active request tracking
    std::unordered_map<std::string, uint32_t> activeRequests; // entityId -> requestId
    std::unordered_set<uint32_t> cancelledRequests;
      // Active tasks - keep futures alive for tracking async tasks
    std::unordered_map<uint32_t, std::future<void>> activeTasks; // requestId -> future
    
    // Request ID management
    std::atomic<uint32_t> nextRequestId;
    
    // System state
    std::atomic<bool> isRunning;
    
    // Game map reference (thread-safe read-only access)
    const Map* gameMapPtr;
    
    // Process individual pathfinding request as Taskflow task
    void processPathfindingTask(AsyncPathfindingRequest request);
};
