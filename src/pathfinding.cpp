#include "pathfinding.h"
#include "globals.h" // For GRID_SIZE
#include "collision.h" // For collision detection
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <cmath>
#include <algorithm>
#include <iostream>

// Maximum distance for pathfinding to prevent performance issues with very long paths
const float MAX_PATHFINDING_DISTANCE = 100.0f; // Maximum straight-line distance to attempt pathfinding

// Minimum distance that path points must maintain from non-walkable areas
const float MIN_DISTANCE_FROM_NON_WALKABLE_AREAS = 0.1f;

// Using Node structure defined in pathfinding.h

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

// Check if a position is valid (within bounds and not colliding)
bool isPositionValid(float x, float y, float entityCollisionRadius, const Map& gameMap, const std::set<TextureName>& nonTraversableBlocks) {
    // 1. Check if the entity's entire collision body is within game bounds.
    // The entity is a circle centered at (x,y) with radius entityCollisionRadius.
    if (x - entityCollisionRadius < 0.0f || x + entityCollisionRadius >= GRID_SIZE ||
        y - entityCollisionRadius < 0.0f || y + entityCollisionRadius >= GRID_SIZE) {
        // std::cout << \"isPositionValid: FALSE (Out of bounds) for (\" << x << \", \" << y << \") radius \" << entityCollisionRadius << std::endl;
        return false; // Entity would be partially or fully out of bounds.
    }

    // 2. Check for collision with other collidable elements in the game.
    // This function should check if an entity centered at (x,y) with entityCollisionRadius collides with any *other* dynamic elements.
    if (wouldCollideWithElement(x, y, entityCollisionRadius)) {
        // std::cout << \"isPositionValid: FALSE (Element collision) for (\" << x << \", \" << y << \") radius \" << entityCollisionRadius << std::endl;
        return false;
    }

    // 3. Check collision with non-traversable map blocks.
    //    The entity (circle) must not overlap any non-traversable map blocks.
    //    We check the center and points on its circumference.
    const int numCircumferencePoints = 8; // Number of points to sample on the circumference. More points = more precision but more cost.
    const float pi = 3.14159265358979323846f;

    // Check the center point of the entity.
    if (wouldCollideWithMapBlock(x, y, gameMap, nonTraversableBlocks)) {
        // std::cout << \"isPositionValid: FALSE (Map block collision at center) for (\" << x << \", \" << y << \")\" << std::endl;
        return false; // Center of the entity is on a non-traversable block.
    }

    // Check points on the circumference of the entity's collision body.
    for (int i = 0; i < numCircumferencePoints; ++i) {
        float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(numCircumferencePoints);
        float checkX = x + entityCollisionRadius * std::cos(angle);
        float checkY = y + entityCollisionRadius * std::sin(angle);

        if (wouldCollideWithMapBlock(checkX, checkY, gameMap, nonTraversableBlocks)) {
            // std::cout << \"isPositionValid: FALSE (Map block collision at circumference point \" << i << \": \" << checkX << \", \" << checkY << \") for center (\" << x << \", \" << y << \")\" << std::endl;
            return false; // A point on the entity's circumference is on a non-traversable block.
        }
    }

    // If all checks pass, the position is valid for the entity.
    // std::cout << \"isPositionValid: TRUE for (\" << x << \", \" << y << \") radius \" << entityCollisionRadius << std::endl;
    return true;
}

// The integer version of position validation
bool isPositionValid(int x, int y, float collisionRadius, const Map& gameMap) {
    return isPositionValid(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, 
                          collisionRadius, gameMap, std::set<TextureName>());
}

// Get neighboring positions (implement the header-defined function)
std::vector<std::pair<int, int>> getNeighbors(int x, int y, float collisionRadius, const Map& gameMap) {
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
        if (isPositionValid(newX, newY, collisionRadius, gameMap)) {
            neighbors.push_back({newX, newY});
        }
    }

    return neighbors;
}

// Get neighbors with non-traversable blocks
std::vector<std::pair<int, int>> getNeighbors(int x, int y, float collisionRadius, 
                                             const Map& gameMap, 
                                             const std::set<TextureName>& nonTraversableBlocks) {
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
        // Use the float version of isPositionValid that handles non-traversable blocks,
        // checking the CENTER of the potential neighbor cell.
        if (isPositionValid(static_cast<float>(newX) + 0.5f, static_cast<float>(newY) + 0.5f, 
                          collisionRadius, gameMap, nonTraversableBlocks)) {
            neighbors.push_back({newX, newY});
        }
    }

    return neighbors;
}

