#include "player.h"
#include "elementsOnMap.h"
#include "collision.h"
#include "map.h" // For gameMap access
#include <iostream>
#include <cmath>

// Global variables for player state - 'extern' in collision.h
bool playerDebugMode = false;

// Create a player character at the specified position
void createPlayer(float x, float y) {
    // First, check if player already exists to avoid duplicates
    float existingX, existingY;
    bool playerExists = elementsManager.getElementPosition("player1", existingX, existingY);
    
    // Check for a safe starting position regardless
    float safeX = x;
    float safeY = y;
    bool needsSafePosition = false;
    
    // Check if the target position is in a collision area
    if (wouldCollideWithElement(safeX, safeY, 0.2f) || 
        wouldCollideWithMapBlock(safeX, safeY, gameMap)) {
        needsSafePosition = true;
        // Try to find a safe position nearby
        if (!findSafePosition(safeX, safeY, 0.2f, gameMap)) {
            std::cerr << "WARNING: Could not find a safe starting position for player near (" 
                      << x << ", " << y << "). Proceeding with original position." << std::endl;
            safeX = x;
            safeY = y;
        } else {
            std::cout << "Adjusted player position from (" << x << ", " << y 
                      << ") to (" << safeX << ", " << safeY << ") to avoid collisions." << std::endl;
        }
    }
    
    if (playerExists) {
        std::cout << "Player already exists at position (" << existingX << "," << existingY << ")" << std::endl;
        std::cout << "Moving existing player to position (" << safeX << "," << safeY << ")" << std::endl;
        elementsManager.changeElementCoordinates("player1", safeX, safeY);
        return;
    }
    
    // Create a single player character with animation
    // Use the texture's default anchor point as defined in the texture config
    elementsManager.placeElement(
        "player1",                   // Unique instance name
        ElementTextureName::CHARACTER1, // Using the character sprite sheet
        1.6f,                        // Scale (adjust as needed)
        safeX, safeY,                // Position on grid (potentially adjusted to be safe)
        0.0f,                        // No rotation
        0,                           // Animation row 0 (typically downward-facing)
        0,                           // Starting at first frame
        false,                       // Start without animation until movement
        12.0f,                       // Animation speed in FPS
        AnchorPoint::USE_TEXTURE_DEFAULT  // Use the texture's default anchor point
    );
    
    if (needsSafePosition && safeX != x && safeY != y) {
        std::cout << "Player created at safe position (" << safeX << "," << safeY 
                  << ") instead of requested (" << x << "," << y << ")" << std::endl;
    } else {
        std::cout << "Player created at position (" << safeX << "," << safeY << ")" << std::endl;
    }
}

// Function to change the player's facing direction
void changePlayerDirection(int direction) {
    // Direction values represent sprite sheet phases (rows):
    // 0 = Down, 1 = Left, 2 = Right, 3 = Up (for a typical RPG character sprite sheet)
    
    // First, validate the direction
    if (direction < 0 || direction > 3) {
        std::cerr << "Invalid direction value: " << direction << " (must be 0-3)" << std::endl;
        return;
    }
    
    // Change the sprite sheet phase (row) to change the direction
    elementsManager.changeElementSpritePhase("player1", direction);
}

