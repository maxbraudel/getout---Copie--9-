#include "pathfinding.h"
#include "entities.h" // For EntityConfiguration
#include "globals.h" // For GRID_SIZE
#include "collision.h" // For collision detection
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits> // Added to resolve std::numeric_limits errors

// Global variables for minimum distance from avoidance objects
float MIN_DISTANCE_FROM_AVOIDANCE_BLOCKS = 0.4f;  // Default: no buffer
float MIN_DISTANCE_FROM_AVOIDANCE_ELEMENTS = 0.4f; // Default: no buffer

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
std::vector<std::pair<float, float>> expandCollisionShape(const std::vector<std::pair<float, float>>& originalShape, float expandDistance) {
    if (expandDistance <= 0.0f || originalShape.empty()) {
        return originalShape; // No expansion needed
    }
    
    std::vector<std::pair<float, float>> expandedShape;
    
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
        currentAnchorIndexInOriginalPath = furthestReachableIndexInOriginalPath;    }
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

// Find path using A* algorithm with true floating-point coordinates
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    const EntityConfiguration& entityConfig
) {
    // Store original intended goal for messages
    float originalGoalX = goalX;
    float originalGoalY = goalY;
    
    // Define step size for pathfinding resolution (smaller = more precise but slower)
    const float stepSize = 0.5f;
    
    // Check and adjust start position if needed
    if (!isPositionValid(startX, startY, entityConfig, gameMap)) {
        std::cout << "Pathfinding: Start position (" << startX << ", " << startY << ") is invalid. Searching for a nearby valid start..." << std::endl;
        bool foundValidStart = false;
        for (float r = 0.0f; r <= 3.0f; r += stepSize) {
            for (float dx = -r; dx <= r; dx += stepSize) {
                for (float dy = -r; dy <= r; dy += stepSize) {
                    if (r > 0.0f && (std::abs(dx) < r && std::abs(dy) < r)) {
                        continue; // For r > 0, only check the perimeter
                    }
                    float testX = startX + dx;
                    float testY = startY + dy;
                    if (isPositionValid(testX, testY, entityConfig, gameMap)) {
                        startX = testX;
                        startY = testY;
                        std::cout << "Pathfinding: Adjusted start to valid position (" << startX << ", " << startY << ")" << std::endl;
                        foundValidStart = true;
                        goto end_start_search;
                    }
                }
            }
        }
        end_start_search:;
        if (!foundValidStart) {
            std::cerr << "Pathfinding Error: Could not find a valid start position near original (" << originalGoalX << ", " << originalGoalY << ")." << std::endl;
            return {};
        }
    }
    
    // Check and adjust goal position if needed
    if (!isPositionValid(goalX, goalY, entityConfig, gameMap)) {
        std::cout << "Pathfinding: Goal position (" << originalGoalX << ", " << originalGoalY << ") is invalid. Searching for a nearby valid goal..." << std::endl;
        bool foundValidGoal = false;
        for (float r = 0.0f; r <= 3.0f; r += stepSize) {
            for (float dx = -r; dx <= r; dx += stepSize) {
                for (float dy = -r; dy <= r; dy += stepSize) {
                    if (r > 0.0f && (std::abs(dx) < r && std::abs(dy) < r)) {
                        continue;
                    }
                    float testX = goalX + dx;
                    float testY = goalY + dy;
                    if (isPositionValid(testX, testY, entityConfig, gameMap)) {
                        goalX = testX;
                        goalY = testY;
                        std::cout << "Pathfinding: Adjusted goal to valid position (" << goalX << ", " << goalY << ")" << std::endl;
                        foundValidGoal = true;
                        goto end_goal_search;
                    }
                }
            }
        }
        end_goal_search:;
        if (!foundValidGoal) {
            std::cerr << "Pathfinding Error: Could not find a valid goal position near original (" << originalGoalX << ", " << originalGoalY << ")." << std::endl;
            return {};
        }
    }

    // If start and goal are effectively the same, return a direct path
    if (std::abs(startX - goalX) < 0.001f && std::abs(startY - goalY) < 0.001f) {
        return {{startX, startY}};
    }    
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
    const int maxIterations = static_cast<int>((GRID_SIZE * GRID_SIZE) / (stepSize * stepSize)) * 4; // Scale based on step size

    while (!openSet.empty()) {
        iterations++;
        if (iterations > maxIterations) {
            std::cerr << "Pathfinding: Exceeded maximum iterations (" << maxIterations << "). Aborting search." << std::endl;
            std::cerr << "Pathfinding Details: Start (" << startX << ", " << startY << ") Goal (" << goalX << ", " << goalY << ")" << std::endl;
            std::cerr << "Pathfinding Details: Open set size: " << openSet.size() << ", Closed set size: " << closedSet.size() << std::endl;
            if (!openSet.empty()) {
                Node* currentTop = openSet.top();
                std::cerr << "Pathfinding Details: Current top of openSet: (" << currentTop->x << ", " << currentTop->y << ") fCost: " << currentTop->fCost << std::endl;
            }
            for (auto& pair_node : allNodes) { delete pair_node.second; }
            allNodes.clear();
            return {};
        }

        Node* currentNode = openSet.top();
        openSet.pop();

        // Check if we've reached the goal (within a small tolerance for floating-point comparison)
        if (std::abs(currentNode->x - goalX) < stepSize * 0.5f && std::abs(currentNode->y - goalY) < stepSize * 0.5f) {
            Node* temp = currentNode;
            while (temp != nullptr) {
                path.push_back({temp->x, temp->y});
                temp = temp->parent;
            }
            std::reverse(path.begin(), path.end());
            
            for (auto& pair_node : allNodes) { delete pair_node.second; }
            allNodes.clear();

            if (path.empty()) { 
                 if (std::abs(startX - goalX) < 0.001f && std::abs(startY - goalY) < 0.001f) path.push_back({startX, startY});
                 else { path.push_back({startX, startY}); path.push_back({goalX, goalY}); } 
            } else {
                // Ensure exact start and goal positions
                path[0] = {startX, startY};
                path.back() = {goalX, goalY};
                
                // Simplify the path using the new robust method
                simplifyPath(path, entityConfig, gameMap);

                // Ensure simplification didn't break start/end points
                if (!path.empty()) {
                    path[0] = {startX, startY}; // Re-snap start
                    if (path.size() > 1) {
                        path.back() = {goalX, goalY}; // Re-snap end
                    } else {
                        if (std::abs(startX - goalX) > 0.001f || std::abs(startY - goalY) > 0.001f) {
                            if (path[0].first != goalX || path[0].second != goalY) {
                                path.push_back({goalX, goalY});
                            }
                        }
                    }
                } else { // Path became empty after simplification
                    path.push_back({startX, startY}); 
                    if (std::abs(startX - goalX) > 0.001f || std::abs(startY - goalY) > 0.001f) { 
                         path.push_back({goalX, goalY});
                    }
                }
            }
             // Ensure path has at least one point if start and goal are the same.
            if (path.empty() && std::abs(startX - goalX) < 0.001f && std::abs(startY - goalY) < 0.001f) {
                path.push_back({startX, startY});
            }

            return path;
        }

        closedSet.insert({currentNode->x, currentNode->y});

        for (const auto& neighborPos : getNeighbors(currentNode->x, currentNode->y, stepSize, entityConfig, gameMap)) {
            if (closedSet.count(neighborPos)) {
                continue;
            }

            // Calculate actual distance to neighbor (Euclidean distance for floating-point)
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
    }    // Clean up allocated nodes if no path found
    for (auto& pair_node : allNodes) { delete pair_node.second; }
    allNodes.clear();
    
    // Enhanced Debugging Output (if no path found)
    std::cerr << "-------------------------------------------------\\" << std::endl;
    std::cerr << "Pathfinding: No path found! Details:" << std::endl;
    std::cerr << "  Start (final used): (" << startX << ", " << startY << ")" << std::endl;
    std::cerr << "  Goal (final used):  (" << goalX << ", " << goalY << ")" << std::endl;
    std::cerr << "  Original Goal:      (" << originalGoalX << ", " << originalGoalY << ")" << std::endl;
    std::cerr << "  Step Size:          " << stepSize << std::endl;
    std::cerr << "  Open Set Status:    " << (openSet.empty() ? "Empty" : "Not Empty (Error in logic?)") << std::endl;
    std::cerr << "  Nodes Explored (Closed Set Size): " << closedSet.size() << std::endl;
    std::cerr << "  Number of Iterations: " << iterations << std::endl;

    if (closedSet.size() < 20 && closedSet.size() > 0) {
        std::cerr << "  Sample of Closed Set Nodes (x, y):" << std::endl;
        int count = 0;
        for (const auto& node_pair : closedSet) {
            std::cerr << "    (" << node_pair.first << ", " << node_pair.second << ")" << std::endl;
            if (++count >= 5) break; 
        }
    } else if (closedSet.size() >= 20) {
        std::cerr << "  Closed set is large, not printing samples here." << std::endl;
    }
    
    std::cerr << "  Using floating-point pathfinding with step size: " << stepSize << std::endl;
    std::cerr << "-------------------------------------------------\\" << std::endl;

    return {}; // No path found
}