// Find path using A* algorithm
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    float collisionRadius,
    const std::set<TextureName>& nonTraversableBlocks
) {
    // Convert float coordinates to integer grid coordinates for A*
    int startNodeX = static_cast<int>(startX);
    int startNodeY = static_cast<int>(startY);
    int goalNodeX = static_cast<int>(goalX);
    int goalNodeY = static_cast<int>(goalY);

    // Check if start or goal are invalid (e.g., inside an obstacle)
    if (!isPositionValid(startX, startY, collisionRadius, gameMap, nonTraversableBlocks)) {
        std::cerr << "Pathfinding Error: Start position (" << startX << ", " << startY << ") is invalid." << std::endl;
        return {}; // Return empty path
    }
    if (!isPositionValid(goalX, goalY, collisionRadius, gameMap, nonTraversableBlocks)) {
        std::cerr << "Pathfinding Error: Goal position (" << goalX << ", " << goalY << ") is invalid." << std::endl;
        // Attempt to find a nearby valid goal position (optional, simple attempt here)
        bool foundValidGoal = false;
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;
                float newGoalX = goalX + dx;
                float newGoalY = goalY + dy;
                if (isPositionValid(newGoalX, newGoalY, collisionRadius, gameMap, nonTraversableBlocks)) {
                    goalNodeX = static_cast<int>(newGoalX);
                    goalNodeY = static_cast<int>(newGoalY);
                    std::cout << "Pathfinding: Original goal invalid, adjusted to (" << newGoalX << ", " << newGoalY << ")" << std::endl;
                    foundValidGoal = true;
                    break;
                }
            }
            if (foundValidGoal) break;
        }
        if (!foundValidGoal) {
            std::cerr << "Pathfinding Error: Could not find a valid goal position near (" << goalX << ", " << goalY << ")." << std::endl;
            return {};
        }
    }


    std::priority_queue<Node*, std::vector<Node*>, CompareNodes> openSet;
    std::unordered_map<std::pair<int, int>, Node*, PairHash> allNodes; // Stores all created nodes to manage memory and access

    Node* startNode = new Node(startNodeX, startNodeY);
    startNode->gCost = 0;
    startNode->hCost = calculateHeuristic(startNodeX, startNodeY, goalNodeX, goalNodeY);
    startNode->fCost = startNode->gCost + startNode->hCost;
    openSet.push(startNode);
    allNodes[{startNodeX, startNodeY}] = startNode;

    std::unordered_set<std::pair<int, int>, PairHash> closedSet;

    std::vector<std::pair<float, float>> path;

    int iterations = 0;
    const int maxIterations = GRID_SIZE * GRID_SIZE * 2; // A reasonable limit to prevent infinite loops in edge cases

    while (!openSet.empty()) {
        iterations++;
        if (iterations > maxIterations) {
            std::cerr << "Pathfinding: Exceeded maximum iterations (" << maxIterations << "). Aborting search." << std::endl;
            std::cerr << "Pathfinding Details: Start (" << startX << ", " << startY << ") Goal (" << goalX << ", " << goalY << ") Radius (" << collisionRadius << ")" << std::endl;
            std::cerr << "Pathfinding Details: Open set size: " << openSet.size() << ", Closed set size: " << closedSet.size() << std::endl;
            if (!openSet.empty()) {
                Node* currentTop = openSet.top();
                std::cerr << "Pathfinding Details: Current top of openSet: (" << currentTop->x << ", " << currentTop->y << ") fCost: " << currentTop->fCost << std::endl;
            }
            for (auto& pair : allNodes) { delete pair.second; }
            return {};
        }

        Node* currentNode = openSet.top();
        openSet.pop();

        if (currentNode->x == goalNodeX && currentNode->y == goalNodeY) {
            // Path found, reconstruct
            Node* temp = currentNode;
            while (temp != nullptr) {
                // Path waypoints are cell centers
                path.push_back({static_cast<float>(temp->x) + 0.5f, static_cast<float>(temp->y) + 0.5f});
                temp = temp->parent;
            }
            std::reverse(path.begin(), path.end());
            
            // Ensure the final goal position (float) is accurately set if path is found
            if (!path.empty()) {
                // If the path ends at a cell center different from the precise float goal,
                // and the float goal is valid, adjust the last point.
                // The first point will also be adjusted to the precise startX, startY.
                path.back() = {goalX, goalY};
            } else if (startX == goalX && startY == goalY) {
                // Handle case where start and goal are the same
                path.push_back({startX, startY});
            }

            // Clean up allocated nodes
            for (auto& pair : allNodes) {
                delete pair.second;
            }
            simplifyPath(path);
            if (path.size() == 1 && (path[0].first != goalX || path[0].second != goalY) && (startX != goalX || startY != goalY) ) {
                 // If simplified path is just the start, but goal is different, add goal if valid
                 if(isPositionValid(goalX, goalY, collisionRadius, gameMap, nonTraversableBlocks)) {
                    path.push_back({goalX, goalY});
                 }
            } else if (path.empty() && startX == goalX && startY == goalY) {
                 path.push_back({startX, startY});
            } else if (path.size() > 1) {
                // Ensure the very first point is the exact start float coordinates
                path[0] = {startX, startY};
                // Ensure the very last point is the exact goal float coordinates
                path.back() = {goalX, goalY};
            }


            return path;
        }

        closedSet.insert({currentNode->x, currentNode->y});

        for (const auto& neighborPos : getNeighbors(currentNode->x, currentNode->y, collisionRadius, gameMap, nonTraversableBlocks)) {
            if (closedSet.count(neighborPos)) {
                continue;
            }

            float tentativeGCost = currentNode->gCost + 1.0f; // Assuming cost of 1 for adjacent nodes

            Node* neighborNode = nullptr;
            if (allNodes.count(neighborPos)) {
                neighborNode = allNodes[neighborPos];
            } else {
                neighborNode = new Node(neighborPos.first, neighborPos.second);
                allNodes[neighborPos] = neighborNode;
            }

            bool inOpenSet = false; // This check is tricky with raw pointers in priority_queue.
                                    // A better way is to check if gCost can be improved.

            if (tentativeGCost < neighborNode->gCost || neighborNode->gCost == 0 && neighborNode->parent == nullptr && !(neighborNode->x == startNodeX && neighborNode->y == startNodeY) ) { // if gCost is 0 and no parent, it's effectively infinity unless it's the start node
                neighborNode->parent = currentNode;
                neighborNode->gCost = tentativeGCost;
                neighborNode->hCost = calculateHeuristic(neighborNode->x, neighborNode->y, goalNodeX, goalNodeY);
                neighborNode->fCost = neighborNode->gCost + neighborNode->hCost;
                
                // To update priority in openSet, typically involves removing and re-adding,
                // or using a structure that supports decrease-key.
                // A simpler (though less efficient for dense graphs) approach for std::priority_queue
                // is to just add it again; the earlier, worse entry will be processed later or ignored.
                // However, ensure we don't have duplicates if we are checking if it's already there.
                // For now, we'll rely on the gCost check. If it was already in openSet with higher gCost,
                // this new one will be prioritized. If it wasn't in openSet, it's added.
                openSet.push(neighborNode);
            }
        }
    }

    // Clean up allocated nodes if no path found
    for (auto& pair : allNodes) {
        delete pair.second;
    }
    // Enhanced Debugging Output
    std::cerr << "-------------------------------------------------" << std::endl;
    std::cerr << "Pathfinding: No path found! Details:" << std::endl;
    std::cerr << "  Start Node (float): (" << startX << ", " << startY << ")" << std::endl;
    std::cerr << "  Goal Node (float):  (" << goalX << ", " << goalY << ")" << std::endl;
    std::cerr << "  Start Node (int):   (" << startNodeX << ", " << startNodeY << ")" << std::endl;
    std::cerr << "  Goal Node (int):    (" << goalNodeX << ", " << goalNodeY << ")" << std::endl;
    std::cerr << "  Collision Radius:   " << collisionRadius << std::endl;
    std::cerr << "  Open Set Status:    " << (openSet.empty() ? "Empty" : "Not Empty (Error in logic?)") << std::endl;
    std::cerr << "  Nodes Explored (Closed Set Size): " << closedSet.size() << std::endl;
    std::cerr << "  Total Nodes Considered (AllNodes map size): " << allNodes.size() << std::endl;
    std::cerr << "  Number of Iterations: " << iterations << std::endl;

    // Optionally, print some of the closed set nodes if it's small enough or a sample
    if (closedSet.size() < 20 && closedSet.size() > 0) {
        std::cerr << "  Sample of Closed Set Nodes (x, y):" << std::endl;
        int count = 0;
        for (const auto& node_pair : closedSet) {
            std::cerr << "    (" << node_pair.first << ", " << node_pair.second << ")" << std::endl;
            if (++count >= 5) break; // Print first 5 explored nodes
        }
    } else if (closedSet.size() >= 20) {
        std::cerr << "  Closed set is large, not printing samples here." << std::endl;
    }

    std::cerr << "  Non-Traversable Blocks for this entity:";
    if (nonTraversableBlocks.empty()) {
        std::cerr << " (None specified)" << std::endl;
    } else {
        std::cerr << std::endl;
        for (const auto& block : nonTraversableBlocks) {
            std::cerr << "    - " << static_cast<int>(block) << std::endl; // Assuming TextureName can be cast to int for logging
        }
    }
    std::cerr << "-------------------------------------------------" << std::endl;

    return {}; // No path found
}

// Overloaded function for backward compatibility
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    float collisionRadius
) {
    return findPath(startX, startY, goalX, goalY, gameMap, collisionRadius, std::set<TextureName>());
}

// Simplify path by removing waypoints that are too close together
void simplifyPath(std::vector<std::pair<float, float>>& path) {
    if (path.size() <= 2) return;

    std::vector<std::pair<float, float>> simplified;
    simplified.push_back(path.front());

    for (size_t i = 1; i < path.size(); ++i) {
        float dx = path[i].first - simplified.back().first;
        float dy = path[i].second - simplified.back().second;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Only add points that are far enough from the last added point
        if (distance >= MINIMUM_DISTANCE_BETWEEN_WAYPOINTS || i == path.size() - 1) {
            simplified.push_back(path[i]);
        }
    }

    path = std::move(simplified);
}
