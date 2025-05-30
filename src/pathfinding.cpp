#include "pathfinding.h"
#include "entities.h" // For EntityConfiguration
#include "globals.h" // For GRID_SIZE and DEBUG_LOGS
#include "collision.h" // For collision detection
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <limits>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "enumDefinitions.h"


// Global variables for minimum distance from avoidance objects
float MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS = 0.0f;  // Default: no buffer
float MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS = 0.0f; // Default: no buffer

// Pathfinding cooldown system
const float PATH_FINDING_COOLDOWN = 0.7f; // seconds - Minimum time between pathfinding calculations per entity (increased for performance)
static std::unordered_map<std::string, std::chrono::steady_clock::time_point> entityLastPathfindingTime;
static std::mutex pathfindingCooldownMutex;

// Global instances for performance optimization
PathfindingStats g_pathfindingStats;
PreCalculatedCollisionShapes g_collisionCache;
AsyncPathfinder g_asyncPathfinder;

// ===== HIERARCHICAL PATHFINDING IMPLEMENTATION =====

// Global instances for hierarchical pathfinding
HierarchicalPathfindingGraph g_hierarchicalPathfindingGraph;
HierarchicalPathfindingStats g_hierarchicalPathfindingStats;

void HierarchicalPathfindingStats::reset() {
    hierarchicalPathsUsed = 0;
    directPathsUsed = 0;
    clusterPathsGenerated = 0;
    hierarchicalTimeMs = 0.0;
    directTimeMs = 0.0;
    avgHierarchicalSpeedup = 0.0;
}

void HierarchicalPathfindingStats::printStats() const {
    int totalPaths = hierarchicalPathsUsed.load() + directPathsUsed.load();
    if (totalPaths > 0) {
        double hierarchicalPercentage = static_cast<double>(hierarchicalPathsUsed.load()) / totalPaths * 100.0;
        double avgHierarchicalTime = hierarchicalPathsUsed.load() > 0 ? 
            hierarchicalTimeMs.load() / hierarchicalPathsUsed.load() : 0.0;
        double avgDirectTime = directPathsUsed.load() > 0 ? 
            directTimeMs.load() / directPathsUsed.load() : 0.0;
        
        std::cout << "=== Hierarchical Pathfinding Stats ===" << std::endl;
        std::cout << "Total Paths: " << totalPaths << std::endl;
        std::cout << "Hierarchical Paths: " << hierarchicalPathsUsed.load() 
                  << " (" << std::fixed << std::setprecision(1) << hierarchicalPercentage << "%)" << std::endl;
        std::cout << "Direct Paths: " << directPathsUsed.load() << std::endl;
        std::cout << "Cluster Paths Generated: " << clusterPathsGenerated.load() << std::endl;
        std::cout << "Avg Hierarchical Time: " << std::fixed << std::setprecision(2) << avgHierarchicalTime << "ms" << std::endl;
        std::cout << "Avg Direct Time: " << std::fixed << std::setprecision(2) << avgDirectTime << "ms" << std::endl;
        std::cout << "Avg Speedup: " << std::fixed << std::setprecision(2) << avgHierarchicalSpeedup.load() << "x" << std::endl;
    }
}

void HierarchicalPathfindingStats::updateSpeedup(double hierarchicalTime, double estimatedDirectTime) {
    if (hierarchicalTime > 0 && estimatedDirectTime > 0) {
        double speedup = estimatedDirectTime / hierarchicalTime;
        double currentAvg = avgHierarchicalSpeedup.load();
        int currentCount = hierarchicalPathsUsed.load();
        
        // Calculate running average
        double newAvg = (currentAvg * (currentCount - 1) + speedup) / currentCount;
        avgHierarchicalSpeedup.store(newAvg);
    }
}

// HierarchicalPathfindingGraph Implementation
void HierarchicalPathfindingGraph::initialize(const Map& gameMap) {
    if (isInitialized) return;
    
    if (DEBUG_LOGS) {
        std::cout << "Initializing hierarchical pathfinding graph..." << std::endl;
    }
    
    clear();
    generateClusters(gameMap);
    findClusterConnections(gameMap);
    
    isInitialized = true;
    lastUpdateTime = static_cast<float>(glfwGetTime());
    
    if (DEBUG_LOGS) {
        std::cout << "Hierarchical pathfinding graph initialized with " 
                  << clusters.size() << " clusters" << std::endl;
    }
}

void HierarchicalPathfindingGraph::updateGraph(const Map& gameMap, bool forceUpdate) {
    float currentTime = static_cast<float>(glfwGetTime());
    
    if (!forceUpdate && (currentTime - lastUpdateTime < UPDATE_INTERVAL)) {
        return;
    }
    
    // Re-analyze cluster obstacles (connections remain mostly static)
    for (auto& cluster : clusters) {
        analyzeClusterObstacles(cluster, gameMap);
    }
    
    lastUpdateTime = currentTime;
    
    if (DEBUG_LOGS) {
        std::cout << "Updated hierarchical pathfinding graph" << std::endl;
    }
}

std::vector<int> HierarchicalPathfindingGraph::findClusterPath(int startClusterId, int goalClusterId) {
    if (startClusterId == goalClusterId) {
        return {startClusterId};
    }
    
    // A* search on cluster graph
    std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> openSet;
    std::unordered_map<int, float> gCost;
    std::unordered_map<int, int> parent;
    std::unordered_set<int> closedSet;
    
    openSet.push({0.0f, startClusterId});
    gCost[startClusterId] = 0.0f;
    
    const PathfindingCluster* goalCluster = getCluster(goalClusterId);
    if (!goalCluster) return {};
    
    while (!openSet.empty()) {
        int currentClusterId = openSet.top().second;
        openSet.pop();
        
        if (currentClusterId == goalClusterId) {
            // Reconstruct path
            std::vector<int> path;
            int current = goalClusterId;
            while (current != startClusterId) {
                path.push_back(current);
                current = parent[current];
            }
            path.push_back(startClusterId);
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        if (closedSet.count(currentClusterId)) continue;
        closedSet.insert(currentClusterId);
        
        // Check connections
        auto connectionIt = clusterConnections.find(currentClusterId);
        if (connectionIt != clusterConnections.end()) {
            for (int neighborId : connectionIt->second) {
                if (closedSet.count(neighborId)) continue;
                
                const PathfindingCluster* neighbor = getCluster(neighborId);
                if (!neighbor || neighbor->isObstacle) continue;
                
                float tentativeGCost = gCost[currentClusterId] + calculateClusterDistance(currentClusterId, neighborId);
                
                if (gCost.find(neighborId) == gCost.end() || tentativeGCost < gCost[neighborId]) {
                    gCost[neighborId] = tentativeGCost;
                    parent[neighborId] = currentClusterId;
                    
                    // Heuristic: distance to goal cluster
                    float hCost = calculateClusterDistance(neighborId, goalClusterId);
                    float fCost = tentativeGCost + hCost;
                    
                    openSet.push({fCost, neighborId});
                }
            }
        }
    }
    
    return {}; // No path found
}

std::vector<std::pair<float, float>> HierarchicalPathfindingGraph::clusterPathToWorldPath(
    const std::vector<int>& clusterPath,
    float startX, float startY,
    float goalX, float goalY) {
    
    if (clusterPath.empty()) return {};
    if (clusterPath.size() == 1) {
        return {{startX, startY}, {goalX, goalY}};
    }
    
    std::vector<std::pair<float, float>> worldPath;
    worldPath.push_back({startX, startY});
    
    // Add waypoints at cluster boundaries
    for (size_t i = 1; i < clusterPath.size(); ++i) {
        const PathfindingCluster* cluster = getCluster(clusterPath[i]);
        if (cluster && !cluster->entrancePoints.empty()) {
            // Choose the entrance point closest to the previous position
            float prevX = worldPath.back().first;
            float prevY = worldPath.back().second;
            
            std::pair<float, float> bestEntrance = cluster->entrancePoints[0];
            float bestDistance = std::numeric_limits<float>::max();
            
            for (const auto& entrance : cluster->entrancePoints) {
                float dx = entrance.first - prevX;
                float dy = entrance.second - prevY;
                float distance = std::sqrt(dx * dx + dy * dy);
                
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestEntrance = entrance;
                }
            }
            
            worldPath.push_back(bestEntrance);
        } else {
            // Fallback: use cluster center
            const PathfindingCluster* cluster = getCluster(clusterPath[i]);
            if (cluster) {
                worldPath.push_back({cluster->centerX, cluster->centerY});
            }
        }
    }
    
    worldPath.push_back({goalX, goalY});
    return worldPath;
}

