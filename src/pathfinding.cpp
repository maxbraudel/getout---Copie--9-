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

// Custom comparison for priority queue
struct CompareNodes {
    bool operator()(const Node* a, const Node* b) const {
        return a->fCost > b->fCost; // Lower fCost has higher priority
    }
};

// Calculate heuristic (Manhattan distance)
float calculateHeuristic(int x1, int y1, int x2, int y2) {
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

// Check if a position is valid (within bounds and not colliding) - using entity collision shape
bool isPositionValid(float x, float y, const EntityConfiguration& entityConfig, const Map& gameMap) {
    // 1. Check map boundaries if offMapAvoidance is enabled
    if (entityConfig.offMapAvoidance) {
        if (wouldEntityCollideWithMapBounds(entityConfig, x, y)) {
            return false; // Entity collision shape would extend outside map boundaries
        }
    }
    
    // 2. Check for collision with elements using granular collision control
    // For pathfinding, we only check avoidance elements as obstacles to route around
    // Collision elements only prevent direct physical overlap during movement, not pathfinding
    if (wouldEntityCollideWithElementsGranular(entityConfig, x, y, true)) {
        return false; // Avoidance element detected - pathfinding should find alternate route
    }

    return true;
}

// The integer version of position validation using entity configuration
bool isPositionValid(int x, int y, const EntityConfiguration& entityConfig, const Map& gameMap) {
    return isPositionValid(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, 
                          entityConfig, gameMap);
}

// Get neighboring positions using entity collision shape detection
std::vector<std::pair<int, int>> getNeighbors(int x, int y, const EntityConfiguration& entityConfig, const Map& gameMap) {
    std::vector<std::pair<int, int>> neighbors;
    
    // Define possible movement directions (8 directions)
    const std::pair<int, int> directions[] = {
        {0, 1},   // North
        {1, 0},   // East
        {0, -1},  // South
        {-1, 0},  // West
        {1, 1},   // Northeast
        {1, -1},  // Southeast
        {-1, -1}, // Southwest
        {-1, 1}   // Northwest
    };

    // Add all possible neighbors
    for (const auto& dir : directions) {
        int newX = x + dir.first;
        int newY = y + dir.second;
        if (isPositionValid(newX, newY, entityConfig, gameMap)) {
            neighbors.push_back({newX, newY});
        }
    }
    return neighbors;
}


// Simplify path using "string pulling" method, ensuring segments are valid - simplified for element collision only
static void simplifyPath(std::vector<std::pair<float, float>>& path,
                         const EntityConfiguration& entityConfig,
                         const Map& gameMap) {
    if (path.size() <= 2) {
        // For paths of 0, 1, or 2 points, no simplification is typically needed.
        // However, if it's 2 points, ensure the direct segment is valid.
        if (path.size() == 2) {
            if (!isSegmentValid(path[0].first, path[0].second, path[1].first, path[1].second,
                                entityConfig, gameMap)) {
                // If the direct segment between the two points is invalid,
                // this indicates a potential issue upstream
            }
        }
        return;
    }

    std::vector<std::pair<float, float>> simplifiedPath;
    simplifiedPath.push_back(path[0]); // Always include the starting point

    int currentAnchorIndexInOriginalPath = 0;
    const float epsilon = 0.001f; // For floating point comparisons

    while (static_cast<size_t>(currentAnchorIndexInOriginalPath) < path.size() - 1) {
        int furthestReachableIndexInOriginalPath = currentAnchorIndexInOriginalPath + 1;
        for (size_t i = currentAnchorIndexInOriginalPath + 2; i < path.size(); ++i) {
            float dx = path[i].first - path[currentAnchorIndexInOriginalPath].first;
            float dy = path[i].second - path[currentAnchorIndexInOriginalPath].second;

            bool isHorizontal = std::abs(dy) < epsilon && std::abs(dx) > epsilon;
            bool isVertical = std::abs(dx) < epsilon && std::abs(dy) > epsilon;
            // Check for non-zero dx/dy for diagonal to ensure it's a line, not a point.
            bool isDiagonal = std::abs(std::abs(dx) - std::abs(dy)) < epsilon && std::abs(dx) > epsilon;
              if (isHorizontal || isVertical || isDiagonal) {
                if (isSegmentValid(path[currentAnchorIndexInOriginalPath].first, path[currentAnchorIndexInOriginalPath].second,
                                   path[i].first, path[i].second,
                                   entityConfig, gameMap)) {
                    furthestReachableIndexInOriginalPath = i;
                } else {
                    // Segment is of allowed type but collides
                    break; 
                }
            } else {
                // Segment is not Horizontal, Vertical, or perfect Diagonal
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

// Find path using A* algorithm - simplified for element collision only
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    const EntityConfiguration& entityConfig
) {
    // Store original intended goal for messages, and derive initial cell indices
    float originalGoalX = goalX;
    float originalGoalY = goalY;
    int initialStartNodeX = static_cast<int>(startX);
    int initialStartNodeY = static_cast<int>(startY);
    int initialGoalNodeX = static_cast<int>(goalX);
    int initialGoalNodeY = static_cast<int>(goalY);    // Check and adjust start position
    if (!isPositionValid(startX, startY, entityConfig, gameMap)) {
        std::cout << "Pathfinding: Start position (" << startX << ", " << startY << ") is invalid. Searching for a nearby valid start..." << std::endl;
        bool foundValidStart = false;
        for (int r = 0; r <= 3; ++r) { // Search radius: 0 (original cell), then 1, 2, 3
            for (int dx_offset = -r; dx_offset <= r; ++dx_offset) {
                for (int dy_offset = -r; dy_offset <= r; ++dy_offset) {
                    if (r > 0 && (std::abs(dx_offset) < r && std::abs(dy_offset) < r)) {
                        continue; // For r > 0, only check the perimeter of the current search square
                    }
                    float testX = static_cast<float>(initialStartNodeX + dx_offset) + 0.5f;
                    float testY = static_cast<float>(initialStartNodeY + dy_offset) + 0.5f;
                    if (isPositionValid(testX, testY, entityConfig, gameMap)) {
                        startX = testX; // Update startX to the valid cell center
                        startY = testY; // Update startY to the valid cell center
                        std::cout << "Pathfinding: Adjusted start to valid cell center (" << startX << ", " << startY << ")" << std::endl;
                        foundValidStart = true;
                        goto end_start_search;
                    }
                }
            }
        }
        end_start_search:;
        if (!foundValidStart) {
            std::cerr << "Pathfinding Error: Could not find a valid start position near original (" << initialStartNodeX << ".5, " << initialStartNodeY << ".5)." << std::endl;
            return {};
        }
    }    // Check and adjust goal position
    if (!isPositionValid(goalX, goalY, entityConfig, gameMap)) {
        std::cout << "Pathfinding: Goal position (" << originalGoalX << ", " << originalGoalY << ") is invalid. Searching for a nearby valid goal..." << std::endl;
        bool foundValidGoal = false;
        for (int r = 0; r <= 3; ++r) { // Search radius
            for (int dx_offset = -r; dx_offset <= r; ++dx_offset) {
                for (int dy_offset = -r; dy_offset <= r; ++dy_offset) {
                     if (r > 0 && (std::abs(dx_offset) < r && std::abs(dy_offset) < r)) {
                        continue; 
                    }
                    float testX = static_cast<float>(initialGoalNodeX + dx_offset) + 0.5f;
                    float testY = static_cast<float>(initialGoalNodeY + dy_offset) + 0.5f;
                    if (isPositionValid(testX, testY, entityConfig, gameMap)) {
                        goalX = testX; // Update goalX to the valid cell center
                        goalY = testY; // Update goalY to the valid cell center
                        std::cout << "Pathfinding: Adjusted goal to valid cell center (" << goalX << ", " << goalY << ")" << std::endl;
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

    // Now, derive the A* start/goal nodes from the (potentially adjusted) startX/Y and goalX/Y
    int startNodeX = static_cast<int>(startX);
    int startNodeY = static_cast<int>(startY);
    int goalNodeX = static_cast<int>(goalX);
    int goalNodeY = static_cast<int>(goalY);

    // If, after adjustment, start and goal are effectively the same, return a direct path
    if (std::abs(startX - goalX) < 0.001f && std::abs(startY - goalY) < 0.001f) {
        return {{startX, startY}};
    }
    
    std::priority_queue<Node*, std::vector<Node*>, CompareNodes> openSet;
    std::unordered_map<std::pair<int, int>, Node*, PairHash> allNodes; 

    Node* startNode = new Node(startNodeX, startNodeY);
    startNode->gCost = 0;
    startNode->hCost = calculateHeuristic(startNodeX, startNodeY, goalNodeX, goalNodeY);
    startNode->fCost = startNode->gCost + startNode->hCost;
    openSet.push(startNode);
    allNodes[{startNodeX, startNodeY}] = startNode;

    std::unordered_set<std::pair<int, int>, PairHash> closedSet;
    std::vector<std::pair<float, float>> path;
    int iterations = 0;
    const int maxIterations = GRID_SIZE * GRID_SIZE * 4; // Increased max iterations slightly

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

        if (currentNode->x == goalNodeX && currentNode->y == goalNodeY) {
            Node* temp = currentNode;
            while (temp != nullptr) {
                path.push_back({static_cast<float>(temp->x) + 0.5f, static_cast<float>(temp->y) + 0.5f});
                temp = temp->parent;
            }
            std::reverse(path.begin(), path.end());
            
            for (auto& pair_node : allNodes) { delete pair_node.second; }
            allNodes.clear();

            if (path.empty()) { 
                 if (std::abs(startX - goalX) < 0.001f && std::abs(startY - goalY) < 0.001f) path.push_back({startX, startY});
                 else { path.push_back({startX, startY}); path.push_back({goalX, goalY}); } 
            } else {
                // Path has at least one point (a cell center)
                // Snap endpoints to the (guaranteed valid or adjusted) startX/Y and goalX/Y BEFORE simplification
                path[0] = {startX, startY};
                path.back() = {goalX, goalY};                // Simplify the path using the new robust method (element collision only)
                simplifyPath(path, entityConfig, gameMap);

                // Ensure simplification didn't break start/end points, or if path became very short
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

        for (const auto& neighborPos : getNeighbors(currentNode->x, currentNode->y, entityConfig, gameMap)) {
            if (closedSet.count(neighborPos)) {
                continue;
            }

            float tentativeGCost = currentNode->gCost + 1.0f; 

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
                neighborNode->hCost = calculateHeuristic(neighborNode->x, neighborNode->y, goalNodeX, goalNodeY);
                neighborNode->fCost = neighborNode->gCost + neighborNode->hCost;
                openSet.push(neighborNode);
            }
        }
    }

    // Clean up allocated nodes if no path found
    for (auto& pair_node : allNodes) { delete pair_node.second; }
    allNodes.clear();
    
    // Enhanced Debugging Output (if no path found)
    std::cerr << "-------------------------------------------------\\" << std::endl;
    std::cerr << "Pathfinding: No path found! Details:" << std::endl;
    std::cerr << "  Start (final used): (" << startX << ", " << startY << "), Node: (" << startNodeX << ", " << startNodeY << ")" << std::endl;
    std::cerr << "  Goal (final used):  (" << goalX << ", " << goalY << "), Node: (" << goalNodeX << ", " << goalNodeY << ")" << std::endl;
    std::cerr << "  Original Goal:      (" << originalGoalX << ", " << originalGoalY << ")" << std::endl;
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
    
    std::cerr << "  Non-Traversable Blocks for this entity: (None - entities only check element collisions)" << std::endl;
    std::cerr << "-------------------------------------------------\\" << std::endl;

    return {}; // No path found
}
