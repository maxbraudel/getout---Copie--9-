#ifndef PLAYER_H
#define PLAYER_H

// Forward declaration for safety check
class Map;

// Simple function to create a player character at the specified position
void createPlayer(float x, float y);

// Function to change the player's facing direction
// Direction values: 0 = Down, 1 = Left, 2 = Right, 3 = Up
void changePlayerDirection(int direction);

// Function to move the player relative to its current position
void movePlayer(float deltaX, float deltaY);

// Function to get the player's current position
bool getPlayerPosition(float& x, float& y);

// Function to teleport the player to a specific position
void teleportPlayer(float x, float y);

// Function to set the player's animation state
void setPlayerAnimationState(bool isAnimating);

// Function to toggle player debug mode - shows grid position
void togglePlayerDebugMode();

// Function to ensure player is not stuck in any collision areas (DISABLED)
// Collision resolution mechanisms removed - players will no longer be automatically moved
// Returns false - no automatic position adjustment
bool ensurePlayerNotStuck(const Map& gameMap);

#endif // PLAYER_H
