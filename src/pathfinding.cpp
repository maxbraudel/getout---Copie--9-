#include "pathfinding.h"
#include "collision.h"
#include "globals.h" // For GRID_SIZE
#include <iostream>
#include <algorithm> // For std::reverse

// Function declarations for helper functions
std::vector<std::pair<float, float>> simplifyPath(const std::vector<std::pair<float, float>>& path);

// Find a path from start to goal using A* algorithm
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY, 
    const Map& gameMap,
    float collisionRadius
) {
    // Check if the start and goal positions are the same
    if (std::abs(startX - goalX) < 0.1f && std::abs(startY - goalY) < 0.1f) {
        // Already at the goal
        return { {goalX, goalY} };
    }
    
    // Convert float coordinates to grid positions
    int startGridX = static_cast<int>(startX);
    int startGridY = static_cast<int>(startY);
    int goalGridX = static_cast<int>(goalX);
    int goalGridY = static_cast<int>(goalY);
    
    // Validate both start and goal are within grid bounds
    if (startGridX < 0 || startGridX >= GRID_SIZE || startGridY < 0 || startGridY >= GRID_SIZE ||
        goalGridX < 0 || goalGridX >= GRID_SIZE || goalGridY < 0 || goalGridY >= GRID_SIZE) {
        std::cout << "Invalid path coordinates. Start (" << startX << "," << startY 
                 << ") or goal (" << goalX << "," << goalY << ") are out of bounds." << std::endl;
        return {};
    }
    
    // Initialize the open and closed sets
    std::priority_queue<Node*, std::vector<Node*>, std::greater<Node*>> openSet;
    std::unordered_set<std::pair<int, int>, PairHash> closedSet;
    std::unordered_map<std::pair<int, int>, Node*, PairHash> allNodes;
    
    // Create the start node and add it to the open set
    Node* startNode = new Node(startGridX, startGridY);
    startNode->hCost = calculateHeuristic(startGridX, startGridY, goalGridX, goalGridY);
    startNode->fCost = startNode->hCost;
    
    openSet.push(startNode);
    allNodes[{startGridX, startGridY}] = startNode;
    
    // Variables to help detect if no path exists
    int nodesProcessed = 0;
    const int maxNodes = 1000; // Prevent infinite looping by setting a reasonable limit
    
    // While the open set is not empty
    while (!openSet.empty() && nodesProcessed < maxNodes) {
        // Get the node with the lowest fCost
        Node* current = openSet.top();
        openSet.pop();
        
        // Add current to the closed set
        closedSet.insert({current->x, current->y});
        
        // Check if we reached the goal
        if (current->x == goalGridX && current->y == goalGridY) {
            // Construct the path by backtracking from the goal
            std::vector<std::pair<float, float>> path;
            Node* pathNode = current;
            
            while (pathNode != nullptr) {            // Convert grid coordinates back to world coordinates (center of the cell)
                path.push_back({pathNode->x + 0.0f, pathNode->y + 0.0f});
                pathNode = pathNode->parent;
            }
            
            // Reverse the path so it goes from start to goal
            std::reverse(path.begin(), path.end());
            
            // Simplify the path to reduce zigzagging and make movement smoother
            simplifyPath(path);
            
            // Clean up all allocated nodes
            for (auto& pair : allNodes) {
                delete pair.second;
            }
            
            return path;
        }
        
        // Get neighbors
        std::vector<std::pair<int, int>> neighbors = getNeighbors(current->x, current->y, collisionRadius, gameMap);
        
        // For each neighbor
        for (const auto& neighbor : neighbors) {
            // Skip if this neighbor is already in the closed set
            if (closedSet.find(neighbor) != closedSet.end()) {
                continue;
            }
            
            // Calculate tentative gCost for this neighbor
            // Cost is 1.0 for orthogonal moves, sqrt(2) ≈ 1.414 for diagonal moves
            float moveCost;
            if (neighbor.first == current->x || neighbor.second == current->y) {
                moveCost = 1.0f; // Orthogonal move
            } else {
                moveCost = 1.414f; // Diagonal move
            }
            
            float tentativeGCost = current->gCost + moveCost;
            
            // Check if this node is already in our open set
            auto nodeIt = allNodes.find(neighbor);
            Node* neighborNode;
            
            if (nodeIt == allNodes.end()) {
                // This is a new node, create it
                neighborNode = new Node(neighbor.first, neighbor.second);
                allNodes[neighbor] = neighborNode;
            } else {
                neighborNode = nodeIt->second;
                // If this path to the neighbor is not better, skip
                if (tentativeGCost >= neighborNode->gCost) {
                    continue;
                }
            }
            
            // This is the best path so far, record it
            neighborNode->parent = current;
            neighborNode->gCost = tentativeGCost;
            neighborNode->hCost = calculateHeuristic(neighbor.first, neighbor.second, goalGridX, goalGridY);
            neighborNode->fCost = neighborNode->gCost + neighborNode->hCost;
            
            // Add to the open set if not already there
            openSet.push(neighborNode);
        }
        
        nodesProcessed++;
    }
    
    // If we get here, no path was found
    std::cout << "No path found from (" << startX << ", " << startY
              << ") to (" << goalX << ", " << goalY << ")" << std::endl;
    
    // Clean up all allocated nodes
    for (auto& pair : allNodes) {
        delete pair.second;
    }
    
    // Return an empty path
    return {};
}

