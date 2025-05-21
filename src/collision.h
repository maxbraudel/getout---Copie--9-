#ifndef COLLISION_H
#define COLLISION_H

#include "elementsOnMap.h"
#include <vector>
#include <string>

// Function to check if a position would collide with any collidable element
bool wouldCollideWithElement(float x, float y, float playerRadius = 0.2f);

// Get the names of all elements with collision enabled
std::vector<std::string> getCollidableElementNames();

// Reset the elements cache when new elements are added or removed
void resetCollisionCache();

#endif // COLLISION_H
