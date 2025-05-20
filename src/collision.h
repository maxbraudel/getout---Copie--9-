#ifndef COLLISION_H
#define COLLISION_H

#include "elementsOnMap.h"
#include <vector>
#include <string>

// Function to check if a position would collide with a tree
bool wouldCollideWithTree(float x, float y, float collisionRadius = 0.4f);

// Get the names of all coconut trees in the game
std::vector<std::string> getTreeElementNames();

// Reset the trees cache when new trees are added
void resetTreesCache();

#endif // COLLISION_H
