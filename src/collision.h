#ifndef COLLISION_H
#define COLLISION_H

#include "elementsOnMap.h"
#include "map.h"
#include <vector>
#include <string>
#include <set>

// Forward declaration of playerDebugMode from player.cpp
extern bool playerDebugMode;

// Function to check if a position would collide with any collidable element
bool wouldCollideWithElement(float x, float y, float playerRadius = 0.2f);

// Function to check if a position would collide with a non-traversable block
bool wouldCollideWithMapBlock(float x, float y, const Map& gameMap);

// Get the names of all elements with collision enabled
std::vector<std::string> getCollidableElementNames();

// Reset the elements cache when new elements are added or removed
void resetCollisionCache();

// Set of non-traversable block types (water, lava, etc.)
extern std::set<TextureName> nonTraversableBlocks;

// Functions to manage non-traversable blocks
void addNonTraversableBlock(TextureName blockType);
void removeNonTraversableBlock(TextureName blockType);
bool isBlockNonTraversable(TextureName blockType);
void clearNonTraversableBlocks();
void printNonTraversableBlocks();

// Stream operator for TextureName enum
std::ostream& operator<<(std::ostream& os, const TextureName& blockType);

// Function to find a safe position when player is stuck inside a collision area
// Returns true if the player position was adjusted, false if no adjustment was necessary
bool findSafePosition(float& x, float& y, float playerRadius, const Map& gameMap);

// Spatial partitioning functions for collision optimization
int getSpatialGridIndex(float x, float y);
void updateSpatialGrid();
std::vector<std::string> getNearbyElements(float x, float y, float radius);

#endif // COLLISION_H
