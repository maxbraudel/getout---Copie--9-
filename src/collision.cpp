#include "collision.h"
#include "elementsOnMap.h"
#include "entities.h"  // Added for entitiesManager
#include "globals.h"  // Added for GRID_SIZE
#include "GLFW/glfw3.h" // Added for glfwGetTime
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>  // Added for spatial partitioning
#include <limits> // Added for std::numeric_limits

// Forward declaration for entitiesManager from entities.cpp
extern EntitiesManager entitiesManager;

// Define M_PI if not already defined (needed for angle calculations)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Cache for collidable element names
static std::vector<std::string> collidableElementNames;
static bool collisionCacheInitialized = false;

// Spatial partitioning grid for more efficient collision detection
// This divides the game world into a grid of cells, and only checks collisions
// with elements in the same or neighboring cells
const int SPATIAL_GRID_SIZE = 10; // Size of each spatial grid cell
static std::unordered_map<int, std::vector<std::string>> spatialGrid;
bool spatialGridInitialized = false;
static float lastSpatialGridUpdateTime = 0.0f;

// Last time debug messages were printed to reduce spam
static float lastCollisionDebugTime = 0.0f;

// Define the set of non-traversable blocks
std::set<TextureName> nonTraversableBlocks = {
    TextureName::WATER_1,
    TextureName::WATER_2,
    TextureName::WATER_3,
    TextureName::WATER_4
    // Add more block types here as needed
};

// Function to add a block type to the non-traversable set
void addNonTraversableBlock(TextureName blockType) {
    nonTraversableBlocks.insert(blockType);
    std::cout << "Added block type " << static_cast<int>(blockType) << " to non-traversable blocks." << std::endl;
}

// Function to remove a block type from the non-traversable set
void removeNonTraversableBlock(TextureName blockType) {
    auto it = nonTraversableBlocks.find(blockType);
    if (it != nonTraversableBlocks.end()) {
        nonTraversableBlocks.erase(it);
        std::cout << "Removed block type " << static_cast<int>(blockType) << " from non-traversable blocks." << std::endl;
    }
}

// Function to check if a block type is non-traversable
bool isBlockNonTraversable(TextureName blockType) {
    return nonTraversableBlocks.find(blockType) != nonTraversableBlocks.end();
}

// Function to clear all non-traversable blocks
void clearNonTraversableBlocks() {
    nonTraversableBlocks.clear();
    std::cout << "Cleared all non-traversable blocks." << std::endl;
}

// Function to print all non-traversable block types
void printNonTraversableBlocks() {
    std::cout << "Non-traversable block types (" << nonTraversableBlocks.size() << " total):" << std::endl;
    for (const auto& blockType : nonTraversableBlocks) {
        // std::cout << " - " << getBlockTypeName(blockType) << " (enum: " << static_cast<int>(blockType) << ")" << std::endl;
    }
}

// Function to get all collidable element names in the game
std::vector<std::string> getCollidableElementNames() {
    static float lastCacheUpdateTime = 0.0f;
    static bool firstInit = true;
    
    // Only update cache periodically to improve performance
    float currentTime = static_cast<float>(glfwGetTime());
    if (!collisionCacheInitialized || currentTime - lastCacheUpdateTime > 2.0f) {
        // Update the cache when it's stale or needs initializing
        collidableElementNames.clear();
        lastCacheUpdateTime = currentTime;
        
        // Get all elements and filter for those with collision enabled
        const auto& elements = elementsManager.getElements();
        for (const auto& element : elements) {
            // Check if the element has collision enabled
            if (element.hasCollision) {
                collidableElementNames.push_back(element.instanceName);
            }
        }
        
        collisionCacheInitialized = true;
        
        // Only print this message on first initialization to reduce log spam
        if (firstInit) {
            std::cout << "Initialized collision system with " << collidableElementNames.size() << " collidable elements." << std::endl;
            firstInit = false;
        }
    }
    
    return collidableElementNames;
}