// Overloaded function that uses entity-specific non-traversable blocks
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY, 
    const Map& gameMap,
    float collisionRadius,
    const std::set<TextureName>& nonTraversableBlocks
) {
    // Check if the start and goal positions are the same
    if (std::abs(startX - goalX) < 0.1f && std::abs(startY - goalY) < 0.1f) {
        // Already at the goal
        return { {goalX, goalY} };
    }
    
    // Convert float coordinates to grid positions
    int startGridX = static_cast<int>(startX);
    int startGridY = static_cast<int>(startY);
    int goalGridX = static_cast<int>(goalX);
    int goalGridY = static_cast<int>(goalY);
    
    // Validate both start and goal are within grid bounds
    if (startGridX < 0 || startGridX >= GRID_SIZE || startGridY < 0 || startGridY >= GRID_SIZE ||
        goalGridX < 0 || goalGridX >= GRID_SIZE || goalGridY < 0 || goalGridY >= GRID_SIZE) {
        std::cout << "Invalid path coordinates. Start (" << startX << "," << startY 
                 << ") or goal (" << goalX << "," << goalY << ") are out of bounds." << std::endl;
        return {};
    }
    
    // Initialize the open and closed sets
    std::priority_queue<Node*, std::vector<Node*>, std::greater<Node*>> openSet;
    std::unordered_set<std::pair<int, int>, PairHash> closedSet;
    std::unordered_map<std::pair<int, int>, Node*, PairHash> allNodes;
    
    // Create the start node and add it to the open set
    Node* startNode = new Node(startGridX, startGridY);
    startNode->hCost = calculateHeuristic(startGridX, startGridY, goalGridX, goalGridY);
    startNode->fCost = startNode->hCost;
    
    openSet.push(startNode);
    allNodes[{startGridX, startGridY}] = startNode;
    
    // Variables to help detect if no path exists
    int nodesProcessed = 0;
    const int maxNodes = 1000; // Prevent infinite looping by setting a reasonable limit
    
    // While the open set is not empty
    while (!openSet.empty() && nodesProcessed < maxNodes) {
        // Get the node with the lowest fCost
        Node* current = openSet.top();
        openSet.pop();
        
        // Add current to the closed set
        closedSet.insert({current->x, current->y});
        
        // Check if we reached the goal
        if (current->x == goalGridX && current->y == goalGridY) {
            // Construct the path by backtracking from the goal
            std::vector<std::pair<float, float>> path;
            Node* pathNode = current;
            
            while (pathNode != nullptr) {
                // Convert grid coordinates back to world coordinates (center of the cell)
                path.push_back({pathNode->x + 0.0f, pathNode->y + 0.0f});
                pathNode = pathNode->parent;
            }
            
            // Reverse the path so it goes from start to goal
            std::reverse(path.begin(), path.end());
            
            // Simplify the path to reduce zigzagging and make movement smoother
            simplifyPath(path);
            
            // Clean up all allocated nodes
            for (auto& pair : allNodes) {
                delete pair.second;
            }
            
            return path;
        }
        
        // Get neighbors using entity-specific non-traversable blocks
        std::vector<std::pair<int, int>> neighbors = getNeighbors(current->x, current->y, collisionRadius, gameMap, nonTraversableBlocks);
        
        // For each neighbor
        for (const auto& neighbor : neighbors) {
            // Skip if this neighbor is already in the closed set
            if (closedSet.find(neighbor) != closedSet.end()) {
                continue;
            }
            
            // Calculate tentative gCost for this neighbor
            // Cost is 1.0 for orthogonal moves, sqrt(2) ≈ 1.414 for diagonal moves
            float moveCost;
            if (neighbor.first == current->x || neighbor.second == current->y) {
                moveCost = 1.0f; // Orthogonal move
            } else {
                moveCost = 1.414f; // Diagonal move
            }
            
            float tentativeGCost = current->gCost + moveCost;
            
            // Check if this node is already in our open set
            auto nodeIt = allNodes.find(neighbor);
            Node* neighborNode;
            
            if (nodeIt == allNodes.end()) {
                // This is a new node, create it
                neighborNode = new Node(neighbor.first, neighbor.second);
                allNodes[neighbor] = neighborNode;
            } else {
                neighborNode = nodeIt->second;
                // If this path to the neighbor is not better, skip
                if (tentativeGCost >= neighborNode->gCost) {
                    continue;
                }
            }
            
            // This is the best path so far, record it
            neighborNode->parent = current;
            neighborNode->gCost = tentativeGCost;
            neighborNode->hCost = calculateHeuristic(neighbor.first, neighbor.second, goalGridX, goalGridY);
            neighborNode->fCost = neighborNode->gCost + neighborNode->hCost;
            
            // Add to the open set if not already there
            openSet.push(neighborNode);
        }
        
        nodesProcessed++;
    }
    
    // If we get here, no path was found
    std::cout << "No path found from (" << startX << ", " << startY
              << ") to (" << goalX << ", " << goalY << ")" << std::endl;
    
    // Clean up all allocated nodes
    for (auto& pair : allNodes) {
        delete pair.second;
    }
    
    // Return an empty path
    return {};
}

