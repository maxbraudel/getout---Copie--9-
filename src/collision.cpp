#include "collision.h"
#include "elementsOnMap.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

// Cache for tree element names
static std::vector<std::string> treeElementNames;
static bool treesInitialized = false;

// Function to get all tree element names in the game
std::vector<std::string> getTreeElementNames() {
    if (!treesInitialized) {
        // First time initialization - find all tree elements
        treeElementNames.clear();
        
        // Get all elements and filter for coconut trees
        // This is expensive, so we cache the results
        const auto& elements = elementsManager.getElements();
        for (const auto& element : elements) {
            // Check if the element is a coconut tree by texture name
            if (element.textureName == ElementTextureName::COCONUT_TREE_1 ||
                element.textureName == ElementTextureName::COCONUT_TREE_2 ||
                element.textureName == ElementTextureName::COCONUT_TREE_3) {
                
                treeElementNames.push_back(element.instanceName);
            }
        }
        
        treesInitialized = true;
        std::cout << "Initialized collision system with " << treeElementNames.size() << " trees." << std::endl;
    }
    
    return treeElementNames;
}

// Function to check if a position would collide with a tree
bool wouldCollideWithTree(float x, float y, float collisionRadius) {
    // Get all tree element names (cached after first call)
    std::vector<std::string> trees = getTreeElementNames();
    
    // Check distance to each tree
    for (const auto& treeName : trees) {
        float treeX, treeY;
        if (elementsManager.getElementPosition(treeName, treeX, treeY)) {
            // Calculate distance between player and tree
            float dx = x - treeX;
            float dy = y - treeY;
            float distanceSquared = dx*dx + dy*dy;
            
            // If distance is less than collision radius, we have a collision
            if (distanceSquared < collisionRadius * collisionRadius) {
                return true; // Collision detected
            }
        }
    }
    
    // If we got here, no collision was detected
    return false;
}

// Reset the trees cache when new trees are added
void resetTreesCache() {
    treesInitialized = false;
}