// Define a constant maximum collision range to optimize element checks
const float MAX_COLLISION_CHECK_RANGE = 3.0f; // Only check elements within this range

// Function to check if a position would collide with a collidable element
bool wouldCollideWithElement(float x, float y, float playerRadius) {
    // Make sure the spatial grid is up to date
    if (!spatialGridInitialized) {
        updateSpatialGrid();
    }
    
    // Get only nearby elements instead of checking all elements
    std::vector<std::string> nearbyElements = getNearbyElements(x, y, playerRadius + MAX_COLLISION_CHECK_RANGE);
    
    // Collision detection counters for performance monitoring
    static int collisionCheckCount = 0;
    static int collisionHitCount = 0;
    
    // Check distance to each nearby element
    for (const auto& elementName : nearbyElements) {
        float elementX, elementY;
        // Also need element scale and rotation for transforming polygon points
        float elementScale = 1.0f;
        float elementRotation = 0.0f;
        std::vector<std::pair<float, float>> elementCollisionShapePoints;
        bool elementHasCollision = false;

        // Find the element to get its actual collision properties
        const auto& elements = elementsManager.getElements();
        const PlacedElement* currentElement = nullptr;
        for (const auto& el : elements) {
            if (el.instanceName == elementName) {
                currentElement = &el;
                break;
            }
        }

        if (!currentElement || !currentElement->hasCollision || currentElement->collisionShapePoints.empty()) {
            continue; // Skip elements without collision enabled or without defined shape
        }

        elementX = currentElement->x;
        elementY = currentElement->y;
        elementScale = currentElement->scale; // Assuming uniform scaling for now
        elementRotation = currentElement->rotation; // In degrees
        elementCollisionShapePoints = currentElement->collisionShapePoints;
        elementHasCollision = true;

        collisionCheckCount++;

        // Transform polygon points to world coordinates
        std::vector<std::pair<float, float>> worldShapePoints;
        float angleRad = elementRotation * M_PI / 180.0f; // Convert rotation to radians
        float cosA = cos(angleRad);
        float sinA = sin(angleRad);

        for (const auto& localPoint : elementCollisionShapePoints) {
            // Scale first, then rotate, then translate
            float scaledX = localPoint.first * elementScale;
            float scaledY = localPoint.second * elementScale;
            
            float rotatedX = scaledX * cosA - scaledY * sinA;
            float rotatedY = scaledX * sinA + scaledY * cosA;
            
            worldShapePoints.push_back({elementX + rotatedX, elementY + rotatedY});
        }

        // Perform circle-polygon collision detection
        // For simplicity, we can use a separating axis theorem (SAT) based approach or a point-in-polygon test for the circle's center
        // followed by checking distance to polygon edges if the center is outside.
        // A robust solution is complex. Here's a simplified version:
        // 1. Check if player center is inside the polygon.
        // 2. If not, find the closest point on polygon to player center and check distance.

        bool collision = false;
        // Step 1: Point-in-polygon test (Ray Casting)
        int intersectCount = 0;
        for (size_t i = 0; i < worldShapePoints.size(); ++i) {
            std::pair<float, float> p1 = worldShapePoints[i];
            std::pair<float, float> p2 = worldShapePoints[(i + 1) % worldShapePoints.size()];

            // Check if player.y is between p1.y and p2.y
            if (((p1.second <= y && y < p2.second) || (p2.second <= y && y < p1.second)) &&
                // And player.x is to the left of the intersection point of the ray and the edge
                (x < (p2.first - p1.first) * (y - p1.second) / (p2.second - p1.second) + p1.first)) {
                intersectCount++;
            }
        }
        if (intersectCount % 2 == 1) {
            collision = true; // Player center is inside polygon
        }

        // Step 2: If center is not inside, check distance to polygon edges
        if (!collision) {
            float minDistSq = std::numeric_limits<float>::max();
            for (size_t i = 0; i < worldShapePoints.size(); ++i) {
                std::pair<float, float> p1 = worldShapePoints[i];
                std::pair<float, float> p2 = worldShapePoints[(i + 1) % worldShapePoints.size()];

                // Calculate closest point on segment (p1, p2) to player (x,y)
                float lenSq = (p2.first - p1.first)*(p2.first - p1.first) + (p2.second - p1.second)*(p2.second - p1.second);
                if (lenSq == 0.0) { // p1 and p2 are the same
                    minDistSq = std::min(minDistSq, (x - p1.first)*(x - p1.first) + (y - p1.second)*(y - p1.second));
                    continue;
                }
                float t = ((x - p1.first)*(p2.first - p1.first) + (y - p1.second)*(p2.second - p1.second)) / lenSq;
                t = std::max(0.0f, std::min(1.0f, t)); // Clamp t to [0,1]
                float closestX = p1.first + t * (p2.first - p1.first);
                float closestY = p1.second + t * (p2.second - p1.second);
                float distSq = (x - closestX)*(x - closestX) + (y - closestY)*(y - closestY);
                minDistSq = std::min(minDistSq, distSq);
            }
            if (minDistSq < (playerRadius * playerRadius)) {
                collision = true;
            }
        }

        if (collision) {
            collisionHitCount++;
            float currentTime = static_cast<float>(glfwGetTime());
            if (playerDebugMode && currentTime - lastCollisionDebugTime > 2.0f) {
                lastCollisionDebugTime = currentTime;
                std::cout << "Collision with " << elementName 
                          << " (polygon) at player pos (" << x << ", " << y << ")" 
                          << ", element center: (" << elementX << ", " << elementY << ")" << std::endl;
                // Further debug info can be added here if needed
            }
            return true; // Collision detected
        }
    }
    
    // If we got here, no collision was detected
    return false;
}

