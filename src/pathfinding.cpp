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
#include <limits>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// Global variables for minimum distance from avoidance objects
float MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS = 0.0f;  // Default: no buffer
float MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS = 0.0f; // Default: no buffer

// Global instances for performance optimization
PathfindingStats g_pathfindingStats;
PreCalculatedCollisionShapes g_collisionCache;
AsyncPathfinder g_asyncPathfinder;

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
bool isPositionValid(float x, float y, const EntityConfiguration& entityConfig, const Map& gameMap) {
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
    
    return true;
}

// Optimized position validation using pre-calculated collision shapes
bool isPositionValidOptimized(float x, float y, const EntityConfiguration& entityConfig,
                             const EntityConfiguration& expandedConfigElements,
                             const EntityConfiguration& expandedConfigBlocks,
                             const Map& gameMap) {
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
    
    return true;
}

// Get neighboring positions using entity collision shape detection
// Only allows movement in 8 cardinal and diagonal directions
std::vector<std::pair<float, float>> getNeighbors(float x, float y, float stepSize, const EntityConfiguration& entityConfig, const Map& gameMap) {
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
    };

    // Add all valid neighbors in the 8 allowed directions
    for (const auto& dir : directions) {
        float newX = x + dir.first;
        float newY = y + dir.second;
        if (isPositionValid(newX, newY, entityConfig, gameMap)) {
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
    const Map& gameMap) {
    
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
        
        if (isPositionValidOptimized(newX, newY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap)) {
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
bool isSegmentValid(float x1, float y1, float x2, float y2, const EntityConfiguration& entityConfig, const Map& gameMap) {
    const int numSteps = 10; // Number of steps to check along the segment
    
    for (int i = 0; i <= numSteps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numSteps);
        float x = x1 + t * (x2 - x1);
        float y = y1 + t * (y2 - y1);
        
        // Check if this point along the segment is valid
        if (!isPositionValid(x, y, entityConfig, gameMap)) {
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
    float stepSize) {
    
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
        isPositionValidOptimized(startX, startY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap) :
        isPositionValid(startX, startY, entityConfig, gameMap);
        
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
                    }
                    float testX = startX + dx;
                    float testY = startY + dy;
                    
                    bool testValid = useOptimized ?
                        isPositionValidOptimized(testX, testY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap) :
                        isPositionValid(testX, testY, entityConfig, gameMap);
                        
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
        isPositionValidOptimized(goalX, goalY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap) :
        isPositionValid(goalX, goalY, entityConfig, gameMap);
        
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
                    isPositionValidOptimized(testX, testY, entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap) :
                    isPositionValid(testX, testY, entityConfig, gameMap);
                    
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
    const int maxIterations = static_cast<int>((GRID_SIZE * GRID_SIZE) / (stepSize * stepSize)) * 4;
    
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
                                            entityConfig, expandedConfigElements, expandedConfigBlocks, gameMap);
        } else {
            neighbors = getNeighbors(currentNode->x, currentNode->y, stepSize, entityConfig, gameMap);
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
    const EntityConfiguration& entityConfig
) {
    // Use larger step size for much better performance (4x fewer nodes to explore)
    const float stepSize = 1.0f; // Increased from 0.5f to 1.0f
    
    // Auto-caching: Pre-calculate collision shapes for this entity if not cached
    if (!g_collisionCache.hasEntityShape(entityConfig)) {
        if (DEBUG_LOGS) {
            std::cout << "Auto-caching collision shapes for entity during pathfinding..." << std::endl;
        }
        g_collisionCache.preCalculateEntityShape("runtime_entity", entityConfig);
    }
    
    // Delegate to the optimized version
    return findPathOptimized(startX, startY, goalX, goalY, entityConfig, gameMap, stepSize);
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
            useOptimized ? &expandedConfigBlocks : nullptr
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
    const EntityConfiguration* expandedConfigBlocks) {
    
    // Store original goal for debugging
    float originalGoalX = goalX;
    float originalGoalY = goalY;
    
    // Validate start position
    bool useOptimized = (expandedConfigElements != nullptr && expandedConfigBlocks != nullptr);
    if (useOptimized) {
        if (!isPositionValidOptimized(startX, startY, entityConfig, *expandedConfigElements, *expandedConfigBlocks, gameMap)) {
            if (DEBUG_LOGS) {
                std::cerr << "AsyncPathfinder: Invalid start position (" << startX << ", " << startY << ")" << std::endl;
            }
            return {};
        }
    } else {
        if (!isPositionValid(startX, startY, entityConfig, gameMap)) {
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
        isPositionValidOptimized(goalX, goalY, entityConfig, *expandedConfigElements, *expandedConfigBlocks, gameMap) :
        isPositionValid(goalX, goalY, entityConfig, gameMap);
        
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
                    isPositionValidOptimized(testX, testY, entityConfig, *expandedConfigElements, *expandedConfigBlocks, gameMap) :
                    isPositionValid(testX, testY, entityConfig, gameMap);
                    
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
    const int maxIterations = static_cast<int>((GRID_SIZE * GRID_SIZE) / (stepSize * stepSize)) * 4;
    
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
                
                // Simplify path
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
                                            entityConfig, *expandedConfigElements, *expandedConfigBlocks, gameMap);
        } else {
            neighbors = getNeighbors(currentNode->x, currentNode->y, stepSize, entityConfig, gameMap);
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
        
        try {
            // Use the regular synchronous pathfinding for now
            // In a more advanced implementation, this could use a separate async pathfinder
            std::vector<std::pair<float, float>> path = findPathOptimized(
                request.startX, request.startY,
                request.goalX, request.goalY,
                request.entityConfig,
                request.gameMap,
                request.stepSize
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
