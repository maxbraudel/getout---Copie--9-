#include "player.h"
#include "entities.h" // Use entity system instead of direct element management
#include "collision.h"
#include "map.h" // For gameMap access
#include "globals.h" // Added for GRID_SIZE
#include <iostream>
#include <cmath>

// Global variables for player state - 'extern' in collision.h
bool playerDebugMode = false;

// Player entity instance name - consistent identifier for the entity system
static const std::string PLAYER_INSTANCE_NAME = "player1";

// Create a player character at the specified position using the entity system
void createPlayer(float x, float y) {
    // Use the entity system to create the player
    // The entity system will handle collision checking, safe positioning, and all configuration
    bool success = entitiesManager.placeEntity(PLAYER_INSTANCE_NAME, "player", x, y);
    
    if (success) {
        if (playerDebugMode) {
            float actualX, actualY;
            if (getPlayerPosition(actualX, actualY)) {
                std::cout << "Player created via entity system at position (" << actualX << "," << actualY << ")" << std::endl;
            }
        }
    } else {
        std::cerr << "Failed to create player via entity system at position (" << x << "," << y << ")" << std::endl;
    }
}

// Function to change the player's facing direction - now uses entity system
void changePlayerDirection(int direction) {
    // Direction values represent sprite sheet phases (rows):
    // 0 = Up, 1 = Right, 2 = Left, 3 = Down (based on entity system configuration)
    
    // First, validate the direction
    if (direction < 0 || direction > 3) {
        std::cerr << "Invalid direction value: " << direction << " (must be 0-3)" << std::endl;
        return;
    }
    
    // Get the player entity to access its configuration
    const EntityConfiguration* config = entitiesManager.getConfiguration("player");
    if (!config) {
        std::cerr << "Player configuration not found in entity system" << std::endl;
        return;
    }
    
    // Get the element name for the player
    std::string elementName = EntitiesManager::getElementName(PLAYER_INSTANCE_NAME);
    
    // Map direction to appropriate sprite phase based on entity configuration
    int phase;
    switch (direction) {
        case 0: phase = config->spritePhaseWalkUp; break;
        case 1: phase = config->spritePhaseWalkRight; break;
        case 2: phase = config->spritePhaseWalkLeft; break;
        case 3: phase = config->spritePhaseWalkDown; break;
        default: phase = config->defaultSpriteSheetPhase; break;
    }
    
    // Change the sprite sheet phase (row) to change the direction
    elementsManager.changeElementSpritePhase(elementName, phase);
}



// Function to get the player's current position using entity system
bool getPlayerPosition(float& x, float& y) {
    std::string elementName = EntitiesManager::getElementName(PLAYER_INSTANCE_NAME);
    return elementsManager.getElementPosition(elementName, x, y);
}

// Function to teleport the player to a specific position using entity system
void teleportPlayer(float x, float y) {
    // Use the entity system's teleport function which handles collisions automatically
    bool success = entitiesManager.teleportEntity(PLAYER_INSTANCE_NAME, x, y);
    
    if (!success) {
        std::cerr << "Failed to teleport player to (" << x << ", " << y << ")" << std::endl;
    } else if (playerDebugMode) {
        float actualX, actualY;
        if (getPlayerPosition(actualX, actualY)) {
            std::cout << "Player teleported to (" << actualX << ", " << actualY << ")" << std::endl;
        }
    }
}

// Function to set the player's animation state using entity system
void setPlayerAnimationState(bool isAnimating) {
    std::string elementName = EntitiesManager::getElementName(PLAYER_INSTANCE_NAME);
    elementsManager.changeElementAnimationStatus(elementName, isAnimating);
}

// Function to toggle player debug mode - shows grid position
void togglePlayerDebugMode() {
    playerDebugMode = !playerDebugMode;
    std::cout << "Player debug mode " << (playerDebugMode ? "enabled" : "disabled") << std::endl;
}

// Function to ensure player is not stuck using entity system
bool ensurePlayerNotStuck(const Map& gameMap) {
    // Delegate to the entity system's collision checking
    return entitiesManager.ensureEntityNotStuck(PLAYER_INSTANCE_NAME);
}