// Calculate the heuristic value (estimated cost to goal)
// Using Euclidean distance for natural movement
float calculateHeuristic(int x1, int y1, int x2, int y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

// Check if a position is valid for pathfinding
bool isPositionValid(int x, int y, float collisionRadius, const Map& gameMap) {
    // First check if coordinates are within the grid bounds
    if (x < 0 || y < 0 || x >= GRID_SIZE || y >= GRID_SIZE) {
        // Uncomment this for debugging specific issues, but keep commented to avoid console spam
        // std::cout << "Warning: Coordinates (" << x << ", " << y << ") are outside the grid bounds" << std::endl;
        return false;
    }
    
    // Convert grid coordinates to world coordinates (center of the cell)
    float worldX = x + 0.0f;
    float worldY = y + 0.0f;
    
    // Check if this position would collide with any collision
    bool collisionWithElement = wouldCollideWithElement(worldX, worldY, collisionRadius);
    bool collisionWithMapBlock = wouldCollideWithMapBlock(worldX, worldY, gameMap);
    
    return !collisionWithElement && !collisionWithMapBlock;
}

// Overloaded function that uses entity-specific non-traversable blocks
bool isPositionValid(int x, int y, float collisionRadius, const Map& gameMap, const std::set<TextureName>& nonTraversableBlocks) {
    // First check if coordinates are within the grid bounds
    if (x < 0 || y < 0 || x >= GRID_SIZE || y >= GRID_SIZE) {
        return false;
    }
    
    // Convert grid coordinates to world coordinates (center of the cell)
    float worldX = x + 0.0f;
    float worldY = y + 0.0f;
    
    // Check if this position would collide with any collision
    bool collisionWithElement = wouldCollideWithElement(worldX, worldY, collisionRadius);
    bool collisionWithMapBlock = wouldCollideWithMapBlock(worldX, worldY, gameMap, nonTraversableBlocks);
    
    return !collisionWithElement && !collisionWithMapBlock;
}

// Get all valid neighbors for a position with diagonal movement
std::vector<std::pair<int, int>> getNeighbors(int x, int y, float collisionRadius, const Map& gameMap) {
    std::vector<std::pair<int, int>> neighbors;
    
    // Check all 8 directions: orthogonal and diagonal
    const std::vector<std::pair<int, int>> directions = {
        {0, 1},   // North
        {1, 1},   // Northeast
        {1, 0},   // East
        {1, -1},  // Southeast
        {0, -1},  // South
        {-1, -1}, // Southwest
        {-1, 0},  // West
        {-1, 1}   // Northwest
    };
    
    for (const auto& dir : directions) {
        int nx = x + dir.first;
        int ny = y + dir.second;
        
        // If this is a diagonal move, check that the two adjacent
        // orthogonal cells are also valid (to prevent cutting corners)
        if (dir.first != 0 && dir.second != 0) {
            // This is a diagonal move, check adjacent orthogonal cells
            bool orthogonal1Valid = isPositionValid(x + dir.first, y, collisionRadius, gameMap);
            bool orthogonal2Valid = isPositionValid(x, y + dir.second, collisionRadius, gameMap);
            
            // Only allow the diagonal if both orthogonal directions are clear
            if (!orthogonal1Valid || !orthogonal2Valid) {
                continue;
            }
        }
        
        // Check if the neighbor position is valid
        if (isPositionValid(nx, ny, collisionRadius, gameMap)) {
            neighbors.push_back({nx, ny});
        }
    }
    
    return neighbors;
}

// Overloaded function that uses entity-specific non-traversable blocks
std::vector<std::pair<int, int>> getNeighbors(int x, int y, float collisionRadius, const Map& gameMap, const std::set<TextureName>& nonTraversableBlocks) {
    std::vector<std::pair<int, int>> neighbors;
    
    // Check all 8 directions: orthogonal and diagonal
    const std::vector<std::pair<int, int>> directions = {
        {0, 1},   // North
        {1, 1},   // Northeast
        {1, 0},   // East
        {1, -1},  // Southeast
        {0, -1},  // South
        {-1, -1}, // Southwest
        {-1, 0},  // West
        {-1, 1}   // Northwest
    };
    
    for (const auto& dir : directions) {
        int nx = x + dir.first;
        int ny = y + dir.second;
        
        // If this is a diagonal move, check that the two adjacent
        // orthogonal cells are also valid (to prevent cutting corners)
        if (dir.first != 0 && dir.second != 0) {
            // This is a diagonal move, check adjacent orthogonal cells
            bool orthogonal1Valid = isPositionValid(x + dir.first, y, collisionRadius, gameMap, nonTraversableBlocks);
            bool orthogonal2Valid = isPositionValid(x, y + dir.second, collisionRadius, gameMap, nonTraversableBlocks);
            
            // Only allow the diagonal if both orthogonal directions are clear
            if (!orthogonal1Valid || !orthogonal2Valid) {
                continue;
            }
        }
        
        // Check if the neighbor position is valid
        if (isPositionValid(nx, ny, collisionRadius, gameMap, nonTraversableBlocks)) {
            neighbors.push_back({nx, ny});
        }
    }
    
    return neighbors;
}

// Simplify a path by removing unnecessary waypoints
void simplifyPath(std::vector<std::pair<float, float>>& path) {
    if (path.size() <= 2) {
        // Path is already as simple as it can be
        return;
    }
    
    std::vector<std::pair<float, float>> simplifiedPath;
    simplifiedPath.push_back(path[0]); // Always keep the start point
    
    float cumulativeDistance = 0.0f;
    std::pair<float, float> prevDirection = {0.0f, 0.0f};
    
    // Loop through the path, skipping waypoints based on MINIMUM_DISTANCE_BETWEEN_WAYPOINTS
    for (size_t i = 1; i < path.size(); ++i) {
        // Calculate distance and direction to next waypoint
        float dx = path[i].first - path[i-1].first;
        float dy = path[i].second - path[i-1].second;
        float segmentDistance = std::sqrt(dx * dx + dy * dy);
        
        // Normalize the direction vector
        float dirX = (segmentDistance > 0.0001f) ? dx / segmentDistance : 0.0f;
        float dirY = (segmentDistance > 0.0001f) ? dy / segmentDistance : 0.0f;
        std::pair<float, float> currentDirection = {dirX, dirY};
        
        // Add the distance to our cumulative distance
        cumulativeDistance += segmentDistance;
        
        // Always include the last waypoint in the path
        bool isLastWaypoint = (i == path.size() - 1);
        
        // Check if we should include this waypoint based on:
        // 1. Accumulated distance exceeds minimum
        // 2. Significant direction change (avoid zigzag)
        // 3. It's the last waypoint (always include)
        bool significantDirectionChange = false;
        if (prevDirection.first != 0.0f || prevDirection.second != 0.0f) {
            // Calculate dot product to measure direction similarity
            float dotProduct = prevDirection.first * currentDirection.first + 
                              prevDirection.second * currentDirection.second;
            // If dot product is less than 0.9 (angles differ by more than ~25 degrees)
            significantDirectionChange = (dotProduct < 0.9f);
        }
        
        if (isLastWaypoint || cumulativeDistance >= MINIMUM_DISTANCE_BETWEEN_WAYPOINTS || significantDirectionChange) {
            simplifiedPath.push_back(path[i]);
            cumulativeDistance = 0.0f; // Reset accumulator
            prevDirection = currentDirection; // Remember this direction
        }
    }
    
    // Replace the original path with the simplified one
    path = simplifiedPath;
}