// Reset the elements cache when new elements are added
void resetCollisionCache() {
    collisionCacheInitialized = false;
    spatialGridInitialized = false;
}

// Get spatial grid cell index from world coordinates
int getSpatialGridIndex(float x, float y) {
    int gridX = static_cast<int>(x) / SPATIAL_GRID_SIZE;
    int gridY = static_cast<int>(y) / SPATIAL_GRID_SIZE;
    // Use a simple hash function to convert 2D grid coordinates to a 1D index
    return gridX * 1000 + gridY; // This should be sufficient for our grid size
}

// Update the spatial partitioning grid
void updateSpatialGrid() {
    float currentTime = static_cast<float>(glfwGetTime());
    
    // Only update every 0.5 seconds to avoid performance impact
    if (spatialGridInitialized && currentTime - lastSpatialGridUpdateTime < 0.5f) {
        return;
    }
    
    lastSpatialGridUpdateTime = currentTime;
    spatialGrid.clear();
    
    // Get all collidable elements
    const auto& collidables = getCollidableElementNames();
    
    // Place each element in the appropriate grid cell
    for (const auto& elementName : collidables) {
        float x, y;
        if (elementsManager.getElementPosition(elementName, x, y)) {
            int index = getSpatialGridIndex(x, y);
            spatialGrid[index].push_back(elementName);
        }
    }
    
    spatialGridInitialized = true;
}

// Get all elements in the vicinity of a position
std::vector<std::string> getNearbyElements(float x, float y, float radius) {
    std::vector<std::string> result;
    
    // Make sure the spatial grid is up to date
    if (!spatialGridInitialized) {
        updateSpatialGrid();
    }
    
    // Calculate the grid cell range that could contain elements within the radius
    int cellRadius = static_cast<int>(radius / SPATIAL_GRID_SIZE) + 1;
    int centerCellX = static_cast<int>(x) / SPATIAL_GRID_SIZE;
    int centerCellY = static_cast<int>(y) / SPATIAL_GRID_SIZE;
    
    // Check all cells in the vicinity
    for (int cellX = centerCellX - cellRadius; cellX <= centerCellX + cellRadius; cellX++) {
        for (int cellY = centerCellY - cellRadius; cellY <= centerCellY + cellRadius; cellY++) {
            int index = cellX * 1000 + cellY;
            
            // Get all elements in this grid cell
            auto it = spatialGrid.find(index);
            if (it != spatialGrid.end()) {
                // Add elements from this cell to the result
                result.insert(result.end(), it->second.begin(), it->second.end());
            }
        }
    }
    
    return result;
}

