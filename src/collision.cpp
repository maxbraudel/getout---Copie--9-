#include "collision.h"
#include "elementsOnMap.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include <set>

// Define M_PI if not already defined (needed for angle calculations)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Cache for collidable element names
static std::vector<std::string> collidableElementNames;
static bool collisionCacheInitialized = false;

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
    if (!collisionCacheInitialized) {
        // First time initialization - find all collidable elements
        collidableElementNames.clear();
        
        // Get all elements and filter for those with collision enabled
        // This is expensive, so we cache the results
        const auto& elements = elementsManager.getElements();
        for (const auto& element : elements) {
            // Check if the element has collision enabled
            if (element.hasCollision) {
                collidableElementNames.push_back(element.instanceName);
            }
        }
        
        collisionCacheInitialized = true;
        std::cout << "Initialized collision system with " << collidableElementNames.size() << " collidable elements." << std::endl;
        
        // Log non-traversable blocks
        std::cout << "Non-traversable block types: ";
        for (const auto& blockType : nonTraversableBlocks) {
            std::cout << static_cast<int>(blockType) << " ";
        }
        std::cout << std::endl;
    }
    
    return collidableElementNames;
}

// Function to check if a position would collide with a collidable element
bool wouldCollideWithElement(float x, float y, float playerRadius) {
    // Get all collidable element names (cached after first call)
    std::vector<std::string> collidables = getCollidableElementNames();
    
    // Check distance to each collidable element
    for (const auto& elementName : collidables) {
        float elementX, elementY;
        if (elementsManager.getElementPosition(elementName, elementX, elementY)) {
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
            
            // Calculate distance between player and element
            float dx = x - elementX;
            float dy = y - elementY;
            float distanceSquared = dx*dx + dy*dy;
            
            // Combined radius for collision detection
            float combinedRadius = playerRadius + elementRadius;
            
            // If distance is less than combined radius, we have a collision
            if (distanceSquared < combinedRadius * combinedRadius) {
                // Debug info about collision
                std::cout << "Collision detected with " << elementName 
                          << " at distance " << std::sqrt(distanceSquared)
                          << " (combined radius: " << combinedRadius << ")" << std::endl;
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
}

// Function to check if a position would collide with a non-traversable map block
bool wouldCollideWithMapBlock(float x, float y, const Map& gameMap) {
    // Convert floating point coordinates to grid integers
    int gridX = static_cast<int>(x);
    int gridY = static_cast<int>(y);
    
    // Get the texture type at these coordinates
    TextureName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
    
    // Check if this block type is in our set of non-traversable blocks
    if (nonTraversableBlocks.find(blockType) != nonTraversableBlocks.end()) {
        if (playerDebugMode) {
            std::cout << "Map block collision detected at (" << x << ", " << y 
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
    const float maxDistance = 5.0f; // Increased maximum distance to check for safe position
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
        std::cout << "Spiral search failed, trying grid search..." << std::endl;
    }
    
    // Try a grid search in a square around the player
    for (float dx = -maxDistance; dx <= maxDistance; dx += step) {
        for (float dy = -maxDistance; dy <= maxDistance; dy += step) {
            // Skip points we've already checked in the spiral (approximately)
            if (std::sqrt(dx*dx + dy*dy) <= maxDistance - step) {
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
                              << "), moved player " << std::sqrt(dx*dx + dy*dy) << " units" << std::endl;
                }
                
                x = testX;
                y = testY;
                return true; // Position was adjusted
            }
        }
    }
    
    // If we got here, couldn't find a safe position within reasonable distance
    if (playerDebugMode) {
        std::cout << "Warning: Could not find safe position for player within " 
                  << maxDistance << " units!" << std::endl;
    }
    
    return false; // Couldn't adjust position
}
