#include "pathfinding.h"
#include "globals.h" // For GRID_SIZE
#include "collision.h" // For collision detection
#include "entities.h" // For EntityConfiguration
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits> // Added to resolve std::numeric_limits error

// Maximum distance for pathfinding to prevent performance issues with very long paths
const float MAX_PATHFINDING_DISTANCE = 1000.0f; // Maximum straight-line distance to attempt pathfinding

// Minimum distance that path points must maintain from non-walkable blocks (map terrain)
const float MIN_DISTANCE_FROM_NON_WALKABLE_BLOCKS = 0.05f;

// Minimum distance that path points must maintain from non-walkable elements (objects with collision)
const float MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS = 0.5f;

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
    // 1. Check if the entity\'s entire collision body is within game bounds.
    // This uses the actual entityCollisionRadius.
    if (x - entityCollisionRadius < 0.0f || x + entityCollisionRadius >= GRID_SIZE ||
        y - entityCollisionRadius < 0.0f || y + entityCollisionRadius >= GRID_SIZE) {
        // std::cout << "isPositionValid: FALSE (Out of bounds) for (" << x << ", " << y << ") radius " << entityCollisionRadius << std::endl;
        return false; // Entity would be partially or fully out of bounds.
    }    // 2. Check for collision with other collidable elements in the game, maintaining MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS.
    // First check direct collision with the actual entity collision radius
    if (wouldCollideWithElement(x, y, entityCollisionRadius)) {
        // std::cout << "isPositionValid: FALSE (Element collision) for (" << x << ", " << y << ") radius " << entityCollisionRadius << std::endl;
        return false;
    }
    
    // Then check with margin for pathfinding safety distance
    float effectiveRadiusForElementCheck = entityCollisionRadius + MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS;
    if (wouldCollideWithElement(x, y, effectiveRadiusForElementCheck)) {
        // std::cout << "isPositionValid: FALSE (Element margin collision) for (" << x << ", " << y << ") radius " << effectiveRadiusForElementCheck << std::endl;
        return false;
    }

    // 3. Check collision with non-traversable map blocks, maintaining MIN_DISTANCE_FROM_NON_WALKABLE_BLOCKS.
    
    // First, check if the entity\'s center is directly inside a non-traversable block.
    if (wouldCollideWithMapBlock(x, y, gameMap, nonTraversableBlocks)) {
        // std::cout << "isPositionValid: FALSE (Map block collision at center) for (" << x << ", " << y << ")" << std::endl;
        return false; // Center of the entity is on a non-traversable block.
    }    // Define the effective radius for checking against map blocks and boundaries, incorporating the margin.
    float effectiveRadiusForMarginCheck = entityCollisionRadius + MIN_DISTANCE_FROM_NON_WALKABLE_BLOCKS;

    const int numCircumferencePoints = 8; 
    const float pi = 3.14159265358979323846f;

    // Check points on the circumference of this "effective" safety circle.
    for (int i = 0; i < numCircumferencePoints; ++i) {
        float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(numCircumferencePoints);
        float marginCheckX = x + effectiveRadiusForMarginCheck * std::cos(angle);
        float marginCheckY = y + effectiveRadiusForMarginCheck * std::sin(angle);

        // Check if this margin-aware point is out of game bounds.
        // If it is, (x,y) is too close to a boundary to maintain the required distance.
        if (marginCheckX < 0.0f || marginCheckX >= GRID_SIZE ||
            marginCheckY < 0.0f || marginCheckY >= GRID_SIZE) {
            // std::cout << "isPositionValid: FALSE (Margin check point out of bounds: " << marginCheckX << ", " << marginCheckY << ") for center (" << x << ", " << y << ") with effective radius " << effectiveRadiusForMarginCheck << std::endl;
            return false;
        }

        // Check if this margin-aware point is inside a non-traversable map block.
        if (wouldCollideWithMapBlock(marginCheckX, marginCheckY, gameMap, nonTraversableBlocks)) {
            // std::cout << "isPositionValid: FALSE (Map block collision at margin check point " << i << ": " << marginCheckX << ", " << marginCheckY << ") for center (" << x << ", " << y << ") with effective radius " << effectiveRadiusForMarginCheck << std::endl;
            return false; 
        }
    }

    // If all checks pass, the position is valid for the entity.
    // std::cout << "isPositionValid: TRUE for (" << x << ", " << y << ") radius " << entityCollisionRadius << " with margin " << MIN_DISTANCE_FROM_NON_WALKABLE_BLOCKS << std::endl;
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

