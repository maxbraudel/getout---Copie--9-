#ifndef PLAYER_H
#define PLAYER_H
#include "enumDefinitions.h"


// Forward declaration for safety check
class Map;

// Player stuck detection state variables
struct PlayerStuckState {
    float lastPositionX = 0.0f;
    float lastPositionY = 0.0f;
    float stuckCheckTime = 0.0f;
    float lastPositionChangeTime = 0.0f;
    int stuckCount = 0;
};

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

// Function to handle player stuck detection and resolution
bool handlePlayerStuckDetection(float currentX, float currentY, double deltaTime, bool canMove);

// Function to resolve player collision when stuck
bool resolvePlayerCollisionStuck(float& x, float& y);

// Function to get the player's current facing direction (0=Up, 1=Right, 2=Left, 3=Down)
int getPlayerDirection();

// Function to place an ICE block in front of the player
void placeIceBlockInFront();

// Function to check for coconuts within 1 block radius and collect them
void checkAndCollectCoconuts();

#endif // PLAYER_H
