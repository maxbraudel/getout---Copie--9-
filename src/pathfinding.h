#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "map.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <set>
#include <cmath>

// Forward declaration
struct EntityConfiguration;

// Minimum allowed distance between waypoints to reduce path zigzagging
const float MINIMUM_DISTANCE_BETWEEN_WAYPOINTS = 3.0f;

// A structure to represent a node in the pathfinding grid
struct Node {
    int x;
    int y;
    float gCost;  // Cost from start to this node
    float hCost;  // Heuristic cost from this node to the goal
    float fCost;  // Total cost (g + h)
    Node* parent;
    
    // Constructor with initialization
    Node(int _x, int _y) : x(_x), y(_y), gCost(0), hCost(0), fCost(0), parent(nullptr) {}
    
    // Comparator for priority queue (lowest fCost has highest priority)
    bool operator>(const Node& other) const {
        return fCost > other.fCost;
    }
};

// Custom hash function for std::pair<int, int>
struct PairHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
    }
};

// Find a path from start to goal using A* algorithm
// Returns a vector of positions (x, y) forming the path
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    float collisionRadius
);

// Overloaded function that uses entity-specific non-traversable blocks
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    float collisionRadius,
    const std::set<TextureName>& nonTraversableBlocks
);

// New functions that use EntityConfiguration for collision shape points
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    const EntityConfiguration& entityConfig
);

// Check if a position is valid for pathfinding
bool isPositionValid(int x, int y, float collisionRadius, const Map& gameMap);

// Overloaded function that uses entity-specific non-traversable blocks
bool isPositionValid(int x, int y, float collisionRadius, const Map& gameMap, const std::set<TextureName>& nonTraversableBlocks);

// New function signatures using EntityConfiguration
bool isPositionValid(float x, float y, const EntityConfiguration& entityConfig, const Map& gameMap);

// Calculate the heuristic value (estimated cost to goal)
// Using Euclidean distance for natural movement
float calculateHeuristic(int x1, int y1, int x2, int y2);

// Get all valid neighbors for a position with collision checking
std::vector<std::pair<int, int>> getNeighbors(int x, int y, float collisionRadius, const Map& gameMap);

// Overloaded function that uses entity-specific non-traversable blocks
std::vector<std::pair<int, int>> getNeighbors(int x, int y, float collisionRadius, const Map& gameMap, const std::set<TextureName>& nonTraversableBlocks);

// New function signatures using EntityConfiguration
std::vector<std::pair<int, int>> getNeighbors(int x, int y, const EntityConfiguration& entityConfig, const Map& gameMap);

// Simplify a path by removing unnecessary waypoints
// This reduces zigzagging and makes movement smoother
void simplifyPath(std::vector<std::pair<float, float>>& path);

// Check if a path is clear of element collisions
bool isPathClear(const std::vector<std::pair<float, float>>& path, float collisionRadius);

#endif // PATHFINDING_H
