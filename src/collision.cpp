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
#include <unordered_set>  // Added for hierarchical grid
#include <chrono>         // Added for performance monitoring
#include <iomanip>        // Added for performance stats formatting
#include <limits> // Added for std::numeric_limits
#include "enumDefinitions.h"


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

// Safety distance for collision resolution - ensures entities aren't teleported too close to collision areas
const float SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION = 1.0f;

// Last time debug messages were printed to reduce spam
static float lastCollisionDebugTime = 0.0f;

// Define the set of non-traversable blocks
std::set<BlockName> nonTraversableBlocks = {
    BlockName::WATER_1,
    BlockName::WATER_2,
    BlockName::WATER_3,
    BlockName::WATER_4
    // Add more block types here as needed
};

// Function to add a block type to the non-traversable set
void addNonTraversableBlock(BlockName blockType) {
    nonTraversableBlocks.insert(blockType);
    std::cout << "Added block type " << static_cast<int>(blockType) << " to non-traversable blocks." << std::endl;
}

// Function to remove a block type from the non-traversable set
void removeNonTraversableBlock(BlockName blockType) {
    auto it = nonTraversableBlocks.find(blockType);
    if (it != nonTraversableBlocks.end()) {
        nonTraversableBlocks.erase(it);
        std::cout << "Removed block type " << static_cast<int>(blockType) << " from non-traversable blocks." << std::endl;
    }
}

// Function to check if a block type is non-traversable
bool isBlockNonTraversable(BlockName blockType) {
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
    
    // CRASH FIX: Validate elementsManager before accessing
    try {
        // Only update cache periodically to improve performance
        float currentTime = static_cast<float>(glfwGetTime());
        if (!collisionCacheInitialized || currentTime - lastCacheUpdateTime > 2.0f) {
            // Update the cache when it's stale or needs initializing
            collidableElementNames.clear();
            lastCacheUpdateTime = currentTime;
            
            // Get all elements and filter for those with collision enabled
            const auto& elements = elementsManager.getElements();
            
            // CRASH FIX: Reserve memory to prevent reallocations
            collidableElementNames.reserve(elements.size());
            
            for (const auto& element : elements) {
                // Check if the element has collision enabled
                if (element.hasCollision) {
                    collidableElementNames.push_back(element.instanceName);
                }
            }
            
            collisionCacheInitialized = true;
        }
    } catch (const std::exception& e) {
        std::cerr << "CRASH FIX: Exception in getCollidableElementNames: " << e.what() << std::endl;
        // Return safe empty vector on error
        return std::vector<std::string>();
    }
    
    return collidableElementNames;
}

// Define a constant maximum collision range to optimize element checks
const float MAX_COLLISION_CHECK_RANGE = 3.0f; // Only check elements within this range

// Function to check if a position would collide with a collidable element
bool wouldCollideWithElement(float x, float y, float playerRadius) {
    // Use hierarchical collision detection by default for better performance
    return wouldCollideWithElementHierarchical(x, y, playerRadius);
}

