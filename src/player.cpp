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
        10.0f                         // Animation speed in FPS
    );
    
    std::cout << "Player created at position (" << x << "," << y << ")" << std::endl;
}
