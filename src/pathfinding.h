#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "map.h"
#include "entities.h"  // For EntityConfiguration
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <set>
#include <cmath>
#include <future>
#include <atomic>
#include <chrono>
#include "enumDefinitions.h"


// Forward declaration for EntityConfiguration
// struct EntityConfiguration; // No longer needed, we include entities.h

// Minimum allowed distance between waypoints to reduce path zigzagging
const float MINIMUM_DISTANCE_BETWEEN_WAYPOINTS = 3.0f;

// Minimum distance buffer from avoidance blocks (0 = no buffer, >0 = safety margin)
extern float MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS;

// Minimum distance buffer from avoidance elements (0 = no buffer, >0 = safety margin)
extern float MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS;

// A structure to represent a node in the pathfinding system
struct Node {
    float x;
    float y;
    float gCost;  // Cost from start to this node
    float hCost;  // Heuristic cost from this node to the goal
    float fCost;  // Total cost (g + h)
    Node* parent;
    
    // Constructor with initialization
    Node(float _x, float _y) : x(_x), y(_y), gCost(0), hCost(0), fCost(0), parent(nullptr) {}
    
    // Comparator for priority queue (lowest fCost has highest priority)
    bool operator>(const Node& other) const {
        return fCost > other.fCost;
    }
};

// Custom hash function for std::pair<float, float>
struct PairHash {
    std::size_t operator()(const std::pair<float, float>& p) const {
        // Hash floats by converting to bits to avoid floating point precision issues
        auto h1 = std::hash<float>{}(p.first);
        auto h2 = std::hash<float>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

// Find a path from start to goal using A* algorithm with proper entity collision shape detection
// Returns a vector of positions (x, y) forming the path
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    const EntityConfiguration& entityConfig,
    const std::string& excludeInstanceName = ""
);

// Optimized pathfinding function with performance monitoring
std::vector<std::pair<float, float>> findPathOptimized(
    float startX, float startY,
    float goalX, float goalY,
    const EntityConfiguration& entityConfig,
    const Map& gameMap,
    float stepSize,
    const std::string& excludeInstanceName = ""
);

// Check if a position is valid for pathfinding using entity collision shape
bool isPositionValid(float x, float y, const EntityConfiguration& entityConfig, const Map& gameMap, const std::string& excludeInstanceName = "");

// Calculate the heuristic value (estimated cost to goal)
// Using Euclidean distance for floating-point pathfinding
float calculateHeuristic(float x1, float y1, float x2, float y2);

// Get all valid neighbors for a position with collision checking using entity shape
std::vector<std::pair<float, float>> getNeighbors(float x, float y, float stepSize, const EntityConfiguration& entityConfig, const Map& gameMap, const std::string& excludeInstanceName = "");

// Check if a segment between two points is valid (no collisions along the path)
bool isSegmentValid(float x1, float y1, float x2, float y2, const EntityConfiguration& entityConfig, const Map& gameMap, const std::string& excludeInstanceName = "");

// Generate a unique key for an entity configuration for caching purposes
std::string generateEntityKey(const EntityConfiguration& config);

// Initialize pathfinding cache system
void initializePathfindingCache();

// Performance monitoring
struct PathfindingStats {
    std::atomic<int> nodesExplored{0};
    std::atomic<int> collisionChecks{0};
    std::atomic<int> totalPathfindingCalls{0};
    std::atomic<double> totalComputationTimeMs{0.0};
    std::chrono::high_resolution_clock::time_point startTime;
    std::atomic<double> totalTime{0.0};
    
    void reset() {
        nodesExplored = 0;
        collisionChecks = 0;
        totalPathfindingCalls = 0;
        totalComputationTimeMs = 0.0;
        totalTime = 0.0;
        startTime = std::chrono::high_resolution_clock::now();
    }
    
    void updateTime() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        totalTime = duration.count() / 1000.0; // Convert to milliseconds
    }
};

extern PathfindingStats g_pathfindingStats;

// Pre-calculated collision shapes for entities and elements
struct PreCalculatedCollisionShapes {
    std::unordered_map<std::string, std::vector<std::pair<float, float>>> entityShapes;
    std::unordered_map<std::string, std::vector<std::pair<float, float>>> elementShapes;
    std::unordered_map<std::string, EntityConfiguration> expandedEntityConfigs;
    
    void preCalculateEntityShape(const std::string& entityId, const EntityConfiguration& config);
    void preCalculateElementShape(const std::string& elementId, const std::vector<std::pair<float, float>>& shape);
    void clear();
    
    // Check if entity has pre-calculated collision shapes
    bool hasEntityShape(const EntityConfiguration& config) const;
    
