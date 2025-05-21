#include "collision.h"
#include "elementsOnMap.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

// Cache for collidable element names
static std::vector<std::string> collidableElementNames;
static bool collisionCacheInitialized = false;

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