// Function to check if a position would collide with a non-traversable map block
bool wouldCollideWithMapBlock(float x, float y, const Map& gameMap) {
    // Convert floating point coordinates to grid integers
    int gridX = static_cast<int>(x);
    int gridY = static_cast<int>(y);
    
    // Skip invalid coordinates 
    if (gridX < 0 || gridX >= GRID_SIZE || gridY < 0 || gridY >= GRID_SIZE) {
        return true; // Treat out-of-bounds as collision
    }
    
    // Get the texture type at these coordinates
    TextureName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
    
    // Check if this block type is in our set of non-traversable blocks
    if (nonTraversableBlocks.find(blockType) != nonTraversableBlocks.end()) {
        // Throttle debug output much more aggressively
        static int mapCollisionCounter = 0;
        static float lastMapDebugTime = 0.0f;
        float currentTime = static_cast<float>(glfwGetTime());
        
        if (playerDebugMode && currentTime - lastMapDebugTime > 5.0f) {
            lastMapDebugTime = currentTime;
            std::cout << "Map block collision at (" << x << ", " << y 
                      << ") - Block type: " << static_cast<int>(blockType) << std::endl;
        }
        return true; // Collision detected with non-traversable block
    }
    
    return false; // No collision with non-traversable blocks
}

// Overloaded function that uses entity-specific non-traversable blocks
bool wouldCollideWithMapBlock(float x, float y, const Map& gameMap, const std::set<TextureName>& entityNonTraversableBlocks) {
    // Convert floating point coordinates to grid integers
    int gridX = static_cast<int>(x);
    int gridY = static_cast<int>(y);
    
    // Skip invalid coordinates 
    if (gridX < 0 || gridX >= GRID_SIZE || gridY < 0 || gridY >= GRID_SIZE) {
        return true; // Treat out-of-bounds as collision
    }
    
    // Get the texture type at these coordinates
    TextureName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
    
    // Check if this block type is in the entity's set of non-traversable blocks
    if (entityNonTraversableBlocks.find(blockType) != entityNonTraversableBlocks.end()) {
        // Throttle debug output much more aggressively
        static int mapCollisionCounter = 0;
        static float lastMapDebugTime = 0.0f;
        float currentTime = static_cast<float>(glfwGetTime());
        
        if (playerDebugMode && currentTime - lastMapDebugTime > 5.0f) {
            lastMapDebugTime = currentTime;
            std::cout << "Entity-specific map block collision at (" << x << ", " << y 
                      << ") - Block type: " << static_cast<int>(blockType) << std::endl;
        }
        return true; // Collision detected with non-traversable block
    }
    
    return false; // No collision with non-traversable blocks
}