    // Get pre-calculated collision shapes for entity (returns elements and blocks expanded shapes)
    std::pair<std::vector<std::pair<float, float>>, std::vector<std::pair<float, float>>> getEntityShapes(const EntityConfiguration& config) const;
};

extern PreCalculatedCollisionShapes g_collisionCache;

// Async pathfinding result
struct PathfindingResult {
    std::vector<std::pair<float, float>> path;
    bool completed = false;
    bool failed = false;
    bool success = false;
    std::string errorMessage;
    int requestId = 0;
    long long computationTimeMs = 0;
    PathfindingStats stats;
    
    // Constructor
    PathfindingResult() = default;
    
    // Copy constructor
    PathfindingResult(const PathfindingResult& other)
        : path(other.path), completed(other.completed), failed(other.failed), 
          success(other.success), errorMessage(other.errorMessage),
          requestId(other.requestId), computationTimeMs(other.computationTimeMs),
          stats() {} // Don't copy atomic stats
      // Move constructor
    PathfindingResult(PathfindingResult&& other) noexcept
        : path(std::move(other.path)), completed(other.completed), failed(other.failed),
          success(other.success), errorMessage(std::move(other.errorMessage)),
          requestId(other.requestId), computationTimeMs(other.computationTimeMs),
          stats() {} // Don't move atomic stats
    
    // Assignment operators
    PathfindingResult& operator=(const PathfindingResult& other) {
        if (this != &other) {
            path = other.path;
            completed = other.completed;
            failed = other.failed;
            success = other.success;
            errorMessage = other.errorMessage;
            requestId = other.requestId;
            computationTimeMs = other.computationTimeMs;
            // Don't copy stats (atomic)
        }
        return *this;
    }
      PathfindingResult& operator=(PathfindingResult&& other) noexcept {
        if (this != &other) {
            path = std::move(other.path);
            completed = other.completed;
            failed = other.failed;
            success = other.success;
            errorMessage = std::move(other.errorMessage);
            requestId = other.requestId;
            computationTimeMs = other.computationTimeMs;
            // Don't move stats (atomic)
        }
        return *this;
    }
};

// Async pathfinding request
struct PathfindingRequest {
    float startX, startY;
    float goalX, goalY;
    std::string entityId;
    std::string instanceName;  // Instance name of the entity
    int requestId = 0;
    EntityConfiguration entityConfig;
    Map gameMap; // Copy of the map state
    float stepSize = 0.1f;
    float maxSearchTime = 1000.0f; // Maximum search time in milliseconds
};

class AsyncPathfinder {
private:
    std::future<PathfindingResult> future_;
    std::atomic<bool> isRunning_{false};
    std::atomic<bool> shouldCancel_{false};
    std::mutex mutex_;
    std::unique_ptr<PathfindingResult> result_;
    
public:
    // Start async pathfinding
    void startPathfinding(const PathfindingRequest& request);
    
    // Check if pathfinding is complete
    bool isPathfindingComplete();
      // Get the result (non-blocking, returns nullptr if not complete)
    std::unique_ptr<PathfindingResult> getResult();
    
    // Cancel current pathfinding
    void cancelPathfinding();
    
    // Check if currently running
    bool isCurrentlyRunning() const { return isRunning_.load(); }
    
    // Internal async pathfinding method
    PathfindingResult findPathAsync(const PathfindingRequest& request);
      // Internal pathfinding with cancellation support
    std::vector<std::pair<float, float>> findPathWithCancellation(
        float startX, float startY,
        float goalX, float goalY,
        const EntityConfiguration& entityConfig,
        const Map& gameMap,
        float stepSize,
        const EntityConfiguration* expandedConfigElements = nullptr,
        const EntityConfiguration* expandedConfigBlocks = nullptr,
        const std::string& excludeInstanceName = "");
};

extern AsyncPathfinder g_asyncPathfinder;

// Optimized position validation using pre-calculated collision shapes
bool isPositionValidOptimized(float x, float y, const EntityConfiguration& entityConfig,
                             const EntityConfiguration& expandedConfigElements,
                             const EntityConfiguration& expandedConfigBlocks,
                             const Map& gameMap, const std::string& excludeInstanceName = "");

// Async pathfinding function
std::future<PathfindingResult> findPathAsync(const PathfindingRequest& request);

// Pathfinding cooldown system
extern const float PATH_FINDING_COOLDOWN;
bool canEntityRequestPathfinding(const std::string& entityInstanceName);
void updateEntityPathfindingTime(const std::string& entityInstanceName);
void clearEntityPathfindingCooldown(const std::string& entityInstanceName);