int HierarchicalPathfindingGraph::getClusterIdForPosition(float x, float y) const {
    for (const auto& cluster : clusters) {
        if (x >= cluster.minX && x <= cluster.maxX && 
            y >= cluster.minY && y <= cluster.maxY) {
            return cluster.id;
        }
    }
    return -1; // No cluster found
}

bool HierarchicalPathfindingGraph::canConnectClusters(int clusterId1, int clusterId2, const Map& gameMap) const {
    const PathfindingCluster* cluster1 = getCluster(clusterId1);
    const PathfindingCluster* cluster2 = getCluster(clusterId2);
    
    if (!cluster1 || !cluster2) return false;
    
    // Check if clusters are within connection radius
    float dx = cluster1->centerX - cluster2->centerX;
    float dy = cluster1->centerY - cluster2->centerY;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    if (distance > INTER_CLUSTER_CONNECTION_RADIUS) return false;
    
    // Simple line-of-sight check between cluster centers
    float stepSize = 2.0f;
    int steps = static_cast<int>(distance / stepSize);
    
    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / steps;
        float checkX = cluster1->centerX + t * (cluster2->centerX - cluster1->centerX);
        float checkY = cluster1->centerY + t * (cluster2->centerY - cluster1->centerY);
        
        // Check if position is blocked
        if (wouldCollideWithMapBlock(checkX, checkY, gameMap)) {
            return false;
        }
    }
    
    return true;
}

std::vector<std::pair<float, float>> HierarchicalPathfindingGraph::getClusterEntrancePoints(int clusterId) const {
    const PathfindingCluster* cluster = getCluster(clusterId);
    if (cluster) {
        return cluster->entrancePoints;
    }
    return {};
}

void HierarchicalPathfindingGraph::clear() {
    clusters.clear();
    clusterConnections.clear();
    interClusterDistances.clear();
    isInitialized = false;
    lastUpdateTime = 0.0f;
}

bool HierarchicalPathfindingGraph::isEmpty() const {
    return clusters.empty();
}

size_t HierarchicalPathfindingGraph::getClusterCount() const {
    return clusters.size();
}

const PathfindingCluster* HierarchicalPathfindingGraph::getCluster(int clusterId) const {
    for (const auto& cluster : clusters) {
        if (cluster.id == clusterId) {
            return &cluster;
        }
    }
    return nullptr;
}

void HierarchicalPathfindingGraph::generateClusters(const Map& gameMap) {
    int clusterId = 0;
    
    // Generate clusters in a grid pattern
    for (float y = CLUSTER_SIZE / 2; y < GRID_SIZE; y += CLUSTER_SIZE) {
        for (float x = CLUSTER_SIZE / 2; x < GRID_SIZE; x += CLUSTER_SIZE) {
            PathfindingCluster cluster(clusterId++, x, y);
            analyzeClusterObstacles(cluster, gameMap);
            generateEntrancePoints(cluster, gameMap);
            clusters.push_back(cluster);
        }
    }
}

void HierarchicalPathfindingGraph::analyzeClusterObstacles(PathfindingCluster& cluster, const Map& gameMap) {
    int totalCells = 0;
    int blockedCells = 0;
    
    // Sample points within the cluster to determine obstacle percentage
    float sampleStep = 2.0f;
    for (float y = cluster.minY; y <= cluster.maxY; y += sampleStep) {
        for (float x = cluster.minX; x <= cluster.maxX; x += sampleStep) {
            totalCells++;
            
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
                if (wouldCollideWithMapBlock(x, y, gameMap)) {
                    blockedCells++;
                }
            } else {
                blockedCells++; // Out of bounds counts as blocked
            }
        }
    }
    
    if (totalCells > 0) {
        cluster.obstaclePercentage = (blockedCells * 100) / totalCells;
        cluster.isObstacle = cluster.obstaclePercentage > 70; // Mark as obstacle if >70% blocked
    }
}

void HierarchicalPathfindingGraph::findClusterConnections(const Map& gameMap) {
    // Find connections between adjacent clusters
    for (size_t i = 0; i < clusters.size(); ++i) {
        for (size_t j = i + 1; j < clusters.size(); ++j) {
            if (canConnectClusters(clusters[i].id, clusters[j].id, gameMap)) {
                clusterConnections[clusters[i].id].push_back(clusters[j].id);
                clusterConnections[clusters[j].id].push_back(clusters[i].id);
                
                // Cache distance
                float distance = calculateClusterDistance(clusters[i].id, clusters[j].id);
                interClusterDistances[{clusters[i].id, clusters[j].id}] = distance;
                interClusterDistances[{clusters[j].id, clusters[i].id}] = distance;
            }
        }
    }
}

void HierarchicalPathfindingGraph::generateEntrancePoints(PathfindingCluster& cluster, const Map& gameMap) {
    // Generate entrance points at cluster boundaries
    float step = CLUSTER_SIZE / 4.0f;
    
    // Top and bottom edges
    for (float x = cluster.minX + step; x < cluster.maxX; x += step) {
        // Top edge
        if (!wouldCollideWithMapBlock(x, cluster.minY, gameMap)) {
            cluster.entrancePoints.push_back({x, cluster.minY});
        }
        // Bottom edge
        if (!wouldCollideWithMapBlock(x, cluster.maxY, gameMap)) {
            cluster.entrancePoints.push_back({x, cluster.maxY});
        }
    }
    
    // Left and right edges
    for (float y = cluster.minY + step; y < cluster.maxY; y += step) {
        // Left edge
        if (!wouldCollideWithMapBlock(cluster.minX, y, gameMap)) {
            cluster.entrancePoints.push_back({cluster.minX, y});
        }
        // Right edge
        if (!wouldCollideWithMapBlock(cluster.maxX, y, gameMap)) {
            cluster.entrancePoints.push_back({cluster.maxX, y});
        }
    }
    
    // If no entrance points found, use center as fallback
    if (cluster.entrancePoints.empty()) {
        cluster.entrancePoints.push_back({cluster.centerX, cluster.centerY});
    }
}

float HierarchicalPathfindingGraph::calculateClusterDistance(int clusterId1, int clusterId2) const {
    const PathfindingCluster* cluster1 = getCluster(clusterId1);
    const PathfindingCluster* cluster2 = getCluster(clusterId2);
    
    if (!cluster1 || !cluster2) return std::numeric_limits<float>::max();
    
    float dx = cluster1->centerX - cluster2->centerX;
    float dy = cluster1->centerY - cluster2->centerY;
    return std::sqrt(dx * dx + dy * dy);
}