// Function to find a safe position when a character is stuck inside a collision area
bool findSafePosition(float& x, float& y, float playerRadius, const Map& gameMap) {
    // Get the player's non-traversable blocks from the entity system
    const EntityConfiguration* playerConfig = entitiesManager.getConfiguration("player");
    if (!playerConfig) {
        std::cerr << "Error: Player entity configuration not found in findSafePosition" << std::endl;
        return false; // Cannot proceed without player configuration
    }
      // First, check if the current position is already safe
    bool hasElementCollision = wouldCollideWithElement(x, y, playerRadius);
    
    if (!hasElementCollision) {
        return false; // No adjustment needed, position is already safe
    }
    
    if (playerDebugMode) {
        std::cout << "Character found stuck in collision at (" << x << ", " << y 
                  << "), attempting to find safe position..." << std::endl;
    }
    
    // Try to find a safe position by checking in a spiral pattern around the current position
    // This is more efficient than checking in a grid and handles both element and map block collisions
    
    const float step = 0.1f;  // Step size for each potential position check
    const float maxDistance = 15.0f; // Increased maximum distance to check for safe position (was 5.0f)
    float distance = step;
      // Try to find a safe position by spiraling outward
    while (distance <= maxDistance) {
        // Try positions in a circle at current distance
        // Use more angles for finer granularity as the search expands
        float angleStep = 0.2f;
        if (distance > 2.0f) angleStep = 0.1f; // Finer search at greater distances
        
        for (float angle = 0.0f; angle < 2.0f * M_PI; angle += angleStep) {            float testX = x + distance * cos(angle);
            float testY = y + distance * sin(angle);
              // Check if this position is safe
            if (!wouldCollideWithElement(testX, testY, playerRadius)) {
                // Found a safe position, update coordinates and return
                if (playerDebugMode) {
                    std::cout << "Safe position found at (" << testX << ", " << testY 
                              << "), moved character " << distance << " units" << std::endl;
                }
                
                x = testX;
                y = testY;
                return true; // Position was adjusted
            }
        }
        
        // Increase distance for next search ring
        distance += step;
    }
      // If no safe position found in spiral search, try a grid search as fallback
    // This helps in cases where the map has complex collision patterns
    if (playerDebugMode) {
        std::cout << "Spiral search failed, trying grid search with expanded distance..." << std::endl;
    }
    
    // Increase max distance for the grid search to find positions farther away if needed
    const float expandedMaxDistance = maxDistance * 1.5f;
    
    // Try a grid search in a square around the player
    // Use an adaptive step size that gets larger as we move farther from the player
    // for better performance while still maintaining precision near the player
    for (float searchDist = maxDistance; searchDist <= expandedMaxDistance; searchDist += maxDistance * 0.5f) {
        // Step size increases with distance
        float adaptiveStep = step * (1.0f + searchDist / 5.0f);
        
        // Search at this particular distance
        for (float dx = -searchDist; dx <= searchDist; dx += adaptiveStep) {
            for (float dy = -searchDist; dy <= searchDist; dy += adaptiveStep) {
                // Only check points near the perimeter of this search distance
                float pointDist = std::sqrt(dx*dx + dy*dy);
                if (pointDist < searchDist - adaptiveStep || pointDist > searchDist + adaptiveStep) {
                    continue;
                }                float testX = x + dx;
                float testY = y + dy;
                
                // Check if this position is safe
                if (!wouldCollideWithElement(testX, testY, playerRadius)) {
                    // Found a safe position, update coordinates and return
                    if (playerDebugMode) {
                        std::cout << "Grid search found safe position at (" << testX << ", " << testY 
                                  << "), moved character " << pointDist << " units" << std::endl;
                    }
                    
                    x = testX;
                    y = testY;
                    return true; // Position was adjusted
                }
            }
        }
    }
    
    // Final attempt - try random positions across the map within the grid bounds
    if (playerDebugMode) {
        std::cout << "Grid search failed, attempting random position search..." << std::endl;
    }
      // Try a limited number of random positions to avoid infinite loop
    const int MAX_RANDOM_ATTEMPTS = 100;
    for (int attempt = 0; attempt < MAX_RANDOM_ATTEMPTS; attempt++) {
        // Generate random coordinates within the grid
        float testX = 1.0f + static_cast<float>(rand() % (GRID_SIZE - 2));
        float testY = 1.0f + static_cast<float>(rand() % (GRID_SIZE - 2));
          // Check if this position is safe (only check element collisions for entities now)
        if (!wouldCollideWithElement(testX, testY, playerRadius)) {
            // Found a safe position, update coordinates and return
            if (playerDebugMode) {
                std::cout << "Random search found safe position at (" << testX << ", " << testY 
                          << "), teleported character" << std::endl;
            }
            
            x = testX;
            y = testY;
            return true; // Position was adjusted
        }
    }
    
    // If we got here, couldn't find a safe position within reasonable distance
    if (playerDebugMode) {
        std::cout << "Warning: Could not find safe position for player within " 
                  << maxDistance << " units!" << std::endl;
    }
    
    return false; // Couldn't adjust position
}



