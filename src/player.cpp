#include "player.h"
#include "entities.h" // Use entity system instead of direct element management
#include "collision.h"
#include "map.h" // For gameMap access
#include "globals.h" // Added for GRID_SIZE
#include <iostream>
#include <cmath>
#include "enumDefinitions.h"


// Global variables for player state - 'extern' in collision.h
bool playerDebugMode = false;

// Player stuck detection state
static PlayerStuckState playerStuckState;

// Define a getter for player configuration to ensure it's always up-to-date
inline const EntityConfiguration* getPlayerConfig() {
    return entitiesManager.getConfiguration(EntityName::PLAYER);
}


// Player entity instance name - consistent identifier for the entity system
static const std::string PLAYER_INSTANCE_NAME = "player1";

// Create a player character at the specified position using the entity system
void createPlayer(float x, float y) {
    // COLLISION RESOLUTION MECHANISMS DISABLED
    // Use the entity system to create the player at the exact requested coordinates
    // No automatic safe positioning - player will be placed exactly where requested
    bool success = entitiesManager.placeEntity(PLAYER_INSTANCE_NAME, EntityName::PLAYER, x, y);
    
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
    const EntityConfiguration* config = entitiesManager.getConfiguration(EntityName::PLAYER);
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
        return;    }
    
    // Entity collision detection now uses polygon shapes only
    float x, y;
    if (!elementsManager.getElementPosition("player1", x, y)) {
        std::cerr << "ERROR: Cannot move player - player1 does not exist!" << std::endl;
        std::cout << "Current elements in the system:" << std::endl;
        elementsManager.listElements();
        return;
    }

    // COLLISION RESOLUTION MECHANISMS REMOVED
    // Players will no longer be automatically moved to "safe positions" when stuck
      // Calculate the new position
    float newX = x + deltaX;
    float newY = y + deltaY;
    
    // Check if the combined movement would collide with any collidable element or block
    bool collisionWithElement = wouldEntityCollideWithElementsGranular(*config, newX, newY, false);
    bool collisionWithBlock = wouldEntityCollideWithBlocksGranular(*config, newX, newY, false);
    bool canMove = !(collisionWithElement || collisionWithBlock);
    
    // If we can't move diagonally, try to move in single directions (sliding along walls)
    float actualDeltaX = 0;
    float actualDeltaY = 0;
    bool movedPartially = false;
      if (!canMove && (deltaX != 0 && deltaY != 0)) {
        // Try moving only horizontally
        float testX = x + deltaX;
        float testY = y; // Keep Y the same
        bool horizontalElementCollision = wouldEntityCollideWithElementsGranular(*config, testX, testY, false);
        bool horizontalBlockCollision = wouldEntityCollideWithBlocksGranular(*config, testX, testY, false);
        bool horizontalCollision = horizontalElementCollision || horizontalBlockCollision;
        
        // Try moving only vertically
        float testX2 = x; // Keep X the same
        float testY2 = y + deltaY;
        bool verticalElementCollision = wouldEntityCollideWithElementsGranular(*config, testX2, testY2, false);
        bool verticalBlockCollision = wouldEntityCollideWithBlocksGranular(*config, testX2, testY2, false);
        bool verticalCollision = verticalElementCollision || verticalBlockCollision;
        
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
    }

    // Handle player stuck detection and collision resolution
    // Get current actual position after potential movement
    float currentX, currentY;
    if (getPlayerPosition(currentX, currentY)) {
        handlePlayerStuckDetection(currentX, currentY, 0.016, !movedPartially);
    }

    // Change the player's facing direction based on attempted movement direction
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

