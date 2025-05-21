#include "collision.h"
#include "elementsOnMap.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include <set>

// Cache for collidable element names
static std::vector<std::string> collidableElementNames;
static bool collisionCacheInitialized = false;

// Define the set of non-traversable blocks
std::set<TextureName> nonTraversableBlocks = {
    TextureName::WATER_0,
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
