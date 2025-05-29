// Define M_PI early to avoid compilation issues with third-party libraries
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#include "asyncPathfinding.h"
#include "map.h"
#include "crashDebug.h"
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
    : executor(numThreads), nextRequestId(1), isRunning(false), gameMapPtr(nullptr) {
    std::cout << "AsyncEntityPathfinder initialized with " << numThreads << " threads using Taskflow efficiently" << std::endl;
}

AsyncEntityPathfinder::~AsyncEntityPathfinder() {
    stop();
}

void AsyncEntityPathfinder::initialize(const Map* gameMap) {
    std::lock_guard<std::mutex> lock(gameMapMutex);
    DEBUG_VALIDATE_PTR(gameMap);
    if (!gameMap) {
        std::cerr << "ERROR: AsyncEntityPathfinder::initialize - gameMap cannot be null!" << std::endl;
        DEBUG_LOG_MEMORY("pathfinder_init_null_map");
        return;
    }
    gameMapPtr = gameMap;
    DEBUG_LOG_MEMORY("pathfinder_initialized");
    std::cout << "AsyncEntityPathfinder initialized with game map reference" << std::endl;
}

void AsyncEntityPathfinder::start() {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (!gameMapPtr) {
        std::cerr << "ERROR: AsyncEntityPathfinder::start - Must call initialize() with game map first!" << std::endl;
        return;
    }
    if (!isRunning) {
        isRunning = true;
        std::cout << "AsyncEntityPathfinder started with efficient Taskflow-based processing" << std::endl;
    }
}

void AsyncEntityPathfinder::stop() {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (isRunning) {
        isRunning = false;
        
        std::cout << "AsyncEntityPathfinder: Stopping Taskflow-based processing..." << std::endl;
        
        // Clear all cancelled requests and active requests
        {
            std::lock_guard<std::mutex> activeLock(activeRequestsMutex);
            cancelledRequests.clear();
            activeRequests.clear();
        }
        
        // Wait for all running tasks to complete
        std::cout << "Waiting for all async pathfinding tasks to complete..." << std::endl;
        
        try {
            executor.wait_for_all();
            std::cout << "AsyncEntityPathfinder stopped successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception during AsyncEntityPathfinder shutdown: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception during AsyncEntityPathfinder shutdown!" << std::endl;
        }
    }
}

uint32_t AsyncEntityPathfinder::requestPathfinding(const std::string& entityId, 
                                                   float startX, float startY, 
                                                   float endX, float endY,
                                                   const EntityConfiguration& config,
                                                   WalkType walkType) {
    if (!isRunning.load()) {
        std::cerr << "ERROR: AsyncEntityPathfinder is not running!" << std::endl;
        return 0;
    }
    
    // Validate game map
    {
        std::lock_guard<std::mutex> lock(gameMapMutex);
        if (!gameMapPtr) {
            std::cerr << "ERROR: Game map not initialized!" << std::endl;
            return 0;
        }
    }
    
    uint32_t requestId = nextRequestId++;
    
    // Create request
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
    
    // Track active request for this entity (cancel previous if exists)
    {
        std::lock_guard<std::mutex> lock(activeRequestsMutex);
        auto it = activeRequests.find(entityId);
        if (it != activeRequests.end()) {
            // Cancel previous request for this entity
            cancelledRequests.insert(it->second);
            std::cout << "Cancelling previous pathfinding request for entity " << entityId << std::endl;
        }
        activeRequests[entityId] = requestId;
    }
    
    // Create and submit individual Taskflow task - this is the key improvement!
    // Each pathfinding request gets its own task instead of using a queue-based system
    auto taskflow = std::make_shared<tf::Taskflow>();
    taskflow->emplace([this, request]() {
        processPathfindingTask(request);
    });
    
    // Submit task to executor (non-blocking) - Taskflow handles the scheduling efficiently
    executor.run(*taskflow);
    
    std::cout << "Pathfinding task " << requestId << " submitted for entity " << entityId 
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

size_t AsyncEntityPathfinder::getActiveRequestsCount() const {
    std::lock_guard<std::mutex> lock(activeRequestsMutex);
    return activeRequests.size();
}

size_t AsyncEntityPathfinder::getCompletedResultsCount() const {
    std::lock_guard<std::mutex> lock(resultQueueMutex);
    return resultQueue.size();
}

// This is the new efficient processing function that handles individual tasks
void AsyncEntityPathfinder::processPathfindingTask(AsyncPathfindingRequest request) {
    // Early safety checks
    if (!isRunning.load()) {
        std::cout << "Pathfinding request " << request.requestId << " skipped - system shutting down" << std::endl;
        return;
    }
    
    // Check if request was cancelled before processing
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
    
    try {
        // Perform the actual pathfinding calculation with thread-safe map access
        const Map* currentGameMap = nullptr;
        {
            std::lock_guard<std::mutex> mapLock(gameMapMutex);
            currentGameMap = gameMapPtr;
        }
        
        if (!currentGameMap) {
            throw std::runtime_error("Game map not available for pathfinding");
        }
        
        // Call the pathfinding algorithm
        std::vector<std::pair<float, float>> path = findPath(
            request.startX, request.startY,
            request.endX, request.endY,
            *currentGameMap,  // Thread-safe map reference
            request.config
        );
        
        // Check again if request was cancelled after computation (pathfinding can take time)
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
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        result.computationTimeMs = duration.count() / 1000.0f;
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
    
    DEBUG_LOG_MEMORY("pathfinding_task_completed");
}