// Enhanced pathfinding functions
std::vector<std::pair<float, float>> findPathHierarchical(
    float startX, float startY,
    float goalX, float goalY,
    const EntityConfiguration& entityConfig,
    const Map& gameMap,
    float stepSize,
    const std::string& excludeInstanceName) {
    
    auto startTime = std::chrono::high_resolution_clock::now();
      // Initialize hierarchical graph if needed
    if (!g_hierarchicalPathfindingGraph.isInitializedState()) {
        g_hierarchicalPathfindingGraph.initialize(gameMap);
    } else {
        g_hierarchicalPathfindingGraph.updateGraph(gameMap);
    }
    
    // Find cluster path
    int startClusterId = g_hierarchicalPathfindingGraph.getClusterIdForPosition(startX, startY);
    int goalClusterId = g_hierarchicalPathfindingGraph.getClusterIdForPosition(goalX, goalY);
    
    if (startClusterId == -1 || goalClusterId == -1) {
        // Fallback to direct pathfinding
        if (DEBUG_LOGS) {
            std::cout << "Hierarchical pathfinding failed - invalid cluster IDs. Using direct pathfinding." << std::endl;
        }
        return findPathOptimized(startX, startY, goalX, goalY, entityConfig, gameMap, stepSize, excludeInstanceName);
    }
    
    std::vector<int> clusterPath = g_hierarchicalPathfindingGraph.findClusterPath(startClusterId, goalClusterId);
    
    if (clusterPath.empty()) {
        // No high-level path found, fallback to direct pathfinding
        if (DEBUG_LOGS) {
            std::cout << "No cluster path found. Using direct pathfinding." << std::endl;
        }
        return findPathOptimized(startX, startY, goalX, goalY, entityConfig, gameMap, stepSize, excludeInstanceName);
    }
    
    g_hierarchicalPathfindingStats.clusterPathsGenerated++;
    
    // Convert cluster path to world coordinates
    std::vector<std::pair<float, float>> roughPath = g_hierarchicalPathfindingGraph.clusterPathToWorldPath(
        clusterPath, startX, startY, goalX, goalY);
    
    // Refine the path with local pathfinding between waypoints
    std::vector<std::pair<float, float>> refinedPath;
    
    for (size_t i = 0; i < roughPath.size() - 1; ++i) {
        float segmentStartX = roughPath[i].first;
        float segmentStartY = roughPath[i].second;
        float segmentGoalX = roughPath[i + 1].first;
        float segmentGoalY = roughPath[i + 1].second;
        
        // Use faster step size for intermediate segments
        float localStepSize = std::max(stepSize, 2.0f);
        
        std::vector<std::pair<float, float>> segmentPath = findPathOptimized(
            segmentStartX, segmentStartY,
            segmentGoalX, segmentGoalY,
            entityConfig, gameMap, localStepSize, excludeInstanceName);
        
        // Add segment path (excluding the last point to avoid duplicates)
        for (size_t j = 0; j < segmentPath.size() - 1; ++j) {
            refinedPath.push_back(segmentPath[j]);
        }
    }
    
    // Add the final goal point
    if (!roughPath.empty()) {
        refinedPath.push_back(roughPath.back());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    g_hierarchicalPathfindingStats.hierarchicalPathsUsed++;
    g_hierarchicalPathfindingStats.hierarchicalTimeMs.store(
        g_hierarchicalPathfindingStats.hierarchicalTimeMs.load() + duration.count());
    
    // Estimate what direct pathfinding would have taken (rough approximation)
    float distance = std::sqrt((goalX - startX) * (goalX - startX) + (goalY - startY) * (goalY - startY));
    double estimatedDirectTime = distance * 0.5; // Rough estimation: 0.5ms per unit distance
    g_hierarchicalPathfindingStats.updateSpeedup(duration.count(), estimatedDirectTime);
    
    if (DEBUG_LOGS) {
        std::cout << "Hierarchical pathfinding completed in " << duration.count() 
                  << "ms, path size: " << refinedPath.size() << " points" << std::endl;
    }
    
    return refinedPath;
}

std::vector<std::pair<float, float>> findPathHybrid(
    float startX, float startY,
    float goalX, float goalY,
    const EntityConfiguration& entityConfig,
    const Map& gameMap,
    float stepSize,
    const std::string& excludeInstanceName) {
    
    // Calculate distance to determine which approach to use
    float distance = std::sqrt((goalX - startX) * (goalX - startX) + (goalY - startY) * (goalY - startY));
    
    if (distance >= HIERARCHICAL_PATHFINDING_THRESHOLD) {
        // Use hierarchical pathfinding for long distances
        if (DEBUG_LOGS) {
            std::cout << "Using hierarchical pathfinding for distance: " << distance << std::endl;
        }
        return findPathHierarchical(startX, startY, goalX, goalY, entityConfig, gameMap, stepSize, excludeInstanceName);
    } else {
        // Use direct pathfinding for short distances
        if (DEBUG_LOGS) {
            std::cout << "Using direct pathfinding for distance: " << distance << std::endl;
        }
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::vector<std::pair<float, float>> path = findPathOptimized(
            startX, startY, goalX, goalY, entityConfig, gameMap, stepSize, excludeInstanceName);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        g_hierarchicalPathfindingStats.directPathsUsed++;
        g_hierarchicalPathfindingStats.directTimeMs.store(
            g_hierarchicalPathfindingStats.directTimeMs.load() + duration.count());
        
        return path;
    }
}

// Generate a unique key for entity collision shapes based on their properties
std::string generateEntityKey(const EntityConfiguration& config) {
    // Create a simple hash-like key based on collision shape
    std::string key = "entity_";
    if (!config.collisionShapePoints.empty()) {
        // Use first and last points as a simple identifier
        auto first = config.collisionShapePoints.front();
        auto last = config.collisionShapePoints.back();
        key += std::to_string(static_cast<int>(first.first * 100)) + "_" +
               std::to_string(static_cast<int>(first.second * 100)) + "_" +
               std::to_string(static_cast<int>(last.first * 100)) + "_" +
               std::to_string(static_cast<int>(last.second * 100)) + "_" +
               std::to_string(config.collisionShapePoints.size());
    }
    return key;
}

// Pathfinding cooldown system functions
bool canEntityRequestPathfinding(const std::string& entityInstanceName) {
    std::lock_guard<std::mutex> lock(pathfindingCooldownMutex);
    
    auto it = entityLastPathfindingTime.find(entityInstanceName);
    if (it == entityLastPathfindingTime.end()) {
        // Entity has never requested pathfinding before, allow it
        return true;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastRequest = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count() / 1000.0f;
    
    if (timeSinceLastRequest >= PATH_FINDING_COOLDOWN) {
        return true;
    }
    
    if (DEBUG_LOGS) {
        std::cout << "Pathfinding request denied for entity " << entityInstanceName 
                  << " - cooldown active (time since last: " << timeSinceLastRequest << "s, required: " 
                  << PATH_FINDING_COOLDOWN << "s)" << std::endl;
    }
    
    return false;
}

void updateEntityPathfindingTime(const std::string& entityInstanceName) {
    std::lock_guard<std::mutex> lock(pathfindingCooldownMutex);
    entityLastPathfindingTime[entityInstanceName] = std::chrono::steady_clock::now();
    
    if (DEBUG_LOGS) {
        std::cout << "Updated pathfinding time for entity " << entityInstanceName << std::endl;
    }
}

void clearEntityPathfindingCooldown(const std::string& entityInstanceName) {
    std::lock_guard<std::mutex> lock(pathfindingCooldownMutex);
    entityLastPathfindingTime.erase(entityInstanceName);
    
    if (DEBUG_LOGS) {
        std::cout << "Cleared pathfinding cooldown for entity " << entityInstanceName << std::endl;
    }
}

// Initialize pathfinding cache for optimal performance
void initializePathfindingCache() {
    if (DEBUG_LOGS) {
        std::cout << "Initializing pathfinding collision cache..." << std::endl;
    }
    
    // Clear existing cache
    g_collisionCache.clear();
    
    // Pre-calculate for common entity shapes
    // This would be called with actual entity configurations from your game
    
    if (DEBUG_LOGS) {
        std::cout << "Pathfinding cache initialized" << std::endl;
    }
}

// Custom comparison for priority queue
struct CompareNodes {
    bool operator()(const Node* a, const Node* b) const {
        return a->fCost > b->fCost; // Lower fCost has higher priority
    }
};

// Calculate heuristic (Euclidean distance for smooth paths)
float calculateHeuristic(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

// Helper function to expand collision shape points by a given distance for safety buffer
// CRASH FIX: Add bounds checking for collision shape expansion
std::vector<std::pair<float, float>> expandCollisionShape(const std::vector<std::pair<float, float>>& originalShape, float expandDistance) {
    // CRASH FIX: Validate input parameters
    if (originalShape.empty()) {
        std::cerr << "WARNING: Attempting to expand empty collision shape" << std::endl;
        return originalShape;
    }
    
    if (expandDistance <= 0.0f) {
        return originalShape; // No expansion needed
    }
    
    // CRASH FIX: Limit maximum expansion to prevent extreme values
    if (expandDistance > 100.0f) {
        std::cerr << "WARNING: Collision expansion distance too large: " << expandDistance << ", clamping to 100.0f" << std::endl;
        expandDistance = 100.0f;
    }
    
    std::vector<std::pair<float, float>> expandedShape;
    expandedShape.reserve(originalShape.size()); // CRASH FIX: Pre-allocate memory
    
    // For each point, move it outward from the center of the shape
    // First, find the center point of the shape
    float centerX = 0.0f, centerY = 0.0f;
    for (const auto& point : originalShape) {
        centerX += point.first;
        centerY += point.second;
    }
    centerX /= static_cast<float>(originalShape.size());
    centerY /= static_cast<float>(originalShape.size());
    
    // Expand each point outward from the center
    for (const auto& point : originalShape) {
        float dx = point.first - centerX;
        float dy = point.second - centerY;
        
        // Calculate the distance from center to this point
        float distance = std::sqrt(dx * dx + dy * dy);
        
        if (distance > 0.0f) {
            // Normalize the direction vector and expand by the desired distance
            float normalizedDx = dx / distance;
            float normalizedDy = dy / distance;
            
            expandedShape.push_back({
                point.first + normalizedDx * expandDistance,
                point.second + normalizedDy * expandDistance
            });
        } else {
            // If point is at center, just add the original point
            expandedShape.push_back(point);
        }
    }
    return expandedShape;
}

// ===========================================
// PRE-CALCULATED COLLISION SHAPES IMPLEMENTATION
// ===========================================

void PreCalculatedCollisionShapes::preCalculateEntityShape(const std::string& entityId, const EntityConfiguration& config) {
    if (DEBUG_LOGS) {
        std::cout << "Pre-calculating collision shapes for entity: " << entityId << std::endl;
    }
    
    std::string entityKey = generateEntityKey(config);
    
    // Store original shape
    entityShapes[entityKey] = config.collisionShapePoints;
    
    // Pre-calculate expanded shapes for both elements and blocks
    EntityConfiguration expandedForElements = config;
    EntityConfiguration expandedForBlocks = config;
    
    if (MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS > 0.0f) {
        expandedForElements.collisionShapePoints = expandCollisionShape(
            config.collisionShapePoints, MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS);
        expandedEntityConfigs[entityKey + "_elements"] = expandedForElements;
    }
    
    if (MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS > 0.0f) {
        expandedForBlocks.collisionShapePoints = expandCollisionShape(
            config.collisionShapePoints, MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS);
        expandedEntityConfigs[entityKey + "_blocks"] = expandedForBlocks;
    }
    
    if (DEBUG_LOGS) {
        std::cout << "  Cached shapes for key: " << entityKey << std::endl;
        std::cout << "  Original shape points: " << config.collisionShapePoints.size() << std::endl;
        if (MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS > 0.0f) {
            std::cout << "  Expanded for elements: " << expandedForElements.collisionShapePoints.size() << " points" << std::endl;
        }
        if (MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS > 0.0f) {
            std::cout << "  Expanded for blocks: " << expandedForBlocks.collisionShapePoints.size() << " points" << std::endl;
        }
    }
}

void PreCalculatedCollisionShapes::preCalculateElementShape(const std::string& elementId, const std::vector<std::pair<float, float>>& shape) {
    elementShapes[elementId] = shape;
    if (DEBUG_LOGS) {
        std::cout << "Pre-calculated collision shape for element: " << elementId << " (" << shape.size() << " points)" << std::endl;
    }
}

void PreCalculatedCollisionShapes::clear() {
    entityShapes.clear();
    elementShapes.clear();
    expandedEntityConfigs.clear();
    if (DEBUG_LOGS) {
        std::cout << "Cleared all pre-calculated collision shapes" << std::endl;
    }
}

// Check if entity has pre-calculated collision shapes
bool PreCalculatedCollisionShapes::hasEntityShape(const EntityConfiguration& config) const {
    std::string entityKey = generateEntityKey(config);
    return expandedEntityConfigs.find(entityKey + "_elements") != expandedEntityConfigs.end() ||
           expandedEntityConfigs.find(entityKey + "_blocks") != expandedEntityConfigs.end();
}

// Get pre-calculated collision shapes for entity (returns elements and blocks expanded shapes)
std::pair<std::vector<std::pair<float, float>>, std::vector<std::pair<float, float>>> 
PreCalculatedCollisionShapes::getEntityShapes(const EntityConfiguration& config) const {
    std::string entityKey = generateEntityKey(config);
    
    std::vector<std::pair<float, float>> elementsShape = config.collisionShapePoints;
    std::vector<std::pair<float, float>> blocksShape = config.collisionShapePoints;
    
    auto elementsIt = expandedEntityConfigs.find(entityKey + "_elements");
    if (elementsIt != expandedEntityConfigs.end()) {
        elementsShape = elementsIt->second.collisionShapePoints;
    }
    
    auto blocksIt = expandedEntityConfigs.find(entityKey + "_blocks");
    if (blocksIt != expandedEntityConfigs.end()) {
        blocksShape = blocksIt->second.collisionShapePoints;
    }
    
    return std::make_pair(elementsShape, blocksShape);
}

// Check if a position is valid (within bounds and not colliding) - using entity collision shape
bool isPositionValid(float x, float y, const EntityConfiguration& entityConfig, const Map& gameMap, const std::string& excludeInstanceName) {
    // 1. Check map boundaries if offMapAvoidance is enabled
    if (entityConfig.offMapAvoidance) {
        if (wouldEntityCollideWithMapBounds(entityConfig, x, y)) {
            return false; // Entity collision shape would extend outside map boundaries
        }
    }
    
    // 2. Check for collision with elements using granular collision control with safety buffer
    // For pathfinding, we only check avoidance elements as obstacles to route around
    // Collision elements only prevent direct physical overlap during movement, not pathfinding
    if (MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS > 0.0f) {
        // Create a temporary entity config with expanded collision shape for elements
        EntityConfiguration expandedConfig = entityConfig;
        expandedConfig.collisionShapePoints = expandCollisionShape(entityConfig.collisionShapePoints, MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS);
        
        if (wouldEntityCollideWithElementsGranular(expandedConfig, x, y, true)) {
            return false; // Avoidance element detected within safety buffer - pathfinding should find alternate route
        }
    } else {
        if (wouldEntityCollideWithElementsGranular(entityConfig, x, y, true)) {
            return false; // Avoidance element detected - pathfinding should find alternate route
        }
    }
    
    // 3. Check for collision with blocks using granular block collision control with safety buffer
    // For pathfinding, we only check avoidance blocks as obstacles to route around
    // Collision blocks only prevent direct physical overlap during movement, not pathfinding
    if (MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS > 0.0f) {
        // Create a temporary entity config with expanded collision shape for blocks
        EntityConfiguration expandedConfig = entityConfig;
        expandedConfig.collisionShapePoints = expandCollisionShape(entityConfig.collisionShapePoints, MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS);
        
        if (wouldEntityCollideWithBlocksGranular(expandedConfig, x, y, true)) {
            return false; // Avoidance block detected within safety buffer - pathfinding should find alternate route
        }
    } else {
        if (wouldEntityCollideWithBlocksGranular(entityConfig, x, y, true)) {
            return false; // Avoidance block detected - pathfinding should find alternate route
        }
    }
      // 4. Check for collision with other entities using granular entity collision control
    // For pathfinding, we only check avoidance entities as obstacles to route around
    // Collision entities only prevent direct physical overlap during movement, not pathfinding
    if (wouldEntityCollideWithEntitiesGranular(entityConfig, x, y, true, excludeInstanceName)) {
        return false; // Avoidance entity detected - pathfinding should find alternate route
    }
    
    return true;
}

// Optimized position validation using pre-calculated collision shapes
bool isPositionValidOptimized(float x, float y, const EntityConfiguration& entityConfig,
                             const EntityConfiguration& expandedConfigElements,
                             const EntityConfiguration& expandedConfigBlocks,
                             const Map& gameMap, const std::string& excludeInstanceName) {
    // Increment collision check counter for performance monitoring
    g_pathfindingStats.collisionChecks++;
    
    // 1. Check map boundaries if offMapAvoidance is enabled
    if (entityConfig.offMapAvoidance) {
        if (wouldEntityCollideWithMapBounds(entityConfig, x, y)) {
            return false;
        }
    }
    
    // 2. Check elements with pre-expanded collision shape
    if (MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS > 0.0f) {
        if (wouldEntityCollideWithElementsGranular(expandedConfigElements, x, y, true)) {
            return false;
        }
    } else {
        if (wouldEntityCollideWithElementsGranular(entityConfig, x, y, true)) {
            return false;
        }
    }
    
    // 3. Check blocks with pre-expanded collision shape
    if (MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS > 0.0f) {
        if (wouldEntityCollideWithBlocksGranular(expandedConfigBlocks, x, y, true)) {
            return false;
        }
    } else {
        if (wouldEntityCollideWithBlocksGranular(entityConfig, x, y, true)) {
            return false;
        }
    }
      // 4. Check for collision with other entities using granular entity collision control
    // For pathfinding, we only check avoidance entities as obstacles to route around
    // Collision entities only prevent direct physical overlap during movement, not pathfinding
    if (wouldEntityCollideWithEntitiesGranular(entityConfig, x, y, true, excludeInstanceName)) {
        return false;
    }
    
    return true;
}

// Get neighboring positions using entity collision shape detection
// Only allows movement in 8 cardinal and diagonal directions
std::vector<std::pair<float, float>> getNeighbors(float x, float y, float stepSize, const EntityConfiguration& entityConfig, const Map& gameMap, const std::string& excludeInstanceName) {
    std::vector<std::pair<float, float>> neighbors;
    
    // Define possible movement directions (8 directions only - cardinal and diagonal)
    const std::pair<float, float> directions[] = {
        {0.0f, stepSize},         // North (vertical)
        {stepSize, 0.0f},         // East (horizontal)
        {0.0f, -stepSize},        // South (vertical)
        {-stepSize, 0.0f},        // West (horizontal)
        {stepSize, stepSize},     // Northeast (45° diagonal)
        {stepSize, -stepSize},    // Southeast (45° diagonal)
        {-stepSize, -stepSize},   // Southwest (45° diagonal)
        {-stepSize, stepSize}     // Northwest (45° diagonal)
    };    // Add all valid neighbors in the 8 allowed directions
    for (const auto& dir : directions) {
        float newX = x + dir.first;
        float newY = y + dir.second;
        if (isPositionValid(newX, newY, entityConfig, gameMap, excludeInstanceName)) {
            neighbors.push_back({newX, newY});
        }
    }
    return neighbors;
}

// Optimized neighbor generation using pre-calculated collision shapes
std::vector<std::pair<float, float>> getNeighborsOptimized(
    float x, float y, float stepSize,
    const EntityConfiguration& entityConfig,
    const EntityConfiguration& expandedConfigElements,
    const EntityConfiguration& expandedConfigBlocks,
    const Map& gameMap, const std::string& excludeInstanceName = "") {
    
    std::vector<std::pair<float, float>> neighbors;
    
    // 8-directional movement
    const std::pair<float, float> directions[] = {
        {0.0f, stepSize},         // North
        {stepSize, 0.0f},         // East  
        {0.0f, -stepSize},        // South
        {-stepSize, 0.0f},        // West
        {stepSize, stepSize},     // Northeast
        {stepSize, -stepSize},    // Southeast
        {-stepSize, -stepSize},   // Southwest
        {-stepSize, stepSize}     // Northwest
    };
      for (const auto& dir : directions) {
        float newX = x + dir.first;
        float newY = y + dir.second;
        
        if (isPositionValidOptimized(newX, newY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap, excludeInstanceName)) {
            neighbors.push_back({newX, newY});
        }
    }
    
    return neighbors;
}

// Check if a segment follows geometric constraints (horizontal, vertical, or diagonal)
static bool isGeometricSegment(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    
    // Allow some tolerance for floating-point precision
    const float tolerance = 0.001f;
    
    // Check if it's horizontal (dy ≈ 0)
    if (std::abs(dy) < tolerance) {
        return true;
    }
    
    // Check if it's vertical (dx ≈ 0)
    if (std::abs(dx) < tolerance) {
        return true;
    }
    
    // Check if it's diagonal (|dx| ≈ |dy|)
    if (std::abs(std::abs(dx) - std::abs(dy)) < tolerance) {
        return true;
    }
    
    return false;
}

// Simplify path using "string pulling" method with geometric constraints
static void simplifyPath(std::vector<std::pair<float, float>>& path,
                         const EntityConfiguration& entityConfig,
                         const Map& gameMap) {
    if (path.size() <= 2) {
        // For paths of 0, 1, or 2 points, no simplification is needed.
        // However, if it's 2 points, ensure the direct segment is valid and geometric.
        if (path.size() == 2) {
            if (!isSegmentValid(path[0].first, path[0].second, path[1].first, path[1].second,
                                entityConfig, gameMap) ||
                !isGeometricSegment(path[0].first, path[0].second, path[1].first, path[1].second)) {
                // If the direct segment between the two points is invalid or non-geometric,
                // this indicates a potential issue upstream
            }
        }
        return;
    }

    std::vector<std::pair<float, float>> simplifiedPath;
    simplifiedPath.push_back(path[0]); // Always include the starting point

    int currentAnchorIndexInOriginalPath = 0;

    while (static_cast<size_t>(currentAnchorIndexInOriginalPath) < path.size() - 1) {
        int furthestReachableIndexInOriginalPath = currentAnchorIndexInOriginalPath + 1;
        
        // Try to reach as far as possible from the current anchor with geometric constraints
        for (size_t i = currentAnchorIndexInOriginalPath + 2; i < path.size(); ++i) {
            // Test if we can go directly from anchor to this point with geometric and collision constraints
            if (isSegmentValid(path[currentAnchorIndexInOriginalPath].first, path[currentAnchorIndexInOriginalPath].second,
                               path[i].first, path[i].second,
                               entityConfig, gameMap) &&
                isGeometricSegment(path[currentAnchorIndexInOriginalPath].first, path[currentAnchorIndexInOriginalPath].second,
                                   path[i].first, path[i].second)) {
                furthestReachableIndexInOriginalPath = i;
            } else {
                // Can't reach this point directly with constraints, stop here
                break; 
            }
        }
        
        // Add the furthest reachable point and update the anchor
        if (static_cast<size_t>(furthestReachableIndexInOriginalPath) < path.size()) {
            if (simplifiedPath.back().first != path[furthestReachableIndexInOriginalPath].first ||
                simplifiedPath.back().second != path[furthestReachableIndexInOriginalPath].second) {
                simplifiedPath.push_back(path[furthestReachableIndexInOriginalPath]);
            }
        } else {
            break; 
        }
        currentAnchorIndexInOriginalPath = furthestReachableIndexInOriginalPath;
    }
    path = simplifiedPath;
}

// Check if a segment between two points is valid (no collisions along the path)
bool isSegmentValid(float x1, float y1, float x2, float y2, const EntityConfiguration& entityConfig, const Map& gameMap, const std::string& excludeInstanceName) {
    const int numSteps = 10; // Number of steps to check along the segment
    
    for (int i = 0; i <= numSteps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numSteps);
        float x = x1 + t * (x2 - x1);
        float y = y1 + t * (y2 - y1);
          // Check if this point along the segment is valid
        if (!isPositionValid(x, y, entityConfig, gameMap, excludeInstanceName)) {
            return false; // Found invalid position along the segment
        }
    }
    
    return true; // All points along the segment are valid
}

// Optimized pathfinding algorithm with pre-calculated collision shapes
std::vector<std::pair<float, float>> findPathOptimized(
    float startX, float startY,
    float goalX, float goalY,
    const EntityConfiguration& entityConfig,
    const Map& gameMap,
    float stepSize, const std::string& excludeInstanceName) {
    
    // Performance monitoring - start timer
    auto pathfindingStart = std::chrono::high_resolution_clock::now();
    
    // Reset performance counters
    g_pathfindingStats.nodesExplored = 0;
    g_pathfindingStats.collisionChecks = 0;
    
    // Increment total pathfinding calls
    g_pathfindingStats.totalPathfindingCalls++;
    
    // Check if we have pre-calculated collision shapes for optimization
    bool useOptimized = g_collisionCache.hasEntityShape(entityConfig);
    EntityConfiguration expandedConfigElements = entityConfig;
    EntityConfiguration expandedConfigBlocks = entityConfig;
    
    if (useOptimized) {
        auto cachedShapes = g_collisionCache.getEntityShapes(entityConfig);
        expandedConfigElements.collisionShapePoints = cachedShapes.first;
        expandedConfigBlocks.collisionShapePoints = cachedShapes.second;
        
        if (DEBUG_LOGS) {
            std::cout << "Pathfinding: Using pre-calculated collision shapes for optimization" << std::endl;
        }
    } else {
        // Calculate shapes once during this pathfinding call
        if (MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS > 0.0f) {
            expandedConfigElements.collisionShapePoints = expandCollisionShape(
                entityConfig.collisionShapePoints, MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS);
        }
        
        if (MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS > 0.0f) {
            expandedConfigBlocks.collisionShapePoints = expandCollisionShape(
                entityConfig.collisionShapePoints, MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS);
        }
        
        if (DEBUG_LOGS) {
            std::cout << "Pathfinding: Calculated collision shapes on-the-fly" << std::endl;
        }
    }
    
    // Store original intended goal for messages
    float originalGoalX = goalX;
    float originalGoalY = goalY;
      // Check and adjust start position if needed
    bool startValid = useOptimized ? 
        isPositionValidOptimized(startX, startY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap, excludeInstanceName) :
        isPositionValid(startX, startY, entityConfig, gameMap, excludeInstanceName);
        
    if (!startValid) {
        if (DEBUG_LOGS) {
            std::cout << "Pathfinding: Start position (" << startX << ", " << startY << ") is invalid. Searching for nearby valid start..." << std::endl;
        }
        
        bool foundValidStart = false;
        for (float r = 0.0f; r <= 3.0f; r += stepSize) {
            for (float dx = -r; dx <= r; dx += stepSize) {
                for (float dy = -r; dy <= r; dy += stepSize) {
                    if (r > 0.0f && (std::abs(dx) < r && std::abs(dy) < r)) {
                        continue; // For r > 0, only check the perimeter
                    }                    float testX = startX + dx;
                    float testY = startY + dy;
                    
                    bool testValid = useOptimized ?
                        isPositionValidOptimized(testX, testY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap, excludeInstanceName) :
                        isPositionValid(testX, testY, entityConfig, gameMap, excludeInstanceName);
                        
                    if (testValid) {
                        startX = testX;
                        startY = testY;
                        if (DEBUG_LOGS) {
                            std::cout << "Pathfinding: Adjusted start to valid position (" << startX << ", " << startY << ")" << std::endl;
                        }
                        foundValidStart = true;
                        goto end_start_search;
                    }
                }
            }
        }
        end_start_search:;
        
        if (!foundValidStart) {
            if (DEBUG_LOGS) {
                std::cerr << "Pathfinding Error: Could not find a valid start position near original (" << startX << ", " << startY << ")." << std::endl;
            }
            return {};
        }
    }
      // Check and adjust goal position if needed
    bool goalValid = useOptimized ?
        isPositionValidOptimized(goalX, goalY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap, excludeInstanceName) :
        isPositionValid(goalX, goalY, entityConfig, gameMap, excludeInstanceName);
        
    if (!goalValid) {
        if (DEBUG_LOGS) {
            std::cout << "Pathfinding: Goal position (" << goalX << ", " << goalY << ") is invalid, searching for nearby valid position..." << std::endl;
        }
        
        bool foundValidGoal = false;
        const float searchRadius = stepSize * 3.0f;
        const int searchSteps = 8;
        
        for (int radius = 1; radius <= 3 && !foundValidGoal; radius++) {
            for (int step = 0; step < searchSteps && !foundValidGoal; step++) {
                float angle = (2.0f * M_PI * step) / searchSteps;
                float dx = radius * searchRadius * std::cos(angle);
                float dy = radius * searchRadius * std::sin(angle);
                float testX = goalX + dx;
                float testY = goalY + dy;
                  bool testValid = useOptimized ?
                    isPositionValidOptimized(testX, testY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap, excludeInstanceName) :
                    isPositionValid(testX, testY, entityConfig, gameMap, excludeInstanceName);
                    
                if (testValid) {
                    goalX = testX;
                    goalY = testY;
                    foundValidGoal = true;
                    if (DEBUG_LOGS) {
                        std::cout << "Pathfinding: Adjusted goal to valid position (" << goalX << ", " << goalY << ")" << std::endl;
                    }
                }
            }
        }
        
        if (!foundValidGoal) {
            if (DEBUG_LOGS) {
                std::cerr << "Pathfinding Error: Could not find a valid goal position near original (" << originalGoalX << ", " << originalGoalY << ")." << std::endl;
            }
            return {};
        }
    }
    
    // If start and goal are effectively the same, return a direct path
    if (std::abs(startX - goalX) < 0.001f && std::abs(startY - goalY) < 0.001f) {
        return {{startX, startY}};
    }
    
    // A* pathfinding algorithm
    std::priority_queue<Node*, std::vector<Node*>, CompareNodes> openSet;
    std::unordered_map<std::pair<float, float>, Node*, PairHash> allNodes;
    
    Node* startNode = new Node(startX, startY);
    startNode->gCost = 0;
    startNode->hCost = calculateHeuristic(startX, startY, goalX, goalY);
    startNode->fCost = startNode->gCost + startNode->hCost;
    openSet.push(startNode);
    allNodes[{startX, startY}] = startNode;    
    std::unordered_set<std::pair<float, float>, PairHash> closedSet;
    std::vector<std::pair<float, float>> path;
    int iterations = 0;
    // CRITICAL PERFORMANCE FIX: Limit maximum iterations to prevent catastrophic performance
    // Was: (GRID_SIZE * GRID_SIZE) / (stepSize * stepSize) * 4 = up to 4M+ iterations!
    // Now: Fixed maximum of 2000 iterations regardless of grid size
    const int maxIterations = 2000;
      while (!openSet.empty()) {
        iterations++;
        if (iterations > maxIterations) {
            if (DEBUG_LOGS) {
                std::cerr << "Pathfinding: Exceeded maximum iterations (" << maxIterations << "). Aborting search." << std::endl;
                std::cerr << "Pathfinding Details: Start (" << startX << ", " << startY << ") Goal (" << goalX << ", " << goalY << ")" << std::endl;
            }
            for (auto& pair_node : allNodes) { 
                delete pair_node.second; 
            }
            allNodes.clear();
            return {};
        }
        
        // PERFORMANCE FIX: Early termination for unreachable goals
        // If we've explored many nodes and the best node isn't getting closer, abort
        if (iterations > 500 && iterations % 100 == 0) {
            Node* bestNode = openSet.top();
            float distanceToGoal = calculateHeuristic(bestNode->x, bestNode->y, goalX, goalY);
            
            // If the closest node we can find is still very far from the goal, 
            // and we've tried many iterations, the goal is likely unreachable
            if (distanceToGoal > stepSize * 10.0f) {
                if (DEBUG_LOGS) {
                    std::cerr << "Pathfinding: Early termination - goal likely unreachable. Distance: " 
                              << distanceToGoal << " after " << iterations << " iterations." << std::endl;
                }
                for (auto& pair_node : allNodes) { 
                    delete pair_node.second; 
                }
                allNodes.clear();
                return {};
            }
        }
        
        Node* currentNode = openSet.top();
        openSet.pop();
        
        // Check if we've reached the goal
        if (std::abs(currentNode->x - goalX) < stepSize * 0.5f && 
            std::abs(currentNode->y - goalY) < stepSize * 0.5f) {
            
            // Reconstruct path
            Node* temp = currentNode;
            while (temp != nullptr) {
                path.push_back({temp->x, temp->y});
                temp = temp->parent;
            }
            std::reverse(path.begin(), path.end());
            
            // Clean up
            for (auto& pair_node : allNodes) { 
                delete pair_node.second; 
            }
            allNodes.clear();
            
            if (path.empty()) {
                if (std::abs(startX - goalX) < 0.001f && std::abs(startY - goalY) < 0.001f) {
                    path.push_back({startX, startY});
                } else {
                    path.push_back({startX, startY});
                    path.push_back({goalX, goalY});
                }
            } else {
                // Ensure exact start and goal positions
                path[0] = {startX, startY};
                path.back() = {goalX, goalY};
                
                // Simplify the path
                simplifyPath(path, entityConfig, gameMap);
                
                // Re-ensure start/end points after simplification
                if (!path.empty()) {
                    path[0] = {startX, startY};
                    if (path.size() > 1) {
                        path.back() = {goalX, goalY};
                    } else {
                        if (std::abs(startX - goalX) > 0.001f || std::abs(startY - goalY) > 0.001f) {
                            if (path[0].first != goalX || path[0].second != goalY) {
                                path.push_back({goalX, goalY});
                            }
                        }
                    }
                } else {
                    path.push_back({startX, startY});
                    if (std::abs(startX - goalX) > 0.001f || std::abs(startY - goalY) > 0.001f) {
                        path.push_back({goalX, goalY});
                    }
                }
            }
              // Ensure path has at least one point if start and goal are the same
            if (path.empty() && std::abs(startX - goalX) < 0.001f && std::abs(startY - goalY) < 0.001f) {
                path.push_back({startX, startY});
            }
            
            // Performance monitoring - end timer and update stats
            auto pathfindingEnd = std::chrono::high_resolution_clock::now();
            auto pathfindingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(pathfindingEnd - pathfindingStart);
            g_pathfindingStats.totalComputationTimeMs.store(
                g_pathfindingStats.totalComputationTimeMs.load() + static_cast<double>(pathfindingDuration.count())
            );
            g_pathfindingStats.nodesExplored = iterations;
            
            if (DEBUG_LOGS) {
                std::cout << "Pathfinding completed in " << pathfindingDuration.count() << "ms, "
                          << "explored " << iterations << " nodes, "
                          << "performed " << g_pathfindingStats.collisionChecks << " collision checks" << std::endl;
            }
            
            return path;
        }
        
        closedSet.insert({currentNode->x, currentNode->y});
          // Get neighbors using optimized method if available
        std::vector<std::pair<float, float>> neighbors;
        if (useOptimized) {
            neighbors = getNeighborsOptimized(currentNode->x, currentNode->y, stepSize, 
                                            entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap, excludeInstanceName);
        } else {
            neighbors = getNeighbors(currentNode->x, currentNode->y, stepSize, entityConfig, gameMap, excludeInstanceName);
        }
        
        for (const auto& neighborPos : neighbors) {
            if (closedSet.count(neighborPos)) {
                continue;
            }
            
            // Calculate actual distance to neighbor
            float dx = neighborPos.first - currentNode->x;
            float dy = neighborPos.second - currentNode->y;
            float tentativeGCost = currentNode->gCost + std::sqrt(dx * dx + dy * dy);
            
            Node* neighborNode = nullptr;
            auto it = allNodes.find(neighborPos);
            if (it != allNodes.end()) {
                neighborNode = it->second;
            } else {
                neighborNode = new Node(neighborPos.first, neighborPos.second);
                allNodes[neighborPos] = neighborNode;
                neighborNode->gCost = std::numeric_limits<float>::max();
            }
            
            if (tentativeGCost < neighborNode->gCost) {
                neighborNode->parent = currentNode;
                neighborNode->gCost = tentativeGCost;
                neighborNode->hCost = calculateHeuristic(neighborNode->x, neighborNode->y, goalX, goalY);
                neighborNode->fCost = neighborNode->gCost + neighborNode->hCost;
                openSet.push(neighborNode);
            }
        }
    }
      // Clean up if no path found
    for (auto& pair_node : allNodes) { 
        delete pair_node.second; 
    }
    allNodes.clear();
    
    // Performance monitoring - end timer for failed pathfinding
    auto pathfindingEnd = std::chrono::high_resolution_clock::now();
    auto pathfindingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(pathfindingEnd - pathfindingStart);
    g_pathfindingStats.totalComputationTimeMs.store(
        g_pathfindingStats.totalComputationTimeMs.load() + static_cast<double>(pathfindingDuration.count())
    );
    g_pathfindingStats.nodesExplored = iterations;
    
    if (DEBUG_LOGS) {
        std::cerr << "Pathfinding: No path found from (" << startX << ", " << startY 
                  << ") to (" << goalX << ", " << goalY << ") after " << pathfindingDuration.count() 
                  << "ms, explored " << iterations << " nodes" << std::endl;
    }
    
    return {};
}

// Main findPath function - delegates to optimized version
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    const EntityConfiguration& entityConfig,
    const std::string& excludeInstanceName
) {    // PERFORMANCE FIX: Adaptive step size based on distance for better performance
    float distance = std::sqrt((goalX - startX) * (goalX - startX) + (goalY - startY) * (goalY - startY));
    float stepSize;
    if (distance > 20.0f) {
        // Long distance: use larger step size for faster pathfinding
        stepSize = 2.0f;
    } else if (distance > 10.0f) {
        // Medium distance: balanced step size
        stepSize = 1.5f;
    } else {
        // Short distance: smaller step size for precision
        stepSize = 1.0f;
    }
    
    // Auto-caching: Pre-calculate collision shapes for this entity if not cached
    if (!g_collisionCache.hasEntityShape(entityConfig)) {
        if (DEBUG_LOGS) {
            std::cout << "Auto-caching collision shapes for entity during pathfinding..." << std::endl;
        }
        g_collisionCache.preCalculateEntityShape("runtime_entity", entityConfig);
    }
      // Delegate to the optimized version
    return findPathOptimized(startX, startY, goalX, goalY, entityConfig, gameMap, stepSize, excludeInstanceName);
}

// ===== AsyncPathfinder Implementation =====

void AsyncPathfinder::startPathfinding(const PathfindingRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Cancel any ongoing pathfinding
    if (isRunning_) {
        shouldCancel_ = true;
        if (future_.valid()) {
            future_.wait(); // Wait for current pathfinding to finish
        }
    }
    
    // Reset state for new request
    isRunning_ = true;
    shouldCancel_ = false;
    result_.reset();
    
    // Start pathfinding in background thread
    future_ = std::async(std::launch::async, &AsyncPathfinder::findPathAsync, this, request);
    
    if (DEBUG_LOGS) {
        std::cout << "AsyncPathfinder: Started background pathfinding from (" 
                  << request.startX << ", " << request.startY << ") to (" 
                  << request.goalX << ", " << request.goalY << ")" << std::endl;
    }
}

bool AsyncPathfinder::isPathfindingComplete() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isRunning_) {
        return true;
    }
    
    // Check if future is ready
    if (future_.valid() && future_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        // Pathfinding is complete, get the result
        try {
            result_.reset(new PathfindingResult(future_.get()));
            isRunning_ = false;
            return true;
        } catch (const std::exception& e) {
            if (DEBUG_LOGS) {
                std::cerr << "AsyncPathfinder: Exception during pathfinding: " << e.what() << std::endl;
            }
            isRunning_ = false;
            return true; // Consider it complete even if there was an error
        }
    }
    
    return false;
}

