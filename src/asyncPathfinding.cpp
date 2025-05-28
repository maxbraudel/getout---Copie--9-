
// Define M_PI early to avoid compilation issues with third-party libraries
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#include "asyncPathfinding.h"
#include "map.h"
#include <iostream>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declaration of findPath function from pathfinding.cpp
extern std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    const EntityConfiguration& entityConfig
);

AsyncEntityPathfinder::AsyncEntityPathfinder(size_t numThreads) 
    : executor(numThreads), nextRequestId(1), isRunning(false) {
    std::cout << "AsyncEntityPathfinder initialized with " << numThreads << " threads" << std::endl;
}

AsyncEntityPathfinder::~AsyncEntityPathfinder() {
    stop();
}

void AsyncEntityPathfinder::start() {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (!isRunning) {
        isRunning = true;
        startBackgroundProcessing();
        std::cout << "AsyncEntityPathfinder started" << std::endl;
    }
}

void AsyncEntityPathfinder::stop() {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (isRunning) {
        isRunning = false;
        // Cancel all pending requests
        {
            std::lock_guard<std::mutex> requestLock(requestQueueMutex);
            while (!requestQueue.empty()) {
                requestQueue.pop();
            }
        }
        std::cout << "AsyncEntityPathfinder stopped" << std::endl;
    }
}

uint32_t AsyncEntityPathfinder::requestPathfinding(const std::string& entityId, 
                                                   float startX, float startY, 
                                                   float endX, float endY,
                                                   const EntityConfiguration& config,
                                                   WalkType walkType) {
    if (!isRunning) {
        std::cerr << "AsyncEntityPathfinder is not running!" << std::endl;
        return 0; // Invalid request ID
    }    uint32_t requestId = nextRequestId++;
    
    AsyncPathfindingRequest request;
    request.requestId = requestId;
    request.entityId = entityId;
    request.instanceName = entityId; // For now, assume entityId is the instanceName
    request.startX = startX;
    request.startY = startY;
    request.endX = endX;
    request.endY = endY;
    request.config = config;
    request.walkType = walkType;
    request.timestamp = std::chrono::steady_clock::now();
    
    // Add request to queue
    {
        std::lock_guard<std::mutex> lock(requestQueueMutex);
        requestQueue.push(request);
    }
    
    // Track active request for this entity (for cancellation)
    {
        std::lock_guard<std::mutex> lock(activeRequestsMutex);
        auto it = activeRequests.find(entityId);
        if (it != activeRequests.end()) {
            // Cancel previous request for this entity
            cancelledRequests.insert(it->second);
        }
        activeRequests[entityId] = requestId;
    }
    
    std::cout << "Pathfinding request " << requestId << " queued for entity " << entityId 
              << " from (" << startX << ", " << startY << ") to (" << endX << ", " << endY << ")" << std::endl;
    
    return requestId;
}

bool AsyncEntityPathfinder::cancelPathfindingRequest(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(activeRequestsMutex);
    auto it = activeRequests.find(entityId);
    if (it != activeRequests.end()) {
        cancelledRequests.insert(it->second);
        activeRequests.erase(it);
        std::cout << "Cancelled pathfinding request for entity " << entityId << std::endl;
        return true;
    }
    return false;
}

std::vector<AsyncPathfindingResult> AsyncEntityPathfinder::getCompletedResults() {
    std::vector<AsyncPathfindingResult> results;
    
    std::lock_guard<std::mutex> lock(resultQueueMutex);
    while (!resultQueue.empty()) {
        results.push_back(std::move(resultQueue.front()));
        resultQueue.pop();
    }
    
    return results;
}

bool AsyncEntityPathfinder::hasActiveRequest(const std::string& entityId) const {
    std::lock_guard<std::mutex> lock(activeRequestsMutex);
    return activeRequests.find(entityId) != activeRequests.end();
}

size_t AsyncEntityPathfinder::getPendingRequestsCount() const {
    std::lock_guard<std::mutex> lock(requestQueueMutex);
    return requestQueue.size();
}

size_t AsyncEntityPathfinder::getCompletedResultsCount() const {
    std::lock_guard<std::mutex> lock(resultQueueMutex);
    return resultQueue.size();
}

void AsyncEntityPathfinder::startBackgroundProcessing() {
    // Create a taskflow for continuous processing
    taskflow.emplace_back();
    tf::Taskflow& tf = taskflow.back();
    
    // Create a task that processes pathfinding requests
    tf.emplace([this]() {
        processPathfindingRequests();
    });
    
    // Run the taskflow asynchronously
    executor.run(tf);
}

void AsyncEntityPathfinder::processPathfindingRequests() {
    while (isRunning) {
        AsyncPathfindingRequest request;
        bool hasRequest = false;
        
        // Get next request from queue
        {
            std::lock_guard<std::mutex> lock(requestQueueMutex);
            if (!requestQueue.empty()) {
                request = requestQueue.front();
                requestQueue.pop();
                hasRequest = true;
            }
        }
        
        if (hasRequest) {
            processRequest(request);
        } else {
            // No requests available, sleep briefly to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void AsyncEntityPathfinder::processRequest(const AsyncPathfindingRequest& request) {
    // Check if request was cancelled
    {
        std::lock_guard<std::mutex> lock(activeRequestsMutex);
        if (cancelledRequests.find(request.requestId) != cancelledRequests.end()) {
            cancelledRequests.erase(request.requestId);
            std::cout << "Skipping cancelled pathfinding request " << request.requestId << std::endl;
            return;
        }
    }
    
    auto startTime = std::chrono::steady_clock::now();
      AsyncPathfindingResult result;
    result.requestId = request.requestId;
    result.entityId = request.entityId;
    result.instanceName = request.instanceName;
    result.walkType = request.walkType;
    result.targetX = request.endX;
    result.targetY = request.endY;
    result.success = false;
    result.completed = false;
    result.failed = false;
    result.errorMessage = "";
    
    try {        // Perform the actual pathfinding calculation
        std::vector<std::pair<float, float>> path = findPath(
            request.startX, request.startY,
            request.endX, request.endY,
            gameMap,
            request.config
        );
        
        // Check again if request was cancelled (pathfinding can take time)
        {
            std::lock_guard<std::mutex> lock(activeRequestsMutex);
            if (cancelledRequests.find(request.requestId) != cancelledRequests.end()) {
                cancelledRequests.erase(request.requestId);
                std::cout << "Pathfinding request " << request.requestId << " cancelled after computation" << std::endl;
                return;
            }
        }
          result.path = std::move(path);
        result.success = !result.path.empty();
        result.completed = true;
        result.failed = false;
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        result.computationTimeMs = duration.count() / 1000.0f;
        
        std::cout << "Pathfinding request " << request.requestId << " completed in " 
                  << result.computationTimeMs << "ms, path size: " << result.path.size() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Pathfinding request " << request.requestId << " failed: " << e.what() << std::endl;
        result.success = false;
        result.completed = true;
        result.failed = true;
        result.errorMessage = e.what();
        result.path.clear();
    }
    
    // Add result to result queue
    {
        std::lock_guard<std::mutex> lock(resultQueueMutex);
        resultQueue.push(std::move(result));
    }
    
    // Remove from active requests
    {
        std::lock_guard<std::mutex> lock(activeRequestsMutex);
        auto it = activeRequests.find(request.entityId);
        if (it != activeRequests.end() && it->second == request.requestId) {
            activeRequests.erase(it);
        }
    }
}
