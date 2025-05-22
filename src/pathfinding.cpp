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
bool isPositionValid(float x, float y, float collisionRadius, const Map& gameMap, const std::set<TextureName>& nonTraversableBlocks) {
    // Check bounds
    if (x < 0 || y < 0 || x >= GRID_SIZE || y >= GRID_SIZE) {
        return false;
    }

    // Check for collisions with map blocks
    if (wouldCollideWithMapBlock(x, y, gameMap, nonTraversableBlocks)) {
        return false;
    }

    // Check for collisions with elements (like coconut trees)
    if (wouldCollideWithElement(x, y, collisionRadius)) {
        return false;
    }

    return true;
}

// The integer version of position validation
bool isPositionValid(int x, int y, float collisionRadius, const Map& gameMap) {
    return isPositionValid(static_cast<float>(x), static_cast<float>(y), 
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
        // Use the float version of isPositionValid that handles non-traversable blocks
        if (isPositionValid(static_cast<float>(newX), static_cast<float>(newY), 
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
    // Check if the target is too far away
    float straightLineDistance = std::sqrt(
        std::pow(goalX - startX, 2) + std::pow(goalY - startY, 2)
    );
    
    if (straightLineDistance > MAX_PATHFINDING_DISTANCE) {
        std::cout << "Path destination (" << goalX << ", " << goalY 
                  << ") is too far from start (" << startX << ", " << startY 
                  << "). Distance: " << straightLineDistance 
                  << " exceeds limit of " << MAX_PATHFINDING_DISTANCE << std::endl;
        return std::vector<std::pair<float, float>>();
    }

    // Initialize open and closed sets
    std::priority_queue<Node*, std::vector<Node*>, CompareNodes> openSet;
    std::set<std::pair<int, int>> closedSet; // Using grid coordinates for closed set
    std::vector<Node*> allNodes; // For cleanup    // Create start node with grid coordinates
    Node* startNode = new Node(static_cast<int>(startX), static_cast<int>(startY));
    startNode->hCost = calculateHeuristic(static_cast<int>(startX), static_cast<int>(startY), 
                                        static_cast<int>(goalX), static_cast<int>(goalY));
    startNode->fCost = startNode->hCost;
    openSet.push(startNode);
    allNodes.push_back(startNode);

    // A* search
    while (!openSet.empty()) {
        Node* current = openSet.top();
        openSet.pop();        // Get grid coordinates from node
        int gridX = current->x;
        int gridY = current->y;
        
        // Skip if already processed
        if (closedSet.find({gridX, gridY}) != closedSet.end()) {
            continue;
        }
        
        // Add to closed set
        closedSet.insert({gridX, gridY});

        // Check if we reached the goal
        float distToGoal = std::sqrt(std::pow(current->x - goalX, 2) + std::pow(current->y - goalY, 2));
        if (distToGoal < 0.5f) { // Within acceptable range of goal
            // Reconstruct path
            std::vector<std::pair<float, float>> path;
            Node* pathNode = current;
            while (pathNode != nullptr) {
                path.push_back({pathNode->x, pathNode->y});
                pathNode = pathNode->parent;
            }
            std::reverse(path.begin(), path.end());

            // Cleanup
            for (Node* node : allNodes) {
                delete node;
            }            // Simplify path to remove too-close waypoints
            if (path.size() > 2) {
                simplifyPath(path);
            }

            return path;
        }        // Get neighbors using the entity-specific version
        auto neighbors = getNeighbors(current->x, current->y, collisionRadius, gameMap, nonTraversableBlocks);
        for (const auto& neighbor : neighbors) {
            // We already know the position is valid since getNeighbors checks it// Calculate new costs
            float newGCost = current->gCost + std::sqrt(
                std::pow(neighbor.first - current->x, 2) + 
                std::pow(neighbor.second - current->y, 2)
            );

            Node* neighborNode = new Node(static_cast<int>(neighbor.first), static_cast<int>(neighbor.second));
            neighborNode->gCost = newGCost;
            neighborNode->hCost = calculateHeuristic(static_cast<int>(neighbor.first), 
                                                   static_cast<int>(neighbor.second), 
                                                   static_cast<int>(goalX), 
                                                   static_cast<int>(goalY));
            neighborNode->fCost = neighborNode->gCost + neighborNode->hCost;
            neighborNode->parent = current;

            openSet.push(neighborNode);
            allNodes.push_back(neighborNode);
        }
    }

    // No path found, return empty path
    for (Node* node : allNodes) {
        delete node;
    }
    
    // Return direct path if no other path found (entities might still try to move)
    std::vector<std::pair<float, float>> directPath;
    directPath.push_back({startX, startY});
    directPath.push_back({goalX, goalY});
    return directPath;
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
