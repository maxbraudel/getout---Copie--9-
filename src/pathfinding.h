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

// Forward declaration for EntityConfiguration
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

// Find a path from start to goal using A* algorithm with proper entity collision shape detection
// Returns a vector of positions (x, y) forming the path
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    const EntityConfiguration& entityConfig
);

// Check if a position is valid for pathfinding using entity collision shape
bool isPositionValid(int x, int y, const EntityConfiguration& entityConfig, const Map& gameMap);
bool isPositionValid(float x, float y, const EntityConfiguration& entityConfig, const Map& gameMap);

// Calculate the heuristic value (estimated cost to goal)
// Using Manhattan distance for grid-based pathfinding
float calculateHeuristic(int x1, int y1, int x2, int y2);

// Get all valid neighbors for a position with collision checking using entity shape
std::vector<std::pair<int, int>> getNeighbors(int x, int y, const EntityConfiguration& entityConfig, const Map& gameMap);

// Check if a segment between two points is valid (no collisions along the path)
bool isSegmentValid(float x1, float y1, float x2, float y2, const EntityConfiguration& entityConfig, const Map& gameMap);

#endif // PATHFINDING_H
