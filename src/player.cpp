#include "player.h"
#include "entities.h" // Use entity system instead of direct element management
#include "collision.h"
#include "map.h" // For gameMap access
#include "globals.h" // Added for GRID_SIZE
#include <iostream>
#include <cmath>

// Global variables for player state - 'extern' in collision.h
bool playerDebugMode = false;

// Define a getter for player configuration to ensure it's always up-to-date
inline const EntityConfiguration* getPlayerConfig() {
    return entitiesManager.getConfiguration("player");
}


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

// Function to move the player relative to its current position
void movePlayer(float deltaX, float deltaY) {
    // Before moving, check if player1 exists by trying to get its position
    // Get player entity configuration for collision parameters
    const EntityConfiguration* config = getPlayerConfig();
    if (!config) {
        std::cerr << "ERROR: Cannot move player - player configuration not found!" << std::endl;
        return;
    }
      // Use player-specific non-traversable blocks from entity configuration
    std::set<TextureName> playerNonTraversableBlocks = config->nonTraversableBlocks;

    float x, y;
    if (!elementsManager.getElementPosition("player1", x, y)) {
        std::cerr << "ERROR: Cannot move player - player1 does not exist!" << std::endl;
        std::cout << "Current elements in the system:" << std::endl;
        elementsManager.listElements();
        return;
    }      // First, check if the player is already in a collision state and try to fix it
    // This handles the case where the player somehow got stuck in a collision area
    float currentX = x;
    float currentY = y;
    bool wasStuck = false;      // Try to find a safe position for the player if they're currently stuck
    if (wouldEntityCollideWithElement(*config, currentX, currentY) || 
        wouldCollideWithMapBlock(currentX, currentY, gameMap, playerNonTraversableBlocks)) {
        wasStuck = findSafePosition(currentX, currentY, config->collisionRadius, gameMap);
        
        if (wasStuck) {
            // Player was stuck and we found a safe position, teleport them there
            elementsManager.changeElementCoordinates("player1", currentX, currentY);
            if (playerDebugMode) {
                std::cout << "Player was stuck in collision, moved to safe position: (" 
                          << currentX << ", " << currentY << ")" << std::endl;
            }
            x = currentX;
            y = currentY;
        }else {
            // Failed to find a safe position - try with a larger radius search
            // This is a more aggressive attempt to get the player unstuck
            std::cout << "WARNING: Player is stuck in collision and standard recovery failed." << std::endl;
            std::cout << "Attempting emergency teleport to find a safe location..." << std::endl;
            
            // Scan the entire map if needed to find a safe position
            // Try first at logical starting points like (10,10) and then expand search
            
            // Start with potential safe locations rather than just the center
            std::vector<std::pair<float, float>> safeLocations = {
                {10.0f, 10.0f},  // Center area
                {20.0f, 20.0f},  // Another likely safe spot
                {15.0f, 15.0f},  // Another region
                {5.0f, 5.0f}     // Near the start
            };
            
            bool foundSafe = false;
            for (const auto& location : safeLocations) {
                currentX = location.first;
                currentY = location.second;                // Make sure this location is actually safe
                if (!wouldEntityCollideWithElement(*config, currentX, currentY) && 
                    !wouldCollideWithMapBlock(currentX, currentY, gameMap, playerNonTraversableBlocks)) {
                    elementsManager.changeElementCoordinates("player1", currentX, currentY);
                    std::cout << "Player emergency teleported to (" << currentX << ", " << currentY << ")" << std::endl;
                    x = currentX;
                    y = currentY;
                    wasStuck = true;
                    foundSafe = true;
                    break;
                }
            }
            
            // If still stuck, use a grid search over large areas of the map
            if (!foundSafe) {
                std::cout << "Initial safe locations failed, scanning map grid..." << std::endl;
                
                // Grid search with larger step size for efficiency
                for (int gridX = 0; gridX < GRID_SIZE && !foundSafe; gridX += 5) {
                    for (int gridY = 0; gridY < GRID_SIZE && !foundSafe; gridY += 5) {
                        // Center of the block
                        currentX = gridX + 0.5f;
                        currentY = gridY + 0.5f;                        // Check if position is safe
                        if (!wouldEntityCollideWithElement(*config, currentX, currentY) && 
                            !wouldCollideWithMapBlock(currentX, currentY, gameMap, playerNonTraversableBlocks)) {
                            // Found a safe position
                            elementsManager.changeElementCoordinates("player1", currentX, currentY);
                            std::cout << "Player emergency teleported to (" << currentX << ", " << currentY << ")" << std::endl;
                            x = currentX;
                            y = currentY;
                            wasStuck = true;
                            foundSafe = true;
                        }
                    }
                }
            }
            
            // If for some extreme reason all failed, force the player to a position and disable collisions temporarily
            if (!foundSafe) {
                std::cout << "CRITICAL: All safe position searches failed! Forcing player to center of map." << std::endl;
                elementsManager.changeElementCoordinates("player1", 10.0f, 10.0f);
                x = 10.0f;
                y = 10.0f;
                wasStuck = true;
            }
        }
    }      // Calculate the new position after potential unstuck operation
    float newX = x + deltaX;
    float newY = y + deltaY;      // Check if the combined movement would collide with any collidable element or map block
    bool collisionWithElement = wouldEntityCollideWithElement(*config, newX, newY);
    bool collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap, playerNonTraversableBlocks);
    bool canMove = !(collisionWithElement || collisionWithMapBlock);
    
    // If we can't move diagonally, try to move in single directions (sliding along walls)
    float actualDeltaX = 0;
    float actualDeltaY = 0;
    bool movedPartially = false;
    
    if (!canMove && (deltaX != 0 && deltaY != 0)) {        // Try moving only horizontally
        float testX = x + deltaX;
        float testY = y; // Keep Y the same
        
        bool horizontalCollision = wouldEntityCollideWithElement(*config, testX, testY) || 
                                  wouldCollideWithMapBlock(testX, testY, gameMap, playerNonTraversableBlocks);
          // Try moving only vertically
        float testX2 = x; // Keep X the same
        float testY2 = y + deltaY;
        
        bool verticalCollision = wouldEntityCollideWithElement(*config, testX2, testY2) || 
                                wouldCollideWithMapBlock(testX2, testY2, gameMap, playerNonTraversableBlocks);
        
        // If horizontal movement is possible
        if (!horizontalCollision) {
            actualDeltaX = deltaX;
            movedPartially = true;
            
            if (playerDebugMode) {
                std::cout << "Player can slide horizontally by " << deltaX << std::endl;
            }
        }
        
        // If vertical movement is possible
        if (!verticalCollision) {
            actualDeltaY = deltaY;
            movedPartially = true;
            
            if (playerDebugMode) {
                std::cout << "Player can slide vertically by " << deltaY << std::endl;
            }
        }
    } else if (canMove) {
        // If we can move diagonally, use the original deltas
        actualDeltaX = deltaX;
        actualDeltaY = deltaY;
        movedPartially = true;
    }
    
    if (movedPartially) {
        // Move the player with the possible movement deltas
        elementsManager.moveElement("player1", actualDeltaX, actualDeltaY);
        
        // Enable animation since player is moving (at least partially)
        elementsManager.changeElementAnimationStatus("player1", true);
        
        // Set the appropriate animation speed based on movement type (shift key state)
        float animationSpeed;
        if (keyPressedStates[GLFW_KEY_LEFT_SHIFT] || keyPressedStates[GLFW_KEY_RIGHT_SHIFT]) {
            // Sprint animation
            animationSpeed = config->sprintWalkingAnimationSpeed;
        } else {
            // Normal walking animation
            animationSpeed = config->normalWalkingAnimationSpeed;
        }
        
        // Update the animation speed
        elementsManager.changeElementAnimationSpeed("player1", animationSpeed);
    } else {
        // No movement in any direction, disable animation
        if (playerDebugMode) {
            std::cout << "Player cannot move in any direction from (" << x << ", " << y << ")" << std::endl;
        }
        
        // Disable animation since player can't move at all
        elementsManager.changeElementAnimationStatus("player1", false);
        
        // Set to standing frame
        elementsManager.changeElementSpriteFrame("player1", 0);
    }    // Change the player's facing direction based on attempted movement direction
    // Use actualDeltaX/Y if we're moving partially, otherwise use requested deltaX/Y for facing
    float directionDeltaX = movedPartially ? actualDeltaX : deltaX;
    float directionDeltaY = movedPartially ? actualDeltaY : deltaY;
    
    // Use the already declared config variable from above
    if (config) {
        if (directionDeltaX > 0 && std::abs(directionDeltaX) > std::abs(directionDeltaY)) {
            // Moving right (and right movement is dominant)
            elementsManager.changeElementSpritePhase("player1", config->spritePhaseWalkRight);
        } else if (directionDeltaX < 0 && std::abs(directionDeltaX) > std::abs(directionDeltaY)) {
            // Moving left (and left movement is dominant)
            elementsManager.changeElementSpritePhase("player1", config->spritePhaseWalkLeft);
        } else if (directionDeltaY > 0) {
            // Moving up (or up movement is dominant)
            elementsManager.changeElementSpritePhase("player1", config->spritePhaseWalkUp);
        } else if (directionDeltaY < 0) {
            // Moving down (or down movement is dominant)
            elementsManager.changeElementSpritePhase("player1", config->spritePhaseWalkDown);
        }
    }

    // Display player position in debug mode
    if (playerDebugMode) {
        float x, y;
        if (getPlayerPosition(x, y)) {
            std::cout << "Player position: (" << x << ", " << y << ")" << std::endl;
        }
    }
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
    // Get the player entity configuration to use correct collision settings
    const EntityConfiguration* playerConfig = getPlayerConfig();
    if (!playerConfig) {
        std::cerr << "ERROR: Cannot ensure player not stuck - player configuration not found!" << std::endl;
        return false;
    }
    
    // Delegate to the entity system's collision checking
    return entitiesManager.ensureEntityNotStuck(PLAYER_INSTANCE_NAME);
}
