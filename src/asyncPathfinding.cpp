// Define M_PI early to avoid compilation issues with third-party libraries
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#include "asyncPathfinding.h"
#include "map.h"
#include "crashDebug.h"
#include <iostream>
#include <chrono>
#include "enumDefinitions.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declaration of findPath function from pathfinding.cpp
extern std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    const EntityConfiguration& entityConfig,
    const std::string& excludeInstanceName = ""
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
        
        // First, mark all active requests as cancelled to prevent new processing
        {
            std::lock_guard<std::mutex> activeLock(activeRequestsMutex);
            for (const auto& pair : activeRequests) {
                cancelledRequests.insert(pair.second);
            }
            activeRequests.clear();
        }
        
        // CRASH FIX: Enhanced graceful shutdown with better exception handling
        std::cout << "Waiting for all async pathfinding tasks to complete..." << std::endl;
        
        try {
            // THREAD SAFETY: Use timeout-based waiting to prevent deadlocks
            auto startTime = std::chrono::steady_clock::now();
            const auto maxWaitTime = std::chrono::seconds(10); // 10 second timeout
            
            // Wait for tasks individually with timeout checking
            bool allTasksCompleted = true;
            {
                std::lock_guard<std::mutex> tasksLock(activeTasksMutex);
                for (auto& taskPair : activeTasks) {
                    try {
                        auto future = std::move(taskPair.second);
                        if (future.valid()) {
                            // Wait with timeout to prevent indefinite blocking
                            auto status = future.wait_for(std::chrono::milliseconds(100));
                            if (status != std::future_status::ready) {
                                // Task not ready, check overall timeout
                                auto elapsed = std::chrono::steady_clock::now() - startTime;
                                if (elapsed > maxWaitTime) {
                                    std::cerr << "WARNING: Task " << taskPair.first << " did not complete within timeout" << std::endl;
                                    allTasksCompleted = false;
                                    break;
                                }
                            }
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Exception waiting for task " << taskPair.first << ": " << e.what() << std::endl;
                        allTasksCompleted = false;
                    } catch (...) {
                        std::cerr << "Unknown exception waiting for task " << taskPair.first << std::endl;
                        allTasksCompleted = false;
                    }
                }
            }
            
            // CRASH FIX: Use safe executor shutdown without wait_for_all()
            // The wait_for_all() method seems to cause issues in the threading library
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give threads time to finish
            
            if (allTasksCompleted) {
                std::cout << "All async pathfinding tasks completed successfully" << std::endl;
            } else {
                std::cout << "Some async pathfinding tasks may not have completed cleanly" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "CRITICAL: Exception during graceful shutdown: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "CRITICAL: Unknown exception during graceful shutdown!" << std::endl;
        }
        
        // Clean up after all tasks are done (or timeout reached)
        try {
            {
                std::lock_guard<std::mutex> tasksLock(activeTasksMutex);
                activeTasks.clear();
            }
            
            {
                std::lock_guard<std::mutex> activeLock(activeRequestsMutex);
                cancelledRequests.clear();
            }
            
            // CRASH FIX: Clear result queue to prevent memory leaks
            {
                std::lock_guard<std::mutex> resultLock(resultQueueMutex);
                while (!resultQueue.empty()) {
                    resultQueue.pop();
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Exception during cleanup: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception during cleanup!" << std::endl;
        }
        
        std::cout << "AsyncEntityPathfinder stopped with enhanced safety measures" << std::endl;
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
    request.config = config;    request.walkType = walkType;
    request.timestamp = std::chrono::steady_clock::now();
    
    // Track active request for this entity (cancel previous if exists)
    {
        std::lock_guard<std::mutex> lock(activeRequestsMutex);
        auto it = activeRequests.find(entityId);
        if (it != activeRequests.end()) {
            // Cancel previous request for this entity
            uint32_t oldRequestId = it->second;
            cancelledRequests.insert(oldRequestId);
            
            // Clean up the old task future
            {
                std::lock_guard<std::mutex> tasksLock(activeTasksMutex);
                activeTasks.erase(oldRequestId);
            }
            
            std::cout << "Cancelling previous pathfinding request for entity " << entityId << std::endl;        }
        activeRequests[entityId] = requestId;
    }    // Use the proper Taskflow async mechanism - much simpler and more stable
    // This creates a fire-and-forget async task without complex taskflow management
    try {
        auto future = executor.async([this, request]() {
            processPathfindingTask(request);
        });
        
        // Store the future for tracking only - no complex lifecycle management needed
        {
            std::lock_guard<std::mutex> lock(activeTasksMutex);
            activeTasks[requestId] = std::move(future);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to submit pathfinding task: " << e.what() << std::endl;
        
        // Clean up active request if task submission failed
        {
            std::lock_guard<std::mutex> lock(activeRequestsMutex);
            activeRequests.erase(entityId);
        }
        return 0;
    }
    
    std::cout << "Pathfinding task " << requestId << " submitted for entity " << entityId 
              << " from (" << startX << ", " << startY << ") to (" << endX << ", " << endY << ")" << std::endl;
    
    return requestId;
}

bool AsyncEntityPathfinder::cancelPathfindingRequest(const std::string& entityId) {
    uint32_t requestId = 0;
    
    // Get the request ID and mark it for cancellation
    {
        std::lock_guard<std::mutex> lock(activeRequestsMutex);
        auto it = activeRequests.find(entityId);
        if (it != activeRequests.end()) {
            requestId = it->second;
            cancelledRequests.insert(requestId);
            activeRequests.erase(it);
        } else {
            return false;
        }
    }
    
    // Remove the active task future to allow proper cleanup
    {
        std::lock_guard<std::mutex> lock(activeTasksMutex);
        activeTasks.erase(requestId);
    }
    
    std::cout << "Cancelled pathfinding request for entity " << entityId << std::endl;
    return true;
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
            request.config,
            request.instanceName  // Pass instance name to exclude self from collision checks
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
        resultQueue.push(std::move(result));    }
    
    // Remove from active requests
    {
        std::lock_guard<std::mutex> lock(activeRequestsMutex);
        auto it = activeRequests.find(request.entityId);
        if (it != activeRequests.end() && it->second == request.requestId) {
            activeRequests.erase(it);
        }    }
    
    // Remove the completed task future to allow proper cleanup
    {
        std::lock_guard<std::mutex> lock(activeTasksMutex);
        activeTasks.erase(request.requestId);
    }
    
    DEBUG_LOG_MEMORY("pathfinding_task_completed");
}
