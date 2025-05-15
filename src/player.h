#ifndef PLAYER_H
#define PLAYER_H

// Simple function to create a player character at the specified position
void createPlayer(float x, float y);

// Function to change the player's facing direction
// Direction values: 0 = Down, 1 = Left, 2 = Right, 3 = Up
void changePlayerDirection(int direction);

// Function to move the player relative to its current position
void movePlayer(float deltaX, float deltaY);

#endif // PLAYER_H
