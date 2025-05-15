#include "player.h"
#include "elementsOnMap.h"
#include <iostream>

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
    }
      // Create a single player character with animation
    // Using bottom center as anchor point so player appears to be standing on the ground
    elementsManager.placeElement(
        "player1",                   // Unique instance name
        ElementTextureName::CHARACTER1, // Using the character sprite sheet
        2.0f,                        // Scale (adjust as needed)
        x, y,                        // Position on grid
        0.0f,                        // No rotation
        0,                           // Animation row 0 (typically downward-facing)
        0,                           // Starting at first frame
        false,                        // Enable animation
        11.0f,                       // Animation speed in FPS
        AnchorPoint::BOTTOM_LEFT_CORNER, // Anchor at bottom of character sprite
        0.5f, 0.0f                   // Offset X to center horizontally (X: 0.5 = halfway from left to right)
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
    
    // Move the player by the specified amount
    elementsManager.moveElement("player1", deltaX, deltaY);
    
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