// Helper function to check if a line segment is valid by sampling points
static bool isSegmentValid(float x1, float y1, float x2, float y2,
                           float entityCollisionRadius,
                           const Map& gameMap,
                           const std::set<TextureName>& nonTraversableBlocks) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance < 0.001f) { // Essentially the same point
        return isPositionValid(x1, y1, entityCollisionRadius, gameMap, nonTraversableBlocks);
    }

    // Step size for sampling, e.g., half the collision radius.
    // Ensure a minimum step size to prevent issues with very small radii and to ensure progress.
    float stepSize = entityCollisionRadius * 0.5f;
    if (stepSize < 0.01f) { // Minimum practical step size to avoid excessive checks or division by zero if radius is 0.
        stepSize = 0.01f;
    }
    if (stepSize == 0.0f) { // If radius was 0 and somehow stepSize became 0
         stepSize = 0.1f; // A fallback small step
    }


    int numSteps = static_cast<int>(distance / stepSize);
    if (numSteps == 0 && distance > 0.001f) numSteps = 1; // Ensure at least one check for distinct points if distance > epsilon

    // Check points along the segment, including endpoints
    for (int i = 0; i <= numSteps; ++i) {
        float t = (numSteps == 0) ? 0.0f : static_cast<float>(i) / static_cast<float>(numSteps);
        // Ensure the last point is exactly (x2, y2) to avoid floating point drift
        if (i == numSteps) t = 1.0f; 

        float currentX = x1 + t * dx;
        float currentY = y1 + t * dy;

        if (!isPositionValid(currentX, currentY, entityCollisionRadius, gameMap, nonTraversableBlocks)) {
            // For debugging:
            // std::cout << "isSegmentValid: FALSE at point (" << currentX << ", " << currentY 
            //           << ") on segment (" << x1 << "," << y1 << ")-(" << x2 << "," << y2 
            //           << ") with radius " << entityCollisionRadius << ", step " << i << "/" << numSteps << std::endl;
            return false;
        }
    }
    return true;
}