std::unique_ptr<PathfindingResult> AsyncPathfinder::getResult() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        return nullptr; // Pathfinding not complete yet
    }
    
    return std::move(result_);
}

void AsyncPathfinder::cancelPathfinding() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        shouldCancel_ = true;
        if (DEBUG_LOGS) {
            std::cout << "AsyncPathfinder: Cancelling ongoing pathfinding" << std::endl;
        }
    }
}

PathfindingResult AsyncPathfinder::findPathAsync(const PathfindingRequest& request) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    PathfindingResult result;
    result.requestId = request.requestId;
    result.success = false;
    
    try {
        // Use pre-calculated collision shapes if available
        EntityConfiguration expandedConfigElements = request.entityConfig;
        EntityConfiguration expandedConfigBlocks = request.entityConfig;
        
        bool useOptimized = false;
        if (g_collisionCache.hasEntityShape(request.entityConfig)) {
            auto cachedShapes = g_collisionCache.getEntityShapes(request.entityConfig);
            expandedConfigElements.collisionShapePoints = cachedShapes.first;
            expandedConfigBlocks.collisionShapePoints = cachedShapes.second;
            useOptimized = true;
        }
          // Modified A* algorithm with cancellation support
        std::vector<std::pair<float, float>> path = findPathWithCancellation(
            request.startX, request.startY,
            request.goalX, request.goalY,
            request.entityConfig,
            request.gameMap,
            request.stepSize,
            useOptimized ? &expandedConfigElements : nullptr,
            useOptimized ? &expandedConfigBlocks : nullptr,
            request.instanceName
        );
        
        result.path = std::move(path);
        result.success = !result.path.empty();
        
        // Update performance stats
        auto endTime = std::chrono::high_resolution_clock::now();
        result.computationTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
          g_pathfindingStats.totalPathfindingCalls++;
        g_pathfindingStats.totalComputationTimeMs.store(
            g_pathfindingStats.totalComputationTimeMs.load() + static_cast<double>(result.computationTimeMs)
        );
        
        if (DEBUG_LOGS) {
            std::cout << "AsyncPathfinder: Completed pathfinding in " << result.computationTimeMs 
                      << "ms, found " << (result.success ? "valid" : "no") << " path with " 
                      << result.path.size() << " points" << std::endl;
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
        
        if (DEBUG_LOGS) {
            std::cerr << "AsyncPathfinder: Error during pathfinding: " << e.what() << std::endl;
        }
    }
    
    return result;
}

