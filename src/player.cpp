#include "player.h"
#include "elementsOnMap.h"
#include <iostream>

// Create a player character at the specified position
void createPlayer(float x, float y) {
    // Create a single player character with animation
    elementsManager.placeElement(
        "player1",                   // Unique instance name
        ElementTextureName::CHARACTER1, // Using the character sprite sheet
        3.0f,                        // Scale (adjust as needed)
        x, y,                        // Position on grid
        0.0f,                        // No rotation
        0,                           // Animation row 0 (typically downward-facing)
        0,                           // Starting at first frame
        true,                        // Enable animation
        10.0f                        // Animation speed in FPS
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
    // Move the player by the specified amount
    elementsManager.moveElement("player1", deltaX, deltaY);
    
    // Also change the player's facing direction based on movement
    if (deltaX > 0) {
        // Moving right
        elementsManager.changeElementSpritePhase("player1", 2); // Assuming 2 is the right-facing animation
    } else if (deltaX < 0) {
        // Moving left
        elementsManager.changeElementSpritePhase("player1", 1); // Assuming 1 is the left-facing animation
    } else if (deltaY > 0) {
        // Moving up
        elementsManager.changeElementSpritePhase("player1", 3); // Assuming 3 is the up-facing animation
    } else if (deltaY < 0) {
        // Moving down
        elementsManager.changeElementSpritePhase("player1", 0); // Assuming 0 is the down-facing animation
    }
}
