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
    // COLLISION RESOLUTION MECHANISMS DISABLED
    // Entities will no longer be automatically moved to "safe positions"
    return false; // Always return false - no automatic position adjustment
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

// Helper function to check if an entity's collision shape would go beyond map boundaries
bool wouldEntityCollideWithMapBounds(float x, float y, const std::vector<std::pair<float, float>>& collisionShapePoints, float entityScale, float entityRotation) {
    if (collisionShapePoints.empty()) {
        // If no collision shape is defined, check only the center point
        return (x < 0.0f || y < 0.0f || x >= static_cast<float>(GRID_SIZE) || y >= static_cast<float>(GRID_SIZE));
    }
    
    // Transform collision shape points to world coordinates
    std::vector<std::pair<float, float>> worldShapePoints;
    float angleRad = entityRotation * M_PI / 180.0f;
    float cosA = cos(angleRad);
    float sinA = sin(angleRad);
    
    for (const auto& localPoint : collisionShapePoints) {
        // Scale first, then rotate, then translate
        float scaledX = localPoint.first * entityScale;
        float scaledY = localPoint.second * entityScale;
        
        float rotatedX = scaledX * cosA - scaledY * sinA;
        float rotatedY = scaledX * sinA + scaledY * cosA;
        
        float worldX = x + rotatedX;
        float worldY = y + rotatedY;
        
        // Check if any point of the collision shape is outside map bounds
        if (worldX < 0.0f || worldY < 0.0f || worldX >= static_cast<float>(GRID_SIZE) || worldY >= static_cast<float>(GRID_SIZE)) {
            return true; // Collision shape extends beyond map boundaries
        }
    }
    
    return false; // All points of collision shape are within map bounds
}

// Overloaded function that uses EntityConfiguration
bool wouldEntityCollideWithMapBounds(const EntityConfiguration& config, float x, float y) {
    return wouldEntityCollideWithMapBounds(x, y, config.collisionShapePoints, 1.0f, 0.0f);
}