// Hierarchical pathfinding for long-distance optimization
const float HIERARCHICAL_PATHFINDING_THRESHOLD = 50.0f;  // Distance threshold to use hierarchical pathfinding
const float CLUSTER_SIZE = 20.0f;                        // Size of each cluster in the hierarchical graph
const float INTER_CLUSTER_CONNECTION_RADIUS = 30.0f;     // Radius for connecting clusters

// Hierarchical pathfinding cluster representation
struct PathfindingCluster {
    int id;
    float centerX, centerY;
    float minX, minY, maxX, maxY;  // Bounding box
    std::vector<std::pair<float, float>> entrancePoints;  // Points where entities can enter/exit cluster
    std::vector<int> connectedClusters;                   // IDs of connected clusters
    bool isObstacle = false;                              // True if cluster is mostly blocked
    int obstaclePercentage = 0;                           // Percentage of cluster that's blocked
    
    PathfindingCluster(int _id, float _centerX, float _centerY) 
        : id(_id), centerX(_centerX), centerY(_centerY) {
        minX = _centerX - CLUSTER_SIZE / 2;
        maxX = _centerX + CLUSTER_SIZE / 2;
        minY = _centerY - CLUSTER_SIZE / 2;
        maxY = _centerY + CLUSTER_SIZE / 2;
    }
};

// Hierarchical pathfinding graph
class HierarchicalPathfindingGraph {
private:
    std::vector<PathfindingCluster> clusters;
    std::unordered_map<int, std::vector<int>> clusterConnections;
    std::unordered_map<std::pair<int, int>, float, PairHash> interClusterDistances;
    
    bool isInitialized = false;
    float lastUpdateTime = 0.0f;
    const float UPDATE_INTERVAL = 5.0f;  // Update every 5 seconds
    
public:
    void initialize(const Map& gameMap);
    void updateGraph(const Map& gameMap, bool forceUpdate = false);
    
    // Find high-level path between clusters
    std::vector<int> findClusterPath(int startClusterId, int goalClusterId);
    
    // Convert cluster path to world coordinates
    std::vector<std::pair<float, float>> clusterPathToWorldPath(
        const std::vector<int>& clusterPath,
        float startX, float startY,
        float goalX, float goalY
    );
    
    // Get cluster ID for a world position
    int getClusterIdForPosition(float x, float y) const;
    
    // Check if direct path is possible between two clusters
    bool canConnectClusters(int clusterId1, int clusterId2, const Map& gameMap) const;
    
    // Get entrance points for a cluster
    std::vector<std::pair<float, float>> getClusterEntrancePoints(int clusterId) const;
    
    // Utility functions
    void clear();
    bool isEmpty() const;
    size_t getClusterCount() const;
    const PathfindingCluster* getCluster(int clusterId) const;
    bool isInitializedState() const { return isInitialized; }  // Public access to initialization state
    
private:
    void generateClusters(const Map& gameMap);
    void analyzeClusterObstacles(PathfindingCluster& cluster, const Map& gameMap);
    void findClusterConnections(const Map& gameMap);
    void generateEntrancePoints(PathfindingCluster& cluster, const Map& gameMap);
    float calculateClusterDistance(int clusterId1, int clusterId2) const;
};

extern HierarchicalPathfindingGraph g_hierarchicalPathfindingGraph;

// Enhanced pathfinding functions that use hierarchical approach for long distances
std::vector<std::pair<float, float>> findPathHierarchical(
    float startX, float startY,
    float goalX, float goalY,
    const EntityConfiguration& entityConfig,
    const Map& gameMap,
    float stepSize = 1.0f,
    const std::string& excludeInstanceName = ""
);

// Hybrid pathfinding that chooses the best approach based on distance
std::vector<std::pair<float, float>> findPathHybrid(
    float startX, float startY,
    float goalX, float goalY,
    const EntityConfiguration& entityConfig,
    const Map& gameMap,
    float stepSize = 1.0f,
    const std::string& excludeInstanceName = ""
);

// Performance monitoring for hierarchical pathfinding
struct HierarchicalPathfindingStats {
    std::atomic<int> hierarchicalPathsUsed{0};
    std::atomic<int> directPathsUsed{0};
    std::atomic<int> clusterPathsGenerated{0};
    std::atomic<double> hierarchicalTimeMs{0.0};
    std::atomic<double> directTimeMs{0.0};
    std::atomic<double> avgHierarchicalSpeedup{0.0};
    
    void reset();
    void printStats() const;
    void updateSpeedup(double hierarchicalTime, double estimatedDirectTime);
};

extern HierarchicalPathfindingStats g_hierarchicalPathfindingStats;

#endif // PATHFINDING_H
