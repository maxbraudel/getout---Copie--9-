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
        else {
            // Failed to find a safe position - try with a larger radius search
            // This is a more aggressive attempt to get the player unstuck
            std::cout << "WARNING: Player is stuck in collision and standard recovery failed." << std::endl;
            std::cout << "Attempting emergency teleport to a known safe location..." << std::endl;
            
            // Try to teleport to the center of the map or another known safe location
            currentX = 10.0f;
            currentY = 10.0f;
            
            // Make sure this location is actually safe
            if (!wouldCollideWithElement(currentX, currentY, 0.2f) && 
                !wouldCollideWithMapBlock(currentX, currentY, gameMap)) {
                elementsManager.changeElementCoordinates("player1", currentX, currentY);
                std::cout << "Player emergency teleported to (" << currentX << ", " << currentY << ")" << std::endl;
                x = currentX;
                y = currentY;
                wasStuck = true;
            }
        }
    }
      // Calculate the new position after potential unstuck operation
    float newX = x + deltaX;
    float newY = y + deltaY;
    
    // Check if the combined movement would collide with any collidable element or map block
    bool collisionWithElement = wouldCollideWithElement(newX, newY, 0.2f);
    bool collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap);
    bool canMove = !(collisionWithElement || collisionWithMapBlock);
    
    // If we can't move diagonally, try to move in single directions (sliding along walls)
    float actualDeltaX = 0;
    float actualDeltaY = 0;
    bool movedPartially = false;
    
    if (!canMove && (deltaX != 0 && deltaY != 0)) {
        // Try moving only horizontally
        float testX = x + deltaX;
        float testY = y; // Keep Y the same
        
        bool horizontalCollision = wouldCollideWithElement(testX, testY, 0.2f) || 
                                  wouldCollideWithMapBlock(testX, testY, gameMap);
        
        // Try moving only vertically
        float testX2 = x; // Keep X the same
        float testY2 = y + deltaY;
        
        bool verticalCollision = wouldCollideWithElement(testX2, testY2, 0.2f) || 
                                wouldCollideWithMapBlock(testX2, testY2, gameMap);
        
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
      // Change the player's facing direction based on attempted movement direction
    // Use actualDeltaX/Y if we're moving partially, otherwise use requested deltaX/Y for facing
    float directionDeltaX = movedPartially ? actualDeltaX : deltaX;
    float directionDeltaY = movedPartially ? actualDeltaY : deltaY;
    
    if (directionDeltaX > 0 && std::abs(directionDeltaX) > std::abs(directionDeltaY)) {
        // Moving right (and right movement is dominant)
        elementsManager.changeElementSpritePhase("player1", 1); // Right-facing animation
    } else if (directionDeltaX < 0 && std::abs(directionDeltaX) > std::abs(directionDeltaY)) {
        // Moving left (and left movement is dominant)
        elementsManager.changeElementSpritePhase("player1", 2); // Left-facing animation
    } else if (directionDeltaY > 0) {
        // Moving up (or up movement is dominant)
        elementsManager.changeElementSpritePhase("player1", 0); // Up-facing animation
    } else if (directionDeltaY < 0) {
        // Moving down (or down movement is dominant)
        elementsManager.changeElementSpritePhase("player1", 3); // Down-facing animation
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
        // Log the collision type for debugging
        if (playerDebugMode) {            std::cout << "Teleport destination has collision: ";
            if (collisionWithMapBlock) {
                int gridX = static_cast<int>(targetX);
                int gridY = static_cast<int>(targetY);
                TextureName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
                std::cout << "Map block type: " << static_cast<int>(blockType) << " ";
            }
            if (collisionWithElement) {
                std::cout << "Element collision";
            }
            std::cout << std::endl;
        }
        
        // Try to find a safe position near the requested coordinates
        // First attempt - standard search with normal radius
        if (findSafePosition(targetX, targetY, 0.2f, gameMap)) {
            // Found a safe position near the requested coordinates
            std::cout << "Adjusted teleport position from (" << x << ", " << y << ") to ("
                    << targetX << ", " << targetY << ") to avoid collisions." << std::endl;
        } else {
            // Could not find a safe position with standard search
            // Try a different approach - try points in an expanding square
            std::cout << "Standard search failed. Trying alternate safe position search..." << std::endl;
            
            // Try in a square pattern around the target
            bool foundSafe = false;
            float searchRadius = 1.0f;
            float maxSearchRadius = 10.0f;
            
            while (!foundSafe && searchRadius <= maxSearchRadius) {
                // Try the four corners of the square
                float testPoints[4][2] = {
                    {x + searchRadius, y + searchRadius},
                    {x + searchRadius, y - searchRadius},
                    {x - searchRadius, y + searchRadius},
                    {x - searchRadius, y - searchRadius}
                };
                
                for (int i = 0; i < 4; i++) {
                    float testX = testPoints[i][0];
                    float testY = testPoints[i][1];
                    
                    if (!wouldCollideWithElement(testX, testY, 0.2f) && 
                        !wouldCollideWithMapBlock(testX, testY, gameMap)) {
                        targetX = testX;
                        targetY = testY;
                        foundSafe = true;
                        std::cout << "Found alternate safe position at (" << targetX << ", " << targetY << ")" << std::endl;
                        break;
                    }
                }
                
                // Increase search radius for next iteration
                searchRadius += 1.0f;
            }
            
            // If we still haven't found a safe position, use the emergency teleport to 10,10
            if (!foundSafe) {
                // Could not find a safe position
                std::cout << "Cannot find any safe position near (" << x << ", " << y << ") - ";
                if (collisionWithElement) {
                    std::cout << "position is occupied by a collidable element";
                }
                if (collisionWithMapBlock) {
                    std::cout << (collisionWithElement ? " and " : "") << "position has a non-traversable map block";
                }
                std::cout << "." << std::endl;
                
                // Default to a known safe position (assuming center of map is safe)
                targetX = 10.0f;
                targetY = 10.0f;
                std::cout << "Emergency teleport to default position (" << targetX << ", " << targetY << ")" << std::endl;
            }
        }
    }
    
    // Directly set player position without affecting animation
    elementsManager.changeElementCoordinates("player1", targetX, targetY);
    
    // Double-check that the player isn't still stuck somehow
    if (wouldCollideWithElement(targetX, targetY, 0.2f) || 
        wouldCollideWithMapBlock(targetX, targetY, gameMap)) {
        std::cout << "WARNING: Player still in collision after teleport attempt!" << std::endl;
        
        // Run the safety check to force a resolution
        float x = targetX, y = targetY;
        if (findSafePosition(x, y, 0.2f, gameMap)) {
            elementsManager.changeElementCoordinates("player1", x, y);
            std::cout << "Emergency correction applied, player moved to (" << x << ", " << y << ")" << std::endl;
        }
    }
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
    bool hasElementCollision = wouldCollideWithElement(x, y, 0.2f);
    bool hasMapBlockCollision = wouldCollideWithMapBlock(x, y, gameMap);
    
    if (hasElementCollision || hasMapBlockCollision) {
        // Log what we're colliding with for easier debugging
        std::cout << "Player stuck detection: ";
        if (hasElementCollision) std::cout << "Collision with element. ";
        if (hasMapBlockCollision) {
            int gridX = static_cast<int>(x);
            int gridY = static_cast<int>(y);
            TextureName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
            // std::cout << "Collision with map block type: " << getBlockTypeName(blockType);
        }
        std::cout << std::endl;
        
        // Player is stuck, try to find a safe position
        if (findSafePosition(x, y, 0.2f, gameMap)) {
            // Found a safe position, move player there
            elementsManager.changeElementCoordinates("player1", x, y);
            std::cout << "Safety check: Player was stuck in collision, moved to safe position: (" 
                      << x << ", " << y << ")" << std::endl;
            return true; // Position was adjusted
        } else {
            // Could not find a safe position using standard search
            std::cout << "Warning: Player is stuck in collision and automatic recovery failed!" << std::endl;
            
            // Try a more aggressive approach - teleport to a known safe area
            // First try the center of the map
            x = 10.0f;
            y = 10.0f;
            
            // Ensure the target position is actually safe
            if (!wouldCollideWithElement(x, y, 0.2f) && !wouldCollideWithMapBlock(x, y, gameMap)) {
                elementsManager.changeElementCoordinates("player1", x, y);
                std::cout << "Emergency recovery: Player teleported to (" << x << ", " << y << ")" << std::endl;
                return true;
            } else {
                // If center isn't safe, try a few other common positions
                for (int testX = 5; testX < 15; testX += 5) {
                    for (int testY = 5; testY < 15; testY += 5) {
                        x = static_cast<float>(testX);
                        y = static_cast<float>(testY);
                        if (!wouldCollideWithElement(x, y, 0.2f) && !wouldCollideWithMapBlock(x, y, gameMap)) {
                            elementsManager.changeElementCoordinates("player1", x, y);
                            std::cout << "Last resort recovery: Player teleported to (" << x << ", " << y << ")" << std::endl;
                            return true;
                        }
                    }
                }
                
                std::cout << "CRITICAL: Could not find ANY safe position for player!" << std::endl;
            }
        }
    }
    
    return false; // No adjustment needed or possible
}