// Simplify path using "string pulling" method, ensuring segments are valid
static void simplifyPath(std::vector<std::pair<float, float>>& path,
                         float entityCollisionRadius,
                         const Map& gameMap,
                         const std::set<TextureName>& nonTraversableBlocks) {
    if (path.size() <= 2) {
        // For paths of 0, 1, or 2 points, no simplification is typically needed.
        // However, if it's 2 points, ensure the direct segment is valid.
        // This is more of a sanity check as A* should provide valid segments between adjacent cells.
        // The main findPath logic handles snapping, so this simplifyPath assumes input path points are "reasonable".
        if (path.size() == 2) {
            if (!isSegmentValid(path[0].first, path[0].second, path[1].first, path[1].second,
                                entityCollisionRadius, gameMap, nonTraversableBlocks)) {
                // If the direct segment between the two points is invalid,
                // this indicates a potential issue upstream (e.g. A* produced only start/goal
                // but direct line is blocked). In such a case, simplification can't fix it,
                // and we might return the original path or a modified one.
                // For now, we'll let it be, as findPath re-snaps.
                // A more robust solution might involve returning only the start point if the segment is bad.
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
                                   entityCollisionRadius, gameMap, nonTraversableBlocks)) {
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


// Find path using A* algorithm
std::vector<std::pair<float, float>> findPath(
    float startX, float startY,
    float goalX, float goalY,
    const Map& gameMap,
    float collisionRadius,
    const std::set<TextureName>& nonTraversableBlocks
) {
    // Store original intended goal for messages, and derive initial cell indices
    float originalGoalX = goalX;
    float originalGoalY = goalY;
    int initialStartNodeX = static_cast<int>(startX);
    int initialStartNodeY = static_cast<int>(startY);
    int initialGoalNodeX = static_cast<int>(goalX);
    int initialGoalNodeY = static_cast<int>(goalY);

    // Check and adjust start position
    if (!isPositionValid(startX, startY, collisionRadius, gameMap, nonTraversableBlocks)) {
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
                    if (isPositionValid(testX, testY, collisionRadius, gameMap, nonTraversableBlocks)) {
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
    }

    // Check and adjust goal position
    if (!isPositionValid(goalX, goalY, collisionRadius, gameMap, nonTraversableBlocks)) {
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
                    if (isPositionValid(testX, testY, collisionRadius, gameMap, nonTraversableBlocks)) {
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
            std::cerr << "Pathfinding Details: Start (" << startX << ", " << startY << ") Goal (" << goalX << ", " << goalY << ") Radius (" << collisionRadius << ")" << std::endl;
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
                path.back() = {goalX, goalY};

                // Simplify the path using the new robust method
                simplifyPath(path, collisionRadius, gameMap, nonTraversableBlocks); 

                // Ensure simplification didn't break start/end points, or if path became very short
                if (!path.empty()) {
                    path[0] = {startX, startY}; // Re-snap start
                    // If simplifyPath results in a single point, goalX/Y might not be path.back() if startX/Y was that point.
                    // Ensure goal is the last point if path has more than one point, or if path is just goal.
                    if (path.size() > 1) {
                        path.back() = {goalX, goalY}; // Re-snap end
                    } else { // Path has 1 point. If it's not goalX/Y, it's fine.
                             // If startX/Y != goalX/Y but path became 1 point, it should be startX/Y. Add goalX/Y.
                        if (std::abs(startX - goalX) > 0.001f || std::abs(startY - goalY) > 0.001f) {
                            if (path[0].first != goalX || path[0].second != goalY) { // if the single point isn't already the goal
                                path.push_back({goalX, goalY});
                            }
                        }
                    }
                } else { // Path became empty after simplification (e.g. start and goal were same and simplifyPath reduced it)
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

        for (const auto& neighborPos : getNeighbors(currentNode->x, currentNode->y, collisionRadius, gameMap, nonTraversableBlocks)) {
            if (closedSet.count(neighborPos)) {
                continue;
            }

            float tentativeGCost = currentNode->gCost + 1.0f; 
            // Diagonal movement cost (optional, can be sqrt(2) or different if desired)
            // if (std::abs(neighborPos.first - currentNode->x) == 1 && std::abs(neighborPos.second - currentNode->y) == 1) {
            //    tentativeGCost = currentNode->gCost + 1.414f; // Cost for diagonal
            // }


            Node* neighborNode = nullptr;
            auto it = allNodes.find(neighborPos);
            if (it != allNodes.end()) {
                neighborNode = it->second;
            } else {
                neighborNode = new Node(neighborPos.first, neighborPos.second);
                allNodes[neighborPos] = neighborNode;
                 // Initialize gCost to a high value if new, so any path is better
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
    std::cerr << "-------------------------------------------------\\\\" << std::endl;
    std::cerr << "Pathfinding: No path found! Details:" << std::endl;
    std::cerr << "  Start (final used): (" << startX << ", " << startY << "), Node: (" << startNodeX << ", " << startNodeY << ")" << std::endl;
    std::cerr << "  Goal (final used):  (" << goalX << ", " << goalY << "), Node: (" << goalNodeX << ", " << goalNodeY << ")" << std::endl;
    std::cerr << "  Original Goal:      (" << originalGoalX << ", " << originalGoalY << ")" << std::endl;
    std::cerr << "  Collision Radius:   " << collisionRadius << std::endl;
    std::cerr << "  Open Set Status:    " << (openSet.empty() ? "Empty" : "Not Empty (Error in logic?)") << std::endl;
    std::cerr << "  Nodes Explored (Closed Set Size): " << closedSet.size() << std::endl;
    // std::cerr << "  Total Nodes Considered (AllNodes map size at end): " << allNodes.size() << std::endl; // allNodes is cleared
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

    std::cerr << "  Non-Traversable Blocks for this entity:";
    if (nonTraversableBlocks.empty()) {
        std::cerr << " (None specified)" << std::endl;
    } else {
        std::cerr << std::endl;
        for (const auto& block : nonTraversableBlocks) {
            std::cerr << "    - " << static_cast<int>(block) << std::endl; 
        }
    }
    std::cerr << "-------------------------------------------------\\" << std::endl;

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

// NEW FUNCTIONS USING EntityConfiguration FOR COLLISION SHAPE POINTS

// Check if a position is valid using EntityConfiguration
bool isPositionValid(float x, float y, const EntityConfiguration& entityConfig, const Map& gameMap) {
    // 1. Check if the entity's entire collision body is within game bounds
    // For collision shape points, we need to check all points of the collision shape
    if (!entityConfig.collisionShapePoints.empty()) {
        // Check bounds using collision shape points
        for (const auto& point : entityConfig.collisionShapePoints) {
            float pointX = x + point.first;
            float pointY = y + point.second;
            
            if (pointX < 0.0f || pointX >= GRID_SIZE || pointY < 0.0f || pointY >= GRID_SIZE) {
                return false; // Shape would be partially or fully out of bounds
            }
        }
    } else {
        // Fallback to collision radius if no collision shape points defined
        if (x - entityConfig.collisionRadius < 0.0f || x + entityConfig.collisionRadius >= GRID_SIZE ||
            y - entityConfig.collisionRadius < 0.0f || y + entityConfig.collisionRadius >= GRID_SIZE) {
            return false;
        }
    }

    // 2. Check for collision with other collidable elements in the game
    if (wouldEntityCollideWithElement(entityConfig, x, y)) {
        return false;
    }
    
    // Apply safety margin for pathfinding by expanding the collision check slightly
    // For collision shape points, we create a slightly larger version
    if (!entityConfig.collisionShapePoints.empty()) {
        // Create expanded collision shape points for safety margin
        std::vector<std::pair<float, float>> expandedShapePoints;
        for (const auto& point : entityConfig.collisionShapePoints) {
            // Expand each point outward from center by the safety margin
            float expandedX = point.first;
            float expandedY = point.second;
            
            // Apply expansion based on distance from center
            float distance = std::sqrt(expandedX * expandedX + expandedY * expandedY);
            if (distance > 0.001f) { // Avoid division by zero
                float expansionFactor = (distance + MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS) / distance;
                expandedX *= expansionFactor;
                expandedY *= expansionFactor;
            } else {
                // Point is at center, expand by margin in all directions (create a small square)
                expandedShapePoints.push_back({-MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS, -MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS});
                expandedShapePoints.push_back({MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS, -MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS});
                expandedShapePoints.push_back({MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS, MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS});
                expandedShapePoints.push_back({-MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS, MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS});
                continue;
            }
            expandedShapePoints.push_back({expandedX, expandedY});
        }
        
        // Check collision with expanded shape
        if (wouldEntityCollideWithElement(x, y, expandedShapePoints, 1.0f, 0.0f)) {
            return false;
        }
    } else {
        // Fallback to radius-based collision check with margin
        float effectiveRadiusForElementCheck = entityConfig.collisionRadius + MIN_DISTANCE_FROM_NON_WALKABLE_ELEMENTS;
        if (wouldCollideWithElement(x, y, effectiveRadiusForElementCheck)) {
            return false;
        }
    }

    // 3. Check collision with non-traversable map blocks
    if (wouldCollideWithMapBlock(x, y, gameMap, entityConfig.nonTraversableBlocks)) {
        return false;
    }
    
    // Check margin around collision shape for map blocks
    if (!entityConfig.collisionShapePoints.empty()) {
        // Check points around the collision shape boundary for map block safety margin
        for (const auto& point : entityConfig.collisionShapePoints) {
            float checkX = x + point.first;
            float checkY = y + point.second;
            
            // Check points around this collision shape point
            const int numCircumferencePoints = 4; // Fewer points for performance
            const float pi = 3.14159265358979323846f;
            
            for (int i = 0; i < numCircumferencePoints; ++i) {
                float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(numCircumferencePoints);
                float marginCheckX = checkX + MIN_DISTANCE_FROM_NON_WALKABLE_BLOCKS * std::cos(angle);
                float marginCheckY = checkY + MIN_DISTANCE_FROM_NON_WALKABLE_BLOCKS * std::sin(angle);
                
                // Check bounds
                if (marginCheckX < 0.0f || marginCheckX >= GRID_SIZE ||
                    marginCheckY < 0.0f || marginCheckY >= GRID_SIZE) {
                    return false;
                }
                
                // Check map block collision
                if (wouldCollideWithMapBlock(marginCheckX, marginCheckY, gameMap, entityConfig.nonTraversableBlocks)) {
                    return false;
                }
            }
        }
    } else {
        // Fallback to radius-based margin check
        float effectiveRadiusForMarginCheck = entityConfig.collisionRadius + MIN_DISTANCE_FROM_NON_WALKABLE_BLOCKS;
        const int numCircumferencePoints = 8;
        const float pi = 3.14159265358979323846f;
        
        for (int i = 0; i < numCircumferencePoints; ++i) {
            float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(numCircumferencePoints);
            float marginCheckX = x + effectiveRadiusForMarginCheck * std::cos(angle);
            float marginCheckY = y + effectiveRadiusForMarginCheck * std::sin(angle);
            
            if (marginCheckX < 0.0f || marginCheckX >= GRID_SIZE ||
                marginCheckY < 0.0f || marginCheckY >= GRID_SIZE) {
                return false;
            }
            
            if (wouldCollideWithMapBlock(marginCheckX, marginCheckY, gameMap, entityConfig.nonTraversableBlocks)) {
                return false;
            }
        }
    }

    return true;
}

// Get neighbors using EntityConfiguration
std::vector<std::pair<int, int>> getNeighbors(int x, int y, const EntityConfiguration& entityConfig, const Map& gameMap) {
    std::vector<std::pair<int, int>> neighbors;
    
    // 8-directional movement (including diagonals)
    const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    for (int i = 0; i < 8; ++i) {
        int newX = x + directions[i][0];
        int newY = y + directions[i][1];
        
        // Check bounds
        if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE) {
            // Use the EntityConfiguration version of isPositionValid
            if (isPositionValid(static_cast<float>(newX) + 0.5f, static_cast<float>(newY) + 0.5f, 
                              entityConfig, gameMap)) {
                neighbors.push_back({newX, newY});
            }
        }
    }
    
    return neighbors;
}

// Helper function to check if a line segment is valid using EntityConfiguration
static bool isSegmentValid(float x1, float y1, float x2, float y2,
                           const EntityConfiguration& entityConfig,
                           const Map& gameMap) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance < 0.001f) { // Essentially the same point
        return isPositionValid(x1, y1, entityConfig, gameMap);
    }

    // Step size for sampling - use smaller of collision radius or fixed step
    float stepSize = 0.1f; // Fixed small step size for collision shape points
    if (entityConfig.collisionShapePoints.empty()) {
        stepSize = entityConfig.collisionRadius * 0.5f;
        if (stepSize < 0.01f) {
            stepSize = 0.01f;
        }
    }
    
    int numSteps = static_cast<int>(std::ceil(distance / stepSize));
    if (numSteps == 0) numSteps = 1;

    for (int i = 0; i <= numSteps; ++i) {
        float t = (numSteps == 0) ? 0.0f : static_cast<float>(i) / static_cast<float>(numSteps);
        if (i == numSteps) t = 1.0f; // Ensure the last point is exactly (x2, y2)

        float currentX = x1 + t * dx;
        float currentY = y1 + t * dy;

        if (!isPositionValid(currentX, currentY, entityConfig, gameMap)) {
            return false;
        }
    }
    return true;
}

// Simplify path using EntityConfiguration
static void simplifyPath(std::vector<std::pair<float, float>>& path,
                         const EntityConfiguration& entityConfig,
                         const Map& gameMap) {
    if (path.size() <= 2) {
        if (path.size() == 2) {
            if (!isSegmentValid(path[0].first, path[0].second, path[1].first, path[1].second,
                                entityConfig, gameMap)) {
                // If direct segment is invalid, keep the original path
                return;
            }
        }
        return;
    }

    std::vector<std::pair<float, float>> simplifiedPath;
    simplifiedPath.push_back(path[0]); // Start point

    size_t currentIndex = 0;
    while (currentIndex < path.size() - 1) {
        size_t farthestReachable = currentIndex + 1;
        
        // Find the farthest point we can reach directly from current point
        for (size_t i = currentIndex + 2; i < path.size(); ++i) {
            if (isSegmentValid(path[currentIndex].first, path[currentIndex].second,
                               path[i].first, path[i].second, entityConfig, gameMap)) {
                farthestReachable = i;
            } else {
                break; // Can't reach this point, stop checking further
            }
        }
        
        currentIndex = farthestReachable;
        simplifiedPath.push_back(path[currentIndex]);
    }

    path = simplifiedPath;
}

// Find path using EntityConfiguration
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
    int initialGoalNodeY = static_cast<int>(goalY);

    // Check and adjust start position using new collision shape points system
    if (!isPositionValid(startX, startY, entityConfig, gameMap)) {
        std::cout << "Pathfinding: Start position (" << startX << ", " << startY << ") is invalid. Searching for a nearby valid start..." << std::endl;
        bool foundValidStart = false;
        for (int r = 0; r <= 3; ++r) { // Search radius
            for (int dx_offset = -r; dx_offset <= r; ++dx_offset) {
                for (int dy_offset = -r; dy_offset <= r; ++dy_offset) {
                    if (r > 0 && (std::abs(dx_offset) < r && std::abs(dy_offset) < r)) {
                        continue; 
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
    }

    // Check and adjust goal position using new collision shape points system
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

    // Calculate straight-line distance and check if it's too far
    float dx = goalX - startX;
    float dy = goalY - startY;
    float distance = std::sqrt(dx * dx + dy * dy);
    if (distance > MAX_PATHFINDING_DISTANCE) {
        std::cerr << "Pathfinding Error: Distance (" << distance << ") exceeds maximum pathfinding distance (" << MAX_PATHFINDING_DISTANCE << ")." << std::endl;
        return {};
    }

    // Convert adjusted start and goal to node coordinates
    int startNodeX = static_cast<int>(startX);
    int startNodeY = static_cast<int>(startY);
    int goalNodeX = static_cast<int>(goalX);
    int goalNodeY = static_cast<int>(goalY);

    // A* algorithm using collision shape points
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
    const int maxIterations = GRID_SIZE * GRID_SIZE * 4;

    while (!openSet.empty()) {
        iterations++;
        if (iterations > maxIterations) {
            std::cerr << "Pathfinding: Exceeded maximum iterations (" << maxIterations << "). Aborting search." << std::endl;
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
                // Snap endpoints to the (guaranteed valid or adjusted) startX/Y and goalX/Y BEFORE simplification
                path[0] = {startX, startY};
                path.back() = {goalX, goalY};

                // Simplify the path using the new EntityConfiguration-based method
                simplifyPath(path, entityConfig, gameMap); 

                // Ensure simplification didn't break start/end points
                if (!path.empty()) {
                    path[0] = {startX, startY}; // Re-snap start
                    if (path.size() > 1) {
                        path.back() = {goalX, goalY}; // Re-snap end
                    } else { // Path has 1 point. If it's not goalX/Y, it's fine.
                        if (std::abs(startX - goalX) > 0.001f || std::abs(startY - goalY) > 0.001f) {
                            if (path[0].first != goalX || path[0].second != goalY) { // if the single point isn't already the goal
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

        // Get neighbors using EntityConfiguration
        std::vector<std::pair<int, int>> neighbors = getNeighbors(currentNode->x, currentNode->y, entityConfig, gameMap);
        for (const auto& neighbor : neighbors) {
            int neighborX = neighbor.first;
            int neighborY = neighbor.second;

            if (closedSet.find({neighborX, neighborY}) != closedSet.end()) {
                continue; // Skip already processed nodes
            }

            float tentativeGCost = currentNode->gCost + 1.0f; // Cost to move to neighbor

            Node* neighborNode;
            auto it = allNodes.find({neighborX, neighborY});
            if (it != allNodes.end()) {
                neighborNode = it->second;
            } else {
                neighborNode = new Node(neighborX, neighborY);
                allNodes[{neighborX, neighborY}] = neighborNode;
            }

            if (tentativeGCost < neighborNode->gCost || neighborNode->gCost == 0) {
                neighborNode->gCost = tentativeGCost;
                neighborNode->hCost = calculateHeuristic(neighborX, neighborY, goalNodeX, goalNodeY);
                neighborNode->fCost = neighborNode->gCost + neighborNode->hCost;
                neighborNode->parent = currentNode;

                // Check if this neighbor is already in the open set
                bool inOpenSet = false;
                std::priority_queue<Node*, std::vector<Node*>, CompareNodes> tempQueue = openSet;
                while (!tempQueue.empty()) {
                    if (tempQueue.top()->x == neighborX && tempQueue.top()->y == neighborY) {
                        inOpenSet = true;
                        break;
                    }
                    tempQueue.pop();
                }

                if (!inOpenSet) {
                    openSet.push(neighborNode);
                }
            }
        }
    }

    // No path found
    std::cerr << "Pathfinding: No path found from (" << startX << ", " << startY << ") to (" << goalX << ", " << goalY << ") using collision shape points." << std::endl;
    for (auto& pair_node : allNodes) { delete pair_node.second; }
    allNodes.clear();
    return {};
}
