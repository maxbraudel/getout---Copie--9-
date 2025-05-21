#include "collision.h"
#include "elementsOnMap.h"
#include "globals.h"  // Added for GRID_SIZE
#include "GLFW/glfw3.h" // Added for glfwGetTime
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>  // Added for spatial partitioning

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
static bool spatialGridInitialized = false;
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
        if (elementsManager.getElementPosition(elementName, elementX, elementY)) {
            // Count how many collision checks we're performing
            collisionCheckCount++;
            
            // Quick distance check
            float dx = x - elementX;
            float dy = y - elementY;
            float distanceSquared = dx*dx + dy*dy;
            
            // Skip elements that are clearly too far away (optimization)
            float maxCheckRange = MAX_COLLISION_CHECK_RANGE + playerRadius;
            if (distanceSquared > maxCheckRange * maxCheckRange) {
                continue;
            }
            
            // Get the collision radius for this specific element
            float elementRadius = 0.4f; // Default value
            
            // Find the element to get its actual collision radius
            const auto& elements = elementsManager.getElements();
            for (const auto& element : elements) {
                if (element.instanceName == elementName) {
                    elementRadius = element.collisionRadius;
                    break;
                }
            }
            
            // Combined radius for collision detection
            float combinedRadius = playerRadius + elementRadius;
            
            // If distance is less than combined radius, we have a collision
            if (distanceSquared < combinedRadius * combinedRadius) {
                // Count how many actual collisions we're detecting
                collisionHitCount++;
                
                // Only print collision debug info when playerDebugMode is on and throttle the messages
                float currentTime = static_cast<float>(glfwGetTime());
                if (playerDebugMode && currentTime - lastCollisionDebugTime > 2.0f) {
                    lastCollisionDebugTime = currentTime;
                    std::cout << "Collision with " << elementName 
                              << " at distance " << std::sqrt(distanceSquared)
                              << " (combined radius: " << combinedRadius << ")" << std::endl;
                    
                    // Print performance stats occasionally
                    std::cout << "Collision efficiency: " << collisionHitCount << " hits from " 
                              << collisionCheckCount << " checks ("
                              << (collisionCheckCount > 0 ? (100.0f * collisionHitCount / collisionCheckCount) : 0)
                              << "% hit rate)" << std::endl;
                    
                    // Reset counters periodically
                    if (collisionCheckCount > 1000) {
                        collisionCheckCount = 0;
                        collisionHitCount = 0;
                    }
                }
                return true; // Collision detected
            }
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

// Function to find a safe position when player is stuck inside a collision area
bool findSafePosition(float& x, float& y, float playerRadius, const Map& gameMap) {
    // First, check if the current position is already safe
    bool hasElementCollision = wouldCollideWithElement(x, y, playerRadius);
    bool hasMapBlockCollision = wouldCollideWithMapBlock(x, y, gameMap);
    
    if (!hasElementCollision && !hasMapBlockCollision) {
        return false; // No adjustment needed, position is already safe
    }
    
    if (playerDebugMode) {
        std::cout << "Player found stuck in collision at (" << x << ", " << y 
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
        
        for (float angle = 0.0f; angle < 2.0f * M_PI; angle += angleStep) {
            float testX = x + distance * cos(angle);
            float testY = y + distance * sin(angle);
            
            // Check if this position is safe
            if (!wouldCollideWithElement(testX, testY, playerRadius) && 
                !wouldCollideWithMapBlock(testX, testY, gameMap)) {
                // Found a safe position, update coordinates and return
                if (playerDebugMode) {
                    std::cout << "Safe position found at (" << testX << ", " << testY 
                              << "), moved player " << distance << " units" << std::endl;
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
                }
                
                float testX = x + dx;
                float testY = y + dy;
                
                // Check if this position is safe
                if (!wouldCollideWithElement(testX, testY, playerRadius) && 
                    !wouldCollideWithMapBlock(testX, testY, gameMap)) {
                    // Found a safe position, update coordinates and return
                    if (playerDebugMode) {
                        std::cout << "Grid search found safe position at (" << testX << ", " << testY 
                                  << "), moved player " << pointDist << " units" << std::endl;
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
        
        // Check if this position is safe
        if (!wouldCollideWithElement(testX, testY, playerRadius) && 
            !wouldCollideWithMapBlock(testX, testY, gameMap)) {
            // Found a safe position, update coordinates and return
            if (playerDebugMode) {
                std::cout << "Random search found safe position at (" << testX << ", " << testY 
                          << "), teleported player" << std::endl;
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