// Helper function to print information about a position's collision status
void debugCollisionStatus(float x, float y, float collisionRadius) {
    // Check if there's a collision
    bool hasElementCollision = wouldCollideWithElement(x, y, collisionRadius);
    
    if (hasElementCollision) {
        // Get nearby collidable elements
        std::vector<std::string> nearbyElements = getNearbyElements(x, y, collisionRadius + 1.0f);
        
        std::cout << "Collision detected at (" << x << ", " << y << ") with radius " << collisionRadius << std::endl;
        std::cout << "Nearby elements: ";
        
        for (const auto& elementName : nearbyElements) {
            float elementX, elementY;
            if (elementsManager.getElementPosition(elementName, elementX, elementY)) {
                float distance = std::sqrt(std::pow(x - elementX, 2) + std::pow(y - elementY, 2));
                std::cout << elementName << " (distance: " << distance << "), ";
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "No collision at (" << x << ", " << y << ") with radius " << collisionRadius << std::endl;
    }
}

// Function to check if an entity (using collision shape points) would collide with any element
bool wouldEntityCollideWithElement(float x, float y, const std::vector<std::pair<float, float>>& entityCollisionShapePoints, float entityScale, float entityRotation) {
    // Make sure the spatial grid is up to date
    if (!spatialGridInitialized) {
        updateSpatialGrid();
    }
    
    // Calculate approximate radius for nearby element search
    float maxRadius = 0.0f;
    for (const auto& point : entityCollisionShapePoints) {
        float dist = std::sqrt(point.first * point.first + point.second * point.second);
        maxRadius = std::max(maxRadius, dist * entityScale);
    }
    
    // Get only nearby elements instead of checking all elements
    std::vector<std::string> nearbyElements = getNearbyElements(x, y, maxRadius + MAX_COLLISION_CHECK_RANGE);
    
    // Transform entity polygon points to world coordinates
    std::vector<std::pair<float, float>> entityWorldShapePoints;
    float angleRad = entityRotation * M_PI / 180.0f; // Convert rotation to radians
    float cosA = cos(angleRad);
    float sinA = sin(angleRad);

    for (const auto& localPoint : entityCollisionShapePoints) {
        // Scale first, then rotate, then translate
        float scaledX = localPoint.first * entityScale;
        float scaledY = localPoint.second * entityScale;
        
        float rotatedX = scaledX * cosA - scaledY * sinA;
        float rotatedY = scaledX * sinA + scaledY * cosA;
        
        entityWorldShapePoints.push_back({x + rotatedX, y + rotatedY});
    }
    
    // Check collision with each nearby element
    for (const auto& elementName : nearbyElements) {
        float elementX, elementY;
        float elementScale = 1.0f;
        float elementRotation = 0.0f;
        std::vector<std::pair<float, float>> elementCollisionShapePoints;
        bool elementHasCollision = false;

        // Find the element to get its actual collision properties
        const auto& elements = elementsManager.getElements();
        const PlacedElement* currentElement = nullptr;
        for (const auto& el : elements) {
            if (el.instanceName == elementName) {
                currentElement = &el;
                break;
            }
        }

        if (!currentElement || !currentElement->hasCollision || currentElement->collisionShapePoints.empty()) {
            continue; // Skip elements without collision enabled or without defined shape
        }

        elementX = currentElement->x;
        elementY = currentElement->y;
        elementScale = currentElement->scale;
        elementRotation = currentElement->rotation;
        elementCollisionShapePoints = currentElement->collisionShapePoints;

        // Transform element polygon points to world coordinates
        std::vector<std::pair<float, float>> elementWorldShapePoints;
        float elementAngleRad = elementRotation * M_PI / 180.0f;
        float elementCosA = cos(elementAngleRad);
        float elementSinA = sin(elementAngleRad);

        for (const auto& localPoint : elementCollisionShapePoints) {
            float scaledX = localPoint.first * elementScale;
            float scaledY = localPoint.second * elementScale;
            
            float rotatedX = scaledX * elementCosA - scaledY * elementSinA;
            float rotatedY = scaledX * elementSinA + scaledY * elementCosA;
            
            elementWorldShapePoints.push_back({elementX + rotatedX, elementY + rotatedY});
        }

        // Perform polygon-polygon collision detection using Separating Axis Theorem (SAT)
        if (polygonPolygonCollision(entityWorldShapePoints, elementWorldShapePoints)) {
            float currentTime = static_cast<float>(glfwGetTime());
            if (playerDebugMode && currentTime - lastCollisionDebugTime > 2.0f) {
                lastCollisionDebugTime = currentTime;
                std::cout << "Entity polygon collision with " << elementName 
                          << " at entity pos (" << x << ", " << y << ")" 
                          << ", element center: (" << elementX << ", " << elementY << ")" << std::endl;
            }
            return true; // Collision detected
        }
    }
    
    return false; // No collision detected
}

// Helper function to check collision between two polygons using Separating Axis Theorem (SAT)
bool polygonPolygonCollision(const std::vector<std::pair<float, float>>& poly1, const std::vector<std::pair<float, float>>& poly2) {
    // Check if either polygon is empty
    if (poly1.empty() || poly2.empty()) {
        return false;
    }
    
    // Function to get perpendicular axis from an edge
    auto getAxis = [](const std::pair<float, float>& p1, const std::pair<float, float>& p2) -> std::pair<float, float> {
        float edgeX = p2.first - p1.first;
        float edgeY = p2.second - p1.second;
        // Perpendicular to edge (normal)
        return {-edgeY, edgeX};
    };
    
    // Function to normalize a vector
    auto normalize = [](std::pair<float, float>& axis) {
        float length = std::sqrt(axis.first * axis.first + axis.second * axis.second);
        if (length > 0.0f) {
            axis.first /= length;
            axis.second /= length;
        }
    };
    
    // Function to project polygon onto axis
    auto projectPolygon = [](const std::vector<std::pair<float, float>>& polygon, const std::pair<float, float>& axis) -> std::pair<float, float> {
        float min = polygon[0].first * axis.first + polygon[0].second * axis.second;
        float max = min;
        
        for (size_t i = 1; i < polygon.size(); ++i) {
            float projection = polygon[i].first * axis.first + polygon[i].second * axis.second;
            if (projection < min) min = projection;
            if (projection > max) max = projection;
        }
        
        return {min, max};
    };
    
    // Function to check if two projections overlap
    auto projectionsOverlap = [](const std::pair<float, float>& proj1, const std::pair<float, float>& proj2) -> bool {
        return !(proj1.second < proj2.first || proj2.second < proj1.first);
    };
    
    // Check all axes from poly1
    for (size_t i = 0; i < poly1.size(); ++i) {
        std::pair<float, float> axis = getAxis(poly1[i], poly1[(i + 1) % poly1.size()]);
        normalize(axis);
        
        std::pair<float, float> proj1 = projectPolygon(poly1, axis);
        std::pair<float, float> proj2 = projectPolygon(poly2, axis);
        
        if (!projectionsOverlap(proj1, proj2)) {
            return false; // Separating axis found, no collision
        }
    }
    
    // Check all axes from poly2
    for (size_t i = 0; i < poly2.size(); ++i) {
        std::pair<float, float> axis = getAxis(poly2[i], poly2[(i + 1) % poly2.size()]);
        normalize(axis);
        
        std::pair<float, float> proj1 = projectPolygon(poly1, axis);
        std::pair<float, float> proj2 = projectPolygon(poly2, axis);
        
        if (!projectionsOverlap(proj1, proj2)) {
            return false; // Separating axis found, no collision
        }
    }
    
    return true; // No separating axis found, collision detected
}