// Function to handle player stuck detection and collision resolution
bool handlePlayerStuckDetection(float currentX, float currentY, double deltaTime, bool canMove) {
    const EntityConfiguration* config = getPlayerConfig();
    if (!config) {
        return false;
    }

    // STUCK DETECTION AND COLLISION RESOLUTION LOGIC (similar to entities)
    const float stuckThreshold = 1.0f; // Check if player is stuck for 1 second
    const float positionChangeThreshold = 0.02f; // Minimum movement to consider "not stuck"
    
    // Check if player position has changed significantly
    float positionChangeDistance = std::sqrt(
        (currentX - playerStuckState.lastPositionX) * (currentX - playerStuckState.lastPositionX) +
        (currentY - playerStuckState.lastPositionY) * (currentY - playerStuckState.lastPositionY)
    );
    
    // Update stuck detection timing
    playerStuckState.stuckCheckTime += static_cast<float>(deltaTime);
    
    if (positionChangeDistance > positionChangeThreshold) {
        // Player has moved significantly, reset stuck detection
        playerStuckState.lastPositionX = currentX;
        playerStuckState.lastPositionY = currentY;
        playerStuckState.lastPositionChangeTime = playerStuckState.stuckCheckTime;
        playerStuckState.stuckCount = 0;
        return false; // Not stuck
    } else if (playerStuckState.stuckCheckTime - playerStuckState.lastPositionChangeTime >= stuckThreshold) {
        // Player has been stuck for too long, try collision resolution
        playerStuckState.stuckCount++;
        
        if (playerDebugMode) {
            std::cout << "Player is stuck (count: " << playerStuckState.stuckCount 
                      << ") at (" << currentX << ", " << currentY 
                      << ") - attempting collision resolution..." << std::endl;
        }
        
        float safeX = currentX;
        float safeY = currentY;
        
        if (resolvePlayerCollisionStuck(safeX, safeY)) {
            // Successfully found a safe position, teleport player there
            elementsManager.changeElementCoordinates("player1", safeX, safeY);
            
            // Reset stuck detection after successful resolution
            playerStuckState.lastPositionX = safeX;
            playerStuckState.lastPositionY = safeY;
            playerStuckState.lastPositionChangeTime = playerStuckState.stuckCheckTime;
            playerStuckState.stuckCount = 0;
            
            if (playerDebugMode) {
                std::cout << "Successfully resolved stuck condition for player - moved to safe position (" 
                          << safeX << ", " << safeY << ")" << std::endl;
            }
            return true; // Stuck condition resolved
        } else {
            if (playerDebugMode) {
                std::cout << "Failed to resolve stuck condition for player - no safe position found. Attempt count: " 
                          << playerStuckState.stuckCount << std::endl;
            }
            
            // If we've tried many times and still can't resolve, give up for a while
            if (playerStuckState.stuckCount >= 5) {
                if (playerDebugMode) {
                    std::cout << "Player has been stuck too many times - temporarily disabling stuck detection" << std::endl;
                }
                // Reset timing to avoid immediate re-triggering
                playerStuckState.lastPositionChangeTime = playerStuckState.stuckCheckTime;
                playerStuckState.stuckCount = 0;
            }
        }
        
        // Reset stuck check time to avoid immediate re-triggering
        playerStuckState.lastPositionChangeTime = playerStuckState.stuckCheckTime;
    }
    
    return false; // Not resolved or not stuck yet
}

// Function to resolve player collision when stuck
bool resolvePlayerCollisionStuck(float& x, float& y) {
    const EntityConfiguration* config = getPlayerConfig();
    if (!config) {
        return false;
    }
    
    if (playerDebugMode) {
        std::cout << "Collision resolution requested for player at position (" << x << ", " << y << ")" << std::endl;
    }
    
    // Use the enhanced entity collision resolution function with player's entity configuration
    bool success = findSafePositionForEntity(x, y, *config, gameMap);
    
    if (success) {
        if (playerDebugMode) {
            std::cout << "Successfully resolved collision for player - moved to (" << x << ", " << y << ")" << std::endl;
        }
    } else {
        if (playerDebugMode) {
            std::cout << "Failed to resolve collision for player - no safe position found within search radius" << std::endl;
        }
    }
    
    return success;
}

// Function to ensure player is not stuck using entity system (DISABLED)

