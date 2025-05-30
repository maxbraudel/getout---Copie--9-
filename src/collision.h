#ifndef COLLISION_H
#define COLLISION_H

#include "elementsOnMap.h"
#include "map.h"
#include <vector>
#include <string>
#include <set>
#include "enumDefinitions.h"


// Forward declaration
struct EntityConfiguration;

// Forward declaration of playerDebugMode from player.cpp
extern bool playerDebugMode;

// Function to check if a position would collide with any collidable element
bool wouldCollideWithElement(float x, float y, float playerRadius = 0.2f);

// Function to check if an entity (using collision shape points) would collide with any element
bool wouldEntityCollideWithElement(float x, float y, const std::vector<std::pair<float, float>>& entityCollisionShapePoints, float entityScale = 1.0f, float entityRotation = 0.0f);

// Helper function for polygon-polygon collision detection using Separating Axis Theorem (SAT)
bool polygonPolygonCollision(const std::vector<std::pair<float, float>>& poly1, const std::vector<std::pair<float, float>>& poly2);

// Helper functions to check map boundary collisions with entity collision shapes
bool wouldEntityCollideWithMapBounds(float x, float y, const std::vector<std::pair<float, float>>& collisionShapePoints, float entityScale = 1.0f, float entityRotation = 0.0f);
bool wouldEntityCollideWithMapBounds(const EntityConfiguration& config, float x, float y);

// Function to check if a position would collide with a non-traversable block
bool wouldCollideWithMapBlock(float x, float y, const Map& gameMap);

// Overloaded function that checks block collision using entity-specific non-traversable blocks
bool wouldCollideWithMapBlock(float x, float y, const Map& gameMap, const std::set<BlockName>& entityNonTraversableBlocks);

// Get the names of all elements with collision enabled
std::vector<std::string> getCollidableElementNames();

// Reset the elements cache when new elements are added or removed
void resetCollisionCache();

// Spatial partitioning for collision optimization
void updateSpatialGrid();
std::vector<std::string> getNearbyElements(float x, float y, float radius);

// External declarations for collision system variables
extern bool spatialGridInitialized;
extern const float MAX_COLLISION_CHECK_RANGE;

// Set of non-traversable block types (water, lava, etc.)
extern std::set<BlockName> nonTraversableBlocks;

// Collision resolution functions
bool findSafePosition(float& x, float& y, float entityRadius, const Map& gameMap);
bool findSafePositionForEntity(float& x, float& y, const EntityConfiguration& config, const Map& gameMap);
bool resolveEntityCollisionStuck(const std::string& entityId, float& x, float& y, const EntityConfiguration& config, const Map& gameMap);

// Functions to manage non-traversable blocks
void addNonTraversableBlock(BlockName blockType);
void removeNonTraversableBlock(BlockName blockType);
bool isBlockNonTraversable(BlockName blockType);
void clearNonTraversableBlocks();
void printNonTraversableBlocks();

// Stream operator for BlockName enum
std::ostream& operator<<(std::ostream& os, const BlockName& blockType);

// Function removed - collision resolution mechanisms disabled

// Spatial partitioning functions for collision optimization
int getSpatialGridIndex(float x, float y);
void updateSpatialGrid();
std::vector<std::string> getNearbyElements(float x, float y, float radius);

#endif // COLLISION_H
