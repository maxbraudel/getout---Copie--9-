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

#endif // COLLISION_H