// Reset the elements cache when new elements are added
void resetCollisionCache() {
    collisionCacheInitialized = false;
    spatialGridInitialized = false;
    g_hierarchicalGrid.clear();  // Also reset hierarchical grid
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
    BlockName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
    
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
bool wouldCollideWithMapBlock(float x, float y, const Map& gameMap, const std::set<BlockName>& entityNonTraversableBlocks) {
    // Convert floating point coordinates to grid integers
    int gridX = static_cast<int>(x);
    int gridY = static_cast<int>(y);
    
    // Skip invalid coordinates 
    if (gridX < 0 || gridX >= GRID_SIZE || gridY < 0 || gridY >= GRID_SIZE) {
        return true; // Treat out-of-bounds as collision
    }
    
    // Get the texture type at these coordinates
    BlockName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
    
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

// Helper function to check if a position is safe with safety distance buffer
bool isPositionSafeWithBuffer(float x, float y, float playerRadius, const Map& gameMap, float safetyBuffer = SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION) {
    // Check the center position first
    if (wouldCollideWithElement(x, y, playerRadius) || wouldCollideWithMapBlock(x, y, gameMap)) {
        return false;
    }
    
    // Now check positions around the center with the safety buffer to ensure adequate distance from collision areas
    const int bufferCheckDirections = 8; // Check 8 directions around the position
    for (int i = 0; i < bufferCheckDirections; i++) {
        float angle = (i * 2.0f * M_PI) / bufferCheckDirections;
        float bufferX = x + safetyBuffer * cos(angle);
        float bufferY = y + safetyBuffer * sin(angle);
        
        // If any of the buffer positions would collide, this isn't a safe position
        if (wouldCollideWithElement(bufferX, bufferY, playerRadius) || wouldCollideWithMapBlock(bufferX, bufferY, gameMap)) {
            return false;
        }
    }
    
    return true; // Position is safe with adequate buffer distance
}

// Helper function to check if a position is safe with safety distance buffer for entities
bool isEntityPositionSafeWithBuffer(float x, float y, const EntityConfiguration& config, const Map& gameMap, float safetyBuffer = SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION, const std::string& excludeInstanceName = "") {
    // Check the center position first
    if (wouldEntityCollideWithElementsGranular(config, x, y, false) || 
        wouldEntityCollideWithBlocksGranular(config, x, y, false) ||
        wouldEntityCollideWithEntitiesGranular(config, x, y, false, excludeInstanceName)) {
        return false;
    }
    
    // Now check positions around the center with the safety buffer
    const int bufferCheckDirections = 8; // Check 8 directions around the position
    for (int i = 0; i < bufferCheckDirections; i++) {
        float angle = (i * 2.0f * M_PI) / bufferCheckDirections;
        float bufferX = x + safetyBuffer * cos(angle);
        float bufferY = y + safetyBuffer * sin(angle);
        
        // If any of the buffer positions would collide, this isn't a safe position
        if (wouldEntityCollideWithElementsGranular(config, bufferX, bufferY, false) || 
            wouldEntityCollideWithBlocksGranular(config, bufferX, bufferY, false) ||
            wouldEntityCollideWithEntitiesGranular(config, bufferX, bufferY, false, excludeInstanceName)) {
            return false;
        }
    }
    
    return true; // Position is safe with adequate buffer distance
}

// Function to find a safe position when a character is stuck inside a collision area
bool findSafePosition(float& x, float& y, float playerRadius, const Map& gameMap) {
    // First check if current position is already safe with safety buffer
    if (isPositionSafeWithBuffer(x, y, playerRadius, gameMap)) {
        return true; // Already in a safe position with adequate buffer
    }
    
    std::cout << "Entity stuck at (" << x << ", " << y << ") - finding safe position with " 
              << SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION << " unit safety buffer..." << std::endl;
    
    // Search in expanding concentric circles for a safe position
    const float searchStep = 1.0f; // Slightly larger step size for better performance
    const float maxSearchRadius = 5.0f; // Much larger search radius for multi-layer scenarios
    
    std::cout << "Searching for safe position with radius up to " << maxSearchRadius << " units..." << std::endl;
    
    for (float radius = searchStep; radius <= maxSearchRadius; radius += searchStep) {
        // Progress indicator for long searches
        if (static_cast<int>(radius) % 5 == 0 && radius > 5.0f) {
            std::cout << "Still searching... radius " << radius << "/" << maxSearchRadius << std::endl;
        }
        
        // Try 24 directions around the current position (more directions for better coverage)
        for (int i = 0; i < 24; i++) {
            float angle = (i * 2.0f * M_PI) / 24.0f;
            float testX = x + radius * cos(angle);
            float testY = y + radius * sin(angle);
            
            // Make sure we stay within map bounds (with safety buffer margin)
            float margin = SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION + 0.5f;
            if (testX < margin || testX >= (GRID_SIZE - margin) || 
                testY < margin || testY >= (GRID_SIZE - margin)) {
                // Debug: Log boundary rejections for initial attempts
                if (radius <= 2.0f) {
                    std::cout << "Position (" << testX << ", " << testY << ") rejected - outside map bounds (margin: " << margin << ")" << std::endl;
                }
                continue;
            }
            
            // Check if this position is safe with safety buffer
            if (isPositionSafeWithBuffer(testX, testY, playerRadius, gameMap)) {
                // Found a safe position with adequate buffer!
                std::cout << "Found safe position at (" << testX << ", " << testY 
                          << ") - distance: " << radius << " with safety buffer: " 
                          << SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION << std::endl;
                x = testX;
                y = testY;
                return true;
            }
            
            // Debug: Log why this position failed (but only for first few attempts to avoid spam)
            if (radius <= 2.0f) {
                std::cout << "Position (" << testX << ", " << testY << ") rejected - insufficient safety buffer" << std::endl;
            }
        }
    }
    
    std::cout << "Could not find safe position within search radius of " << maxSearchRadius << std::endl;
    return false; // Could not find a safe position
}

// Enhanced function to find a safe position for entities using their collision shape
bool findSafePositionForEntity(float& x, float& y, const EntityConfiguration& config, const Map& gameMap, const std::string& excludeInstanceName) {
    // Store original position for comparison
    float originalX = x;
    float originalY = y;
    
    // DO NOT immediately return if current position seems safe - this causes infinite loops
    // When this function is called, it means the entity is definitively stuck, so we need to find a DIFFERENT position
    
    std::cout << "Entity with collision shape stuck at (" << x << ", " << y << ") - finding safe position with " 
              << SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION << " unit safety buffer..." << std::endl;
    
    // Calculate entity's approximate radius for search optimization
    float entityRadius = 0.5f; // Default fallback
    if (!config.collisionShapePoints.empty()) {
        for (const auto& point : config.collisionShapePoints) {
            float dist = std::sqrt(point.first * point.first + point.second * point.second);
            entityRadius = std::max(entityRadius, dist);
        }
    }
    
    // Search in expanding concentric circles for a safe position
    const float searchStep = 0.2f; // Slightly larger step size for better performance
    const float maxSearchRadius = 5.0f; // Much larger search radius to match player search radius
      std::cout << "Searching for safe entity position with radius up to " << maxSearchRadius << " units..." << std::endl;
    
    for (float radius = searchStep; radius <= maxSearchRadius; radius += searchStep) {
        // Progress indicator for long searches
        if (static_cast<int>(radius) % 5 == 0 && radius > 5.0f) {
            std::cout << "Still searching... radius " << radius << "/" << maxSearchRadius << std::endl;
        }
        
        // Try 32 directions around the current position (more directions for better coverage)
        for (int i = 0; i < 32; i++) {
            float angle = (i * 2.0f * M_PI) / 32.0f;
            float testX = x + radius * cos(angle);
            float testY = y + radius * sin(angle);            // Make sure we stay within map bounds with safety buffer margin
            float margin = SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION + 0.5f;
            if (testX < margin || testX >= (GRID_SIZE - margin) || 
                testY < margin || testY >= (GRID_SIZE - margin)) {
                // Debug: Log boundary rejections for initial attempts
                if (radius <= 2.0f) {
                    std::cout << "Position (" << testX << ", " << testY << ") rejected - outside map bounds (margin: " << margin << ")" << std::endl;
                }
                continue;
            }            // Check if this position is safe with safety buffer
            if (isEntityPositionSafeWithBuffer(testX, testY, config, gameMap, SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION, excludeInstanceName)) {
                // Found a safe position with adequate buffer!
                std::cout << "Found safe position at (" << testX << ", " << testY 
                          << ") - distance: " << radius << " with safety buffer: " 
                          << SAFETY_DISTANCE_FROM_COLLISION_AREA_AFTER_RESOLUTION << std::endl;
                x = testX;
                y = testY;
                return true;
            }
            
            // Debug: Log why this position failed (but only for first few attempts to avoid spam)
            if (radius <= 2.0f) {
                std::cout << "Position (" << testX << ", " << testY << ") rejected - insufficient safety buffer" << std::endl;
            }
        }
    }
    
    std::cout << "Could not find safe position within search radius of " << maxSearchRadius << std::endl;
    return false; // Could not find a safe position
}

// Function to resolve collision when an entity is stuck (to be called from entities system)
bool resolveEntityCollisionStuck(const std::string& entityId, float& x, float& y, const EntityConfiguration& config, const Map& gameMap) {
    std::cout << "Collision resolution requested for entity: " << entityId << " at position (" << x << ", " << y << ")" << std::endl;
      // Use the enhanced entity collision resolution function with entity exclusion
    bool success = findSafePositionForEntity(x, y, config, gameMap, entityId);
    
    if (success) {
        std::cout << "Successfully resolved collision for entity " << entityId << " - moved to (" << x << ", " << y << ")" << std::endl;
    } else {
        std::cout << "Failed to resolve collision for entity " << entityId << " - no safe position found within search radius" << std::endl;
    }
    
    return success;
}


// Function to check if an entity (using collision shape points) would collide with any element
bool wouldEntityCollideWithElement(float x, float y, const std::vector<std::pair<float, float>>& entityCollisionShapePoints, float entityScale, float entityRotation) {
    // Use hierarchical collision detection by default for better performance
    return wouldEntityCollideWithElementHierarchical(x, y, entityCollisionShapePoints, entityScale, entityRotation);
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
        // CRASH FIX: Additional safety check for empty polygon
        if (polygon.empty()) {
            std::cerr << "CRITICAL: Attempting to project empty polygon in SAT collision detection" << std::endl;
            return {0.0f, 0.0f};
        }
        
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

// ===== ENHANCED HIERARCHICAL SPATIAL PARTITIONING IMPLEMENTATION =====

// Global instance of hierarchical spatial grid
HierarchicalSpatialGrid g_hierarchicalGrid;

// Performance monitoring for collision system
CollisionPerformanceStats g_collisionStats;

void CollisionPerformanceStats::reset() {
    broadPhaseChecks.store(0);
    narrowPhaseChecks.store(0);
    hierarchicalHits.store(0);
    totalCollisionQueries.store(0);
    totalTimeMs.store(0.0);
}

void CollisionPerformanceStats::printStats() const {
    if (totalCollisionQueries > 0) {
        double avgTimePerQuery = totalTimeMs.load() / totalCollisionQueries.load();
        double hitRatio = static_cast<double>(hierarchicalHits.load()) / totalCollisionQueries.load() * 100.0;
        
        std::cout << "=== Collision Performance Stats ===" << std::endl;
        std::cout << "Total Queries: " << totalCollisionQueries.load() << std::endl;
        std::cout << "Broad Phase Checks: " << broadPhaseChecks.load() << std::endl;
        std::cout << "Narrow Phase Checks: " << narrowPhaseChecks.load() << std::endl;
        std::cout << "Hierarchical Hits: " << hierarchicalHits.load() << " (" << std::fixed << std::setprecision(1) << hitRatio << "%)" << std::endl;
        std::cout << "Avg Time Per Query: " << std::fixed << std::setprecision(3) << avgTimePerQuery << "ms" << std::endl;
    }
}

// HierarchicalSpatialGrid Implementation
void HierarchicalSpatialGrid::initialize() {
    if (isInitialized) return;
    
    clear();
    
    // Categorize elements as static or dynamic based on their properties
    const auto& elements = elementsManager.getElements();
    for (const auto& element : elements) {
        if (element.hasCollision) {
            // Simple heuristic: elements with certain names or properties are considered dynamic
            // You can enhance this based on your game's specific needs
            bool isDynamic = (element.instanceName.find("player") != std::string::npos ||
                            element.instanceName.find("enemy") != std::string::npos ||
                            element.instanceName.find("npc") != std::string::npos ||
                            element.instanceName.find("movable") != std::string::npos);
            
            if (isDynamic) {
                dynamicElementNames.insert(element.instanceName);
            } else {
                staticElementNames.insert(element.instanceName);
            }
        }
    }
    
    updateGrid(true); // Force initial update
    isInitialized = true;
    
    if (DEBUG_LOGS) {
        std::cout << "HierarchicalSpatialGrid initialized with " 
                  << staticElementNames.size() << " static and " 
                  << dynamicElementNames.size() << " dynamic elements" << std::endl;
    }
}

void HierarchicalSpatialGrid::updateGrid(bool forceUpdate) {    float currentTime = static_cast<float>(glfwGetTime());
    
    bool shouldUpdateStatic = forceUpdate || (currentTime - lastCoarseUpdateTime > HierarchicalSpatialGrid::STATIC_UPDATE_INTERVAL);
    bool shouldUpdateDynamic = forceUpdate || (currentTime - lastFineUpdateTime > HierarchicalSpatialGrid::DYNAMIC_UPDATE_INTERVAL);
    
    if (shouldUpdateStatic) {
        updateStaticElements();
        lastCoarseUpdateTime = currentTime;
    }
    
    if (shouldUpdateDynamic) {
        updateDynamicElements();
        lastFineUpdateTime = currentTime;
    }
}

void HierarchicalSpatialGrid::updateStaticElements() {
    // Clear static elements from both grids
    for (auto& [index, cell] : coarseGrid) {
        cell.staticElements.clear();
    }
    for (auto& [index, cell] : fineGrid) {
        cell.staticElements.clear();
    }
    
    // Re-add static elements to both grids
    for (const auto& elementName : staticElementNames) {
        float x, y;
        if (elementsManager.getElementPosition(elementName, x, y)) {
            addElementToGrid(elementName, x, y, true);
        }
    }
}

void HierarchicalSpatialGrid::updateDynamicElements() {
    // Clear dynamic elements from both grids
    for (auto& [index, cell] : coarseGrid) {
        cell.dynamicElements.clear();
    }
    for (auto& [index, cell] : fineGrid) {
        cell.dynamicElements.clear();
    }
    
    // Re-add dynamic elements to both grids
    for (const auto& elementName : dynamicElementNames) {
        float x, y;
        if (elementsManager.getElementPosition(elementName, x, y)) {
            addElementToGrid(elementName, x, y, false);
        }
    }
}

void HierarchicalSpatialGrid::markElementAsDynamic(const std::string& elementName) {
    staticElementNames.erase(elementName);
    dynamicElementNames.insert(elementName);
}

void HierarchicalSpatialGrid::markElementAsStatic(const std::string& elementName) {
    dynamicElementNames.erase(elementName);
    staticElementNames.insert(elementName);
}

std::vector<std::string> HierarchicalSpatialGrid::getBroadPhaseElements(float x, float y, float radius) {
    std::vector<std::string> result;
    g_collisionStats.broadPhaseChecks++;
    
    // Use coarse grid for broad phase
    int cellRadius = static_cast<int>(radius / COARSE_GRID_SIZE) + 1;
    int centerCellX = static_cast<int>(x) / COARSE_GRID_SIZE;
    int centerCellY = static_cast<int>(y) / COARSE_GRID_SIZE;
    
    for (int cellX = centerCellX - cellRadius; cellX <= centerCellX + cellRadius; cellX++) {
        for (int cellY = centerCellY - cellRadius; cellY <= centerCellY + cellRadius; cellY++) {
            int index = getCoarseGridIndex(cellX * COARSE_GRID_SIZE, cellY * COARSE_GRID_SIZE);
            
            auto it = coarseGrid.find(index);
            if (it != coarseGrid.end()) {
                const auto& cell = it->second;
                result.insert(result.end(), cell.staticElements.begin(), cell.staticElements.end());
                result.insert(result.end(), cell.dynamicElements.begin(), cell.dynamicElements.end());
            }
        }
    }
    
    return result;
}

std::vector<std::string> HierarchicalSpatialGrid::getNarrowPhaseElements(float x, float y, float radius) {
    std::vector<std::string> result;
    g_collisionStats.narrowPhaseChecks++;
    
    // Use fine grid for narrow phase
    int cellRadius = static_cast<int>(radius / FINE_GRID_SIZE) + 1;
    int centerCellX = static_cast<int>(x) / FINE_GRID_SIZE;
    int centerCellY = static_cast<int>(y) / FINE_GRID_SIZE;
    
    for (int cellX = centerCellX - cellRadius; cellX <= centerCellX + cellRadius; cellX++) {
        for (int cellY = centerCellY - cellRadius; cellY <= centerCellY + cellRadius; cellY++) {
            int index = getFineGridIndex(cellX * FINE_GRID_SIZE, cellY * FINE_GRID_SIZE);
            
            auto it = fineGrid.find(index);
            if (it != fineGrid.end()) {
                const auto& cell = it->second;
                result.insert(result.end(), cell.staticElements.begin(), cell.staticElements.end());
                result.insert(result.end(), cell.dynamicElements.begin(), cell.dynamicElements.end());
            }
        }
    }
    
    return result;
}

std::vector<std::string> HierarchicalSpatialGrid::getElementsHierarchical(float x, float y, float radius) {
    auto startTime = std::chrono::high_resolution_clock::now();
    g_collisionStats.totalCollisionQueries++;
    
    std::vector<std::string> result;
    
    // Use broad phase for large radius queries
    if (radius > COARSE_GRID_SIZE * 0.5f) {
        result = getBroadPhaseElements(x, y, radius);
    } else {
        // Use narrow phase for precise queries
        result = getNarrowPhaseElements(x, y, radius);
    }
    
    if (!result.empty()) {
        g_collisionStats.hierarchicalHits++;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    g_collisionStats.totalTimeMs.store(g_collisionStats.totalTimeMs.load() + duration.count() / 1000.0);
    
    return result;
}

void HierarchicalSpatialGrid::clear() {
    coarseGrid.clear();
    fineGrid.clear();
    staticElementNames.clear();
    dynamicElementNames.clear();
    isInitialized = false;
    lastCoarseUpdateTime = 0.0f;
    lastFineUpdateTime = 0.0f;
}

bool HierarchicalSpatialGrid::isEmpty() const {
    return staticElementNames.empty() && dynamicElementNames.empty();
}

bool HierarchicalSpatialGrid::isInitializedState() const {
    return isInitialized;
}

size_t HierarchicalSpatialGrid::getStaticElementCount() const {
    return staticElementNames.size();
}

size_t HierarchicalSpatialGrid::getDynamicElementCount() const {
    return dynamicElementNames.size();
}

int HierarchicalSpatialGrid::getCoarseGridIndex(float x, float y) const {
    int gridX = static_cast<int>(x) / COARSE_GRID_SIZE;
    int gridY = static_cast<int>(y) / COARSE_GRID_SIZE;
    return gridX * 10000 + gridY; // Use larger multiplier for coarse grid
}

int HierarchicalSpatialGrid::getFineGridIndex(float x, float y) const {
    int gridX = static_cast<int>(x) / FINE_GRID_SIZE;
    int gridY = static_cast<int>(y) / FINE_GRID_SIZE;
    return gridX * 1000 + gridY; // Original multiplier for fine grid
}

void HierarchicalSpatialGrid::addElementToGrid(const std::string& elementName, float x, float y, bool isStatic) {
    // Add to coarse grid
    int coarseIndex = getCoarseGridIndex(x, y);
    if (isStatic) {
        coarseGrid[coarseIndex].staticElements.push_back(elementName);
    } else {
        coarseGrid[coarseIndex].dynamicElements.push_back(elementName);
    }
    
    // Add to fine grid
    int fineIndex = getFineGridIndex(x, y);
    if (isStatic) {
        fineGrid[fineIndex].staticElements.push_back(elementName);
    } else {
        fineGrid[fineIndex].dynamicElements.push_back(elementName);
    }
}

void HierarchicalSpatialGrid::removeElementFromGrid(const std::string& elementName) {
    // Remove from both grids - this is expensive so we avoid it in favor of clearing and rebuilding
    // In a more sophisticated implementation, we could track element positions to remove more efficiently
}

// Enhanced collision detection functions using hierarchical grid
bool wouldCollideWithElementHierarchical(float x, float y, float playerRadius) {
    // Initialize hierarchical grid if needed
    if (!g_hierarchicalGrid.isInitializedState()) {
        g_hierarchicalGrid.initialize();
    }
    
    // Update grid before collision check
    g_hierarchicalGrid.updateGrid();
    
    // Get nearby elements using hierarchical lookup
    std::vector<std::string> nearbyElements = g_hierarchicalGrid.getElementsHierarchical(x, y, playerRadius + MAX_COLLISION_CHECK_RANGE);
    
    // Perform collision detection on nearby elements
    for (const auto& elementName : nearbyElements) {
        float elementX, elementY;
        float elementScale = 1.0f;
        float elementRotation = 0.0f;
        std::vector<std::pair<float, float>> elementCollisionShapePoints;
        
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
            continue;
        }
        
        elementX = currentElement->x;
        elementY = currentElement->y;
        elementScale = currentElement->scale;
        elementRotation = currentElement->rotation;
        elementCollisionShapePoints = currentElement->collisionShapePoints;
        
        // Transform polygon points to world coordinates
        std::vector<std::pair<float, float>> worldShapePoints;
        float angleRad = elementRotation * M_PI / 180.0f;
        float cosA = cos(angleRad);
        float sinA = sin(angleRad);
        
        for (const auto& localPoint : elementCollisionShapePoints) {
            float scaledX = localPoint.first * elementScale;
            float scaledY = localPoint.second * elementScale;
            
            float rotatedX = scaledX * cosA - scaledY * sinA;
            float rotatedY = scaledX * sinA + scaledY * cosA;
            
            worldShapePoints.push_back({elementX + rotatedX, elementY + rotatedY});
        }
        
        // Perform circle-polygon collision detection
        bool collision = false;
        int intersectCount = 0;
        for (size_t i = 0; i < worldShapePoints.size(); ++i) {
            std::pair<float, float> p1 = worldShapePoints[i];
            std::pair<float, float> p2 = worldShapePoints[(i + 1) % worldShapePoints.size()];
            
            if (((p1.second > y) != (p2.second > y)) &&
                (x < (p2.first - p1.first) * (y - p1.second) / (p2.second - p1.second) + p1.first)) {
                intersectCount++;
            }
        }
        
        if (intersectCount % 2 == 1) {
            collision = true;
        } else {
            // Check distance to polygon edges
            for (size_t i = 0; i < worldShapePoints.size(); ++i) {
                std::pair<float, float> p1 = worldShapePoints[i];
                std::pair<float, float> p2 = worldShapePoints[(i + 1) % worldShapePoints.size()];
                
                float A = y - p1.second;
                float B = p1.first - x;
                float C = (x - p1.first) * (p2.second - p1.second) - (y - p1.second) * (p2.first - p1.first);
                
                float distance = std::abs(A * p2.first + B * p2.second + C) / std::sqrt(A * A + B * B);
                
                if (distance <= playerRadius) {
                    float dotProduct = (x - p1.first) * (p2.first - p1.first) + (y - p1.second) * (p2.second - p1.second);
                    float squaredLength = (p2.first - p1.first) * (p2.first - p1.first) + (p2.second - p1.second) * (p2.second - p1.second);
                    
                    if (dotProduct >= 0 && dotProduct <= squaredLength) {
                        collision = true;
                        break;
                    }
                }
            }
        }
        
        if (collision) {
            return true;
        }
    }
    
    return false;
}

bool wouldEntityCollideWithElementHierarchical(float x, float y, const std::vector<std::pair<float, float>>& entityCollisionShapePoints, float entityScale, float entityRotation) {
    // Initialize hierarchical grid if needed
    if (!g_hierarchicalGrid.isInitializedState()) {
        g_hierarchicalGrid.initialize();
    }
    
    // Update grid before collision check
    g_hierarchicalGrid.updateGrid();
    
    // Calculate approximate radius for nearby element search
    float maxRadius = 0.0f;
    for (const auto& point : entityCollisionShapePoints) {
        float dist = std::sqrt(point.first * point.first + point.second * point.second);
        maxRadius = std::max(maxRadius, dist * entityScale);
    }
    
    // Get nearby elements using hierarchical lookup
    std::vector<std::string> nearbyElements = g_hierarchicalGrid.getElementsHierarchical(x, y, maxRadius + MAX_COLLISION_CHECK_RANGE);
    
    // Transform entity polygon points to world coordinates
    std::vector<std::pair<float, float>> entityWorldShapePoints;
    float entityAngleRad = entityRotation * M_PI / 180.0f;
    float entityCosA = cos(entityAngleRad);
    float entitySinA = sin(entityAngleRad);
    
    for (const auto& localPoint : entityCollisionShapePoints) {
        float scaledX = localPoint.first * entityScale;
        float scaledY = localPoint.second * entityScale;
        
        float rotatedX = scaledX * entityCosA - scaledY * entitySinA;
        float rotatedY = scaledX * entitySinA + scaledY * entityCosA;
        
        entityWorldShapePoints.push_back({x + rotatedX, y + rotatedY});
    }
    
    // Check collision with each nearby element
    for (const auto& elementName : nearbyElements) {
        const auto& elements = elementsManager.getElements();
        const PlacedElement* currentElement = nullptr;
        for (const auto& el : elements) {
            if (el.instanceName == elementName) {
                currentElement = &el;
                break;
            }
        }
        
        if (!currentElement || !currentElement->hasCollision || currentElement->collisionShapePoints.empty()) {
            continue;
        }
        
        // Transform element polygon points to world coordinates
        std::vector<std::pair<float, float>> elementWorldShapePoints;
        float elementAngleRad = currentElement->rotation * M_PI / 180.0f;
        float elementCosA = cos(elementAngleRad);
        float elementSinA = sin(elementAngleRad);
        
        for (const auto& localPoint : currentElement->collisionShapePoints) {
            float scaledX = localPoint.first * currentElement->scale;
            float scaledY = localPoint.second * currentElement->scale;
            
            float rotatedX = scaledX * elementCosA - scaledY * elementSinA;
            float rotatedY = scaledX * elementSinA + scaledY * elementCosA;
            
            elementWorldShapePoints.push_back({currentElement->x + rotatedX, currentElement->y + rotatedY});
        }
        
        // Perform polygon-polygon collision detection using SAT
        if (!entityWorldShapePoints.empty() && !elementWorldShapePoints.empty()) {
            if (polygonPolygonCollision(entityWorldShapePoints, elementWorldShapePoints)) {
                return true;
            }
        }
    }
    
    return false;
}