// Modified findPath with cancellation support and optimized collision checking
std::vector<std::pair<float, float>> AsyncPathfinder::findPathWithCancellation(
    float startX, float startY,
    float goalX, float goalY,
    const EntityConfiguration& entityConfig,
    const Map& gameMap,
    float stepSize,
    const EntityConfiguration* expandedConfigElements,
    const EntityConfiguration* expandedConfigBlocks, const std::string& excludeInstanceName) {
    
    // Store original goal for debugging
    float originalGoalX = goalX;
    float originalGoalY = goalY;
      // Validate start position
    bool useOptimized = (expandedConfigElements != nullptr && expandedConfigBlocks != nullptr);
    if (useOptimized) {
        if (!isPositionValidOptimized(startX, startY, entityConfig, *expandedConfigElements, *expandedConfigBlocks, gameMap, excludeInstanceName)) {
            if (DEBUG_LOGS) {
                std::cerr << "AsyncPathfinder: Invalid start position (" << startX << ", " << startY << ")" << std::endl;
            }
            return {};
        }
    } else {        if (!isPositionValid(startX, startY, entityConfig, gameMap, excludeInstanceName)) {
            if (DEBUG_LOGS) {
                std::cerr << "AsyncPathfinder: Invalid start position (" << startX << ", " << startY << ")" << std::endl;
            }
            return {};
        }
    }
    
    // Check for cancellation
    if (shouldCancel_) return {};
      // Validate and potentially adjust goal position
    bool goalValid = useOptimized ? 
        isPositionValidOptimized(goalX, goalY, entityConfig, *expandedConfigElements, *expandedConfigBlocks, gameMap, excludeInstanceName) :
        isPositionValid(goalX, goalY, entityConfig, gameMap, excludeInstanceName);
        
    if (!goalValid) {
        if (DEBUG_LOGS) {
            std::cout << "AsyncPathfinder: Goal position (" << goalX << ", " << goalY << ") is invalid, searching for nearby valid position..." << std::endl;
        }
        
        bool foundValidGoal = false;
        const float searchRadius = stepSize * 3.0f;
        const int searchSteps = 8;
        
        for (int radius = 1; radius <= 3 && !foundValidGoal; radius++) {
            for (int step = 0; step < searchSteps && !foundValidGoal; step++) {
                if (shouldCancel_) return {};
                
                float angle = (2.0f * M_PI * step) / searchSteps;
                float dx = radius * searchRadius * std::cos(angle);
                float dy = radius * searchRadius * std::sin(angle);
                float testX = goalX + dx;
                float testY = goalY + dy;
                  bool testValid = useOptimized ?
                    isPositionValidOptimized(testX, testY, entityConfig, *expandedConfigElements, *expandedConfigBlocks, gameMap, excludeInstanceName) :
                    isPositionValid(testX, testY, entityConfig, gameMap, excludeInstanceName);
                    
                if (testValid) {
                    goalX = testX;
                    goalY = testY;
                    foundValidGoal = true;
                    if (DEBUG_LOGS) {
                        std::cout << "AsyncPathfinder: Adjusted goal to valid position (" << goalX << ", " << goalY << ")" << std::endl;
                    }
                }
            }
        }
        
        if (!foundValidGoal) {
            if (DEBUG_LOGS) {
                std::cerr << "AsyncPathfinder: Could not find a valid goal position near (" << originalGoalX << ", " << originalGoalY << ")" << std::endl;
            }
            return {};
        }
    }
    
    // If start and goal are the same, return direct path
    if (std::abs(startX - goalX) < 0.001f && std::abs(startY - goalY) < 0.001f) {
        return {{startX, startY}};
    }
    
    // A* algorithm with cancellation checks
    std::priority_queue<Node*, std::vector<Node*>, CompareNodes> openSet;
    std::unordered_map<std::pair<float, float>, Node*, PairHash> allNodes;
    
    Node* startNode = new Node(startX, startY);
    startNode->gCost = 0;
    startNode->hCost = calculateHeuristic(startX, startY, goalX, goalY);
    startNode->fCost = startNode->gCost + startNode->hCost;
    openSet.push(startNode);
    allNodes[{startX, startY}] = startNode;
      std::unordered_set<std::pair<float, float>, PairHash> closedSet;
    std::vector<std::pair<float, float>> path;
    int iterations = 0;
    // CRITICAL PERFORMANCE FIX: Limit maximum iterations to prevent catastrophic performance  
    // Was: (GRID_SIZE * GRID_SIZE) / (stepSize * stepSize) * 4 = up to 4M+ iterations!
    // Now: Fixed maximum of 2000 iterations regardless of grid size
    const int maxIterations = 2000;
      while (!openSet.empty()) {
        // Check for cancellation every few iterations
        if (iterations % 50 == 0 && shouldCancel_) {
            // Clean up and return empty path
            for (auto& pair_node : allNodes) { 
                delete pair_node.second; 
            }
            allNodes.clear();
            return {};
        }
        
        iterations++;
        if (iterations > maxIterations) {
            if (DEBUG_LOGS) {
                std::cerr << "AsyncPathfinder: Exceeded maximum iterations (" << maxIterations << ")" << std::endl;
            }
            for (auto& pair_node : allNodes) { 
                delete pair_node.second; 
            }
            allNodes.clear();
            return {};
        }
        
        // PERFORMANCE FIX: Early termination for unreachable goals
        // If we've explored many nodes and the best node isn't getting closer, abort
        if (iterations > 500 && iterations % 100 == 0) {
            Node* bestNode = openSet.top();
            float distanceToGoal = calculateHeuristic(bestNode->x, bestNode->y, goalX, goalY);
            
            // If the closest node we can find is still very far from the goal, 
            // and we've tried many iterations, the goal is likely unreachable
            if (distanceToGoal > stepSize * 10.0f) {
                if (DEBUG_LOGS) {
                    std::cerr << "AsyncPathfinder: Early termination - goal likely unreachable. Distance: " 
                              << distanceToGoal << " after " << iterations << " iterations." << std::endl;
                }
                for (auto& pair_node : allNodes) { 
                    delete pair_node.second; 
                }
                allNodes.clear();
                return {};
            }
        }
        
        Node* currentNode = openSet.top();
        openSet.pop();
        
        // Check if we've reached the goal
        if (std::abs(currentNode->x - goalX) < stepSize * 0.5f && 
            std::abs(currentNode->y - goalY) < stepSize * 0.5f) {
            
            // Reconstruct path
            Node* temp = currentNode;
            while (temp != nullptr) {
                path.push_back({temp->x, temp->y});
                temp = temp->parent;
            }
            std::reverse(path.begin(), path.end());
            
            // Clean up
            for (auto& pair_node : allNodes) { 
                delete pair_node.second; 
            }
            allNodes.clear();
            
            // Ensure exact start and goal positions
            if (!path.empty()) {
                path[0] = {startX, startY};
                path.back() = {goalX, goalY};
                
                // Simplify the path
                simplifyPath(path, entityConfig, gameMap);
                
                // Re-ensure start/end points after simplification
                if (!path.empty()) {
                    path[0] = {startX, startY};
                    if (path.size() > 1) {
                        path.back() = {goalX, goalY};
                    }
                }
            }
            
            return path;
        }
        
        closedSet.insert({currentNode->x, currentNode->y});
          // Get neighbors using optimized validation if available
        std::vector<std::pair<float, float>> neighbors;
        if (useOptimized) {
            neighbors = getNeighborsOptimized(currentNode->x, currentNode->y, stepSize, 
                                            entityConfig, *expandedConfigElements, *expandedConfigBlocks, gameMap, excludeInstanceName);
        } else {
            neighbors = getNeighbors(currentNode->x, currentNode->y, stepSize, entityConfig, gameMap, excludeInstanceName);
        }
        
        for (const auto& neighborPos : neighbors) {
            if (closedSet.count(neighborPos)) {
                continue;
            }
            
            // Calculate distance to neighbor
            float dx = neighborPos.first - currentNode->x;
            float dy = neighborPos.second - currentNode->y;
            float tentativeGCost = currentNode->gCost + std::sqrt(dx * dx + dy * dy);
            
            Node* neighborNode = nullptr;
            auto it = allNodes.find(neighborPos);
            if (it != allNodes.end()) {
                neighborNode = it->second;
            } else {
                neighborNode = new Node(neighborPos.first, neighborPos.second);
                allNodes[neighborPos] = neighborNode;
                neighborNode->gCost = std::numeric_limits<float>::max();
            }
            
            if (tentativeGCost < neighborNode->gCost) {
                neighborNode->parent = currentNode;
                neighborNode->gCost = tentativeGCost;
                neighborNode->hCost = calculateHeuristic(neighborNode->x, neighborNode->y, goalX, goalY);
                neighborNode->fCost = neighborNode->gCost + neighborNode->hCost;
                openSet.push(neighborNode);
            }
        }
    }
    
    // Clean up if no path found
    for (auto& pair_node : allNodes) { 
        delete pair_node.second; 
    }
    allNodes.clear();
    
    if (DEBUG_LOGS) {
        std::cerr << "AsyncPathfinder: No path found from (" << startX << ", " << startY 
                  << ") to (" << goalX << ", " << goalY << ")" << std::endl;
    }
    
    return {};
}

// Standalone async pathfinding function
std::future<PathfindingResult> findPathAsync(const PathfindingRequest& request) {
    return std::async(std::launch::async, [request]() -> PathfindingResult {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        PathfindingResult result;
        result.requestId = request.requestId;
        result.success = false;
        
        try {            // Use the regular synchronous pathfinding for now
            // In a more advanced implementation, this could use a separate async pathfinder
            std::vector<std::pair<float, float>> path = findPathOptimized(
                request.startX, request.startY,
                request.goalX, request.goalY,
                request.entityConfig,
                request.gameMap,
                request.stepSize,
                request.instanceName
            );
            
            result.path = std::move(path);
            result.success = !result.path.empty();
            
            auto endTime = std::chrono::high_resolution_clock::now();
            result.computationTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            
            if (DEBUG_LOGS) {
                std::cout << "Async pathfinding completed in " << result.computationTimeMs 
                          << "ms, found " << (result.success ? "valid" : "no") << " path" << std::endl;
            }
            
        } catch (const std::exception& e) {
            result.success = false;
            result.errorMessage = e.what();
            
            if (DEBUG_LOGS) {
                std::cerr << "Async pathfinding error: " << e.what() << std::endl;
            }
        }
        
        return result;
    });
}