// Function to move the player relative to its current position
void movePlayer(float deltaX, float deltaY) {
    // Before moving, check if player1 exists by trying to get its position
    float x, y;
    if (!elementsManager.getElementPosition("player1", x, y)) {
        std::cerr << "ERROR: Cannot move player - player1 does not exist!" << std::endl;
        std::cout << "Current elements in the system:" << std::endl;
        elementsManager.listElements();
        return;
    }
    
    // First, check if the player is already in a collision state and try to fix it
    // This handles the case where the player somehow got stuck in a collision area
    float currentX = x;
    float currentY = y;
    bool wasStuck = false;
    
    // Try to find a safe position for the player if they're currently stuck
    if (wouldCollideWithElement(currentX, currentY, 0.2f) || 
        wouldCollideWithMapBlock(currentX, currentY, gameMap)) {
        wasStuck = findSafePosition(currentX, currentY, 0.2f, gameMap);
        
        if (wasStuck) {
            // Player was stuck and we found a safe position, teleport them there
            elementsManager.changeElementCoordinates("player1", currentX, currentY);
            if (playerDebugMode) {
                std::cout << "Player was stuck in collision, moved to safe position: (" 
                          << currentX << ", " << currentY << ")" << std::endl;
            }
            x = currentX;
            y = currentY;
        }
    }
    
    // Calculate the new position after potential unstuck operation
    float newX = x + deltaX;
    float newY = y + deltaY;
    
    // Check if the new position would collide with any collidable element or map block
    // Using a smaller player radius for better player movement
    bool collisionWithElement = wouldCollideWithElement(newX, newY, 0.2f);
    bool collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap);
    
    if (collisionWithElement || collisionWithMapBlock) {
        // Collision detected, don't move, but still update animation and direction
        if (playerDebugMode) {
            if (collisionWithElement) {
                std::cout << "Player collision with element at position (" << newX << ", " << newY << ")" << std::endl;
            }
            if (collisionWithMapBlock) {
                std::cout << "Player collision with map block at position (" << newX << ", " << newY << ")" << std::endl;
            }
        }
    } else {
        // No collision, move the player
        elementsManager.moveElement("player1", deltaX, deltaY);
    }
    
    // Enable animation when moving
    elementsManager.changeElementAnimationStatus("player1", true);
    
    // Also change the player's facing direction based on movement
    if (deltaX > 0 && std::abs(deltaX) > std::abs(deltaY)) {
        // Moving right (and right movement is dominant)
        elementsManager.changeElementSpritePhase("player1", 1); // Assuming 2 is the right-facing animation
    } else if (deltaX < 0 && std::abs(deltaX) > std::abs(deltaY)) {
        // Moving left (and left movement is dominant)
        elementsManager.changeElementSpritePhase("player1", 2); // Assuming 1 is the left-facing animation
    } else if (deltaY > 0) {
        // Moving up (or up movement is dominant)
        elementsManager.changeElementSpritePhase("player1", 0); // Assuming 3 is the up-facing animation
    } else if (deltaY < 0) {
        // Moving down (or down movement is dominant)
        elementsManager.changeElementSpritePhase("player1", 3); // Assuming 0 is the down-facing animation
    }

    // Display player position in debug mode
    if (playerDebugMode) {
        float x, y;
        if (getPlayerPosition(x, y)) {
            std::cout << "Player position: (" << x << ", " << y << ")" << std::endl;
        }
    }
}

// Function to get the player's current position
bool getPlayerPosition(float& x, float& y) {
    return elementsManager.getElementPosition("player1", x, y);
}

// Function to teleport the player to a specific position
void teleportPlayer(float x, float y) {
    float targetX = x;
    float targetY = y;
    
    // Check for collision at the teleport destination
    bool collisionWithElement = wouldCollideWithElement(targetX, targetY, 0.2f);
    bool collisionWithMapBlock = wouldCollideWithMapBlock(targetX, targetY, gameMap);
    
    if (collisionWithElement || collisionWithMapBlock) {
        // Try to find a safe position near the requested coordinates
        if (findSafePosition(targetX, targetY, 0.2f, gameMap)) {
            // Found a safe position near the requested coordinates
            std::cout << "Adjusted teleport position from (" << x << ", " << y << ") to ("
                    << targetX << ", " << targetY << ") to avoid collisions." << std::endl;
        } else {
            // Could not find a safe position
            std::cout << "Cannot teleport player to (" << x << ", " << y << ") - ";
            if (collisionWithElement) {
                std::cout << "position is occupied by a collidable element";
            }
            if (collisionWithMapBlock) {
                std::cout << (collisionWithElement ? " and " : "") << "position has a non-traversable map block";
            }
            std::cout << "." << std::endl;
            return;
        }
    }
    
    // Directly set player position without affecting animation
    elementsManager.changeElementCoordinates("player1", targetX, targetY);
}

// Function to set the player's animation state
void setPlayerAnimationState(bool isAnimating) {
    elementsManager.changeElementAnimationStatus("player1", isAnimating);
}

// Function to toggle player debug mode - shows grid position
void togglePlayerDebugMode() {
    playerDebugMode = !playerDebugMode;
    std::cout << "Player debug mode " << (playerDebugMode ? "enabled" : "disabled") << std::endl;
}

// Function to ensure player is not stuck in any collision areas
bool ensurePlayerNotStuck(const Map& gameMap) {
    float x, y;
    if (!getPlayerPosition(x, y)) {
        // Player doesn't exist, nothing to do
        return false;
    }
    
    // Check if player is in a collision state
    if (wouldCollideWithElement(x, y, 0.2f) || wouldCollideWithMapBlock(x, y, gameMap)) {
        // Player is stuck, try to find a safe position
        if (findSafePosition(x, y, 0.2f, gameMap)) {
            // Found a safe position, move player there
            elementsManager.changeElementCoordinates("player1", x, y);
            if (playerDebugMode) {
                std::cout << "Safety check: Player was stuck in collision, moved to safe position: (" 
                          << x << ", " << y << ")" << std::endl;
            }
            return true; // Position was adjusted
        } else {
            // Could not find a safe position
            if (playerDebugMode) {
                std::cout << "Warning: Player is stuck in collision at (" << x << ", " << y 
                          << ") and no safe position could be found!" << std::endl;
            }
        }
    }
    
    return false; // No adjustment needed or possible
}
