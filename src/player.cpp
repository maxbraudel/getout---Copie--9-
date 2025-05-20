#include "player.h"
#include "elementsOnMap.h"
#include "collision.h"
#include <iostream>
#include <cmath>

// Global variables for player state
static bool playerDebugMode = false;

// Create a player character at the specified position
void createPlayer(float x, float y) {
    // First, check if player already exists to avoid duplicates
    float existingX, existingY;
    bool playerExists = elementsManager.getElementPosition("player1", existingX, existingY);
    
    if (playerExists) {
        std::cout << "Player already exists at position (" << existingX << "," << existingY << ")" << std::endl;
        std::cout << "Moving existing player to position (" << x << "," << y << ")" << std::endl;
        elementsManager.changeElementCoordinates("player1", x, y);
        return;
    }    // Create a single player character with animation
    // Using the CHARACTER1 texture's default anchor point (BOTTOM_CENTER)
    elementsManager.placeElement(
        "player1",                   // Unique instance name
        ElementTextureName::CHARACTER1, // Using the character sprite sheet
        1.6f,                        // Scale (adjust as needed)
        x, y,                        // Position on grid
        0.0f,                        // No rotation
        0,                           // Animation row 0 (typically downward-facing)
        0,                           // Starting at first frame
        false,                       // Start without animation until movement
        12.0f,
        AnchorPoint::BOTTOM_CENTER                     // Animation speed in FPS
        // No explicit anchor point - use the texture's default BOTTOM_CENTER
    );
    
    std::cout << "Player created at position (" << x << "," << y << ")" << std::endl;
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
    
    // Calculate the new position
    float newX = x + deltaX;
    float newY = y + deltaY;
    
    // Check if the new position would collide with a tree
    if (wouldCollideWithTree(newX, newY)) {
        // Collision detected, don't move, but still update animation and direction
        if (playerDebugMode) {
            std::cout << "Player collision with tree at position (" << newX << ", " << newY << ")" << std::endl;
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
    // Check for collision at the teleport destination
    if (wouldCollideWithTree(x, y)) {
        std::cout << "Cannot teleport player to (" << x << ", " << y << ") - position is occupied by a tree." << std::endl;
        return;
    }
    
    // Directly set player position without affecting animation
    elementsManager.changeElementCoordinates("player1", x, y);
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
