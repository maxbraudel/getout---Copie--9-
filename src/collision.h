#ifndef COLLISION_H
#define COLLISION_H

#include "elementsOnMap.h"
#include "map.h"
#include <vector>
#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <mutex>
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

// Helper function for circle-polygon collision detection
bool circlePolygonCollision(float circleX, float circleY, float radius, const std::vector<std::pair<float, float>>& polygon);

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
extern const float MAX_COLLISION_CHECK_RANGE;

// Set of non-traversable block types (water, lava, etc.)
extern std::set<BlockName> nonTraversableBlocks;

// Collision resolution functions
bool findSafePosition(float& x, float& y, float entityRadius, const Map& gameMap);
bool findSafePositionForEntity(float& x, float& y, const EntityConfiguration& config, const Map& gameMap, const std::string& excludeInstanceName = "");
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

// Enhanced spatial partitioning for collision optimization
struct SpatialCell {
    std::vector<std::string> staticElements;  // Elements that rarely move
    std::vector<std::string> dynamicElements; // Elements that move frequently
    float lastUpdateTime = 0.0f;
    bool isDirty = true;
};

// Hierarchical spatial grid for multi-level collision detection
class HierarchicalSpatialGrid {
private:    static const int COARSE_GRID_SIZE = 50;  // Large cells for broad-phase
    static const int FINE_GRID_SIZE = 10;    // Small cells for narrow-phase
    static constexpr float STATIC_UPDATE_INTERVAL = 2.0f;   // Static elements update less frequently
    static constexpr float DYNAMIC_UPDATE_INTERVAL = 0.1f;  // Dynamic elements update more frequently
    
    std::unordered_map<int, SpatialCell> coarseGrid;
    std::unordered_map<int, SpatialCell> fineGrid;
    std::unordered_set<std::string> staticElementNames;
    std::unordered_set<std::string> dynamicElementNames;
    
    bool isInitialized = false;
    float lastCoarseUpdateTime = 0.0f;
    float lastFineUpdateTime = 0.0f;
    
public:
    void initialize();
    void updateGrid(bool forceUpdate = false);
    void updateStaticElements();
    void updateDynamicElements();
    void markElementAsDynamic(const std::string& elementName);
    void markElementAsStatic(const std::string& elementName);
    
    // Fast broad-phase collision detection
    std::vector<std::string> getBroadPhaseElements(float x, float y, float radius);
    
    // Precise narrow-phase collision detection
    std::vector<std::string> getNarrowPhaseElements(float x, float y, float radius);
    
    // Combined hierarchical lookup
    std::vector<std::string> getElementsHierarchical(float x, float y, float radius);
      void clear();
    bool isEmpty() const;
    bool isInitializedState() const;
    size_t getStaticElementCount() const;
    size_t getDynamicElementCount() const;
    
private:
    int getCoarseGridIndex(float x, float y) const;
    int getFineGridIndex(float x, float y) const;
    void addElementToGrid(const std::string& elementName, float x, float y, bool isStatic);
    void removeElementFromGrid(const std::string& elementName);
};

// Global instance of hierarchical spatial grid
extern HierarchicalSpatialGrid g_hierarchicalGrid;
extern std::mutex g_hierarchicalGridMutex;

// HIERARCHICAL ENTITY GRID FOR OPTIMIZED ENTITY-TO-ENTITY COLLISION
// Thread-safe hierarchical entity spatial grid for massive performance improvements
class HierarchicalEntityGrid {
private:
    static const int COARSE_GRID_SIZE = 50;  // Large cells for broad-phase entity search
    static const int FINE_GRID_SIZE = 10;    // Small cells for narrow-phase entity search
    static constexpr float STATIC_UPDATE_INTERVAL = 1.0f;   // Entity positions change less frequently than elements
    static constexpr float DYNAMIC_UPDATE_INTERVAL = 0.05f; // More frequent updates for moving entities
    
    std::unordered_map<int, SpatialCell> coarseGrid;
    std::unordered_map<int, SpatialCell> fineGrid;
    std::unordered_set<std::string> entityInstanceNames;
    
    bool isInitialized = false;
    float lastCoarseUpdateTime = 0.0f;
    float lastFineUpdateTime = 0.0f;
    
public:
    void initialize();
    void updateGrid(bool forceUpdate = false);
    void updateEntityPositions();
    
    // Fast broad-phase entity collision detection
    std::vector<std::string> getBroadPhaseEntities(float x, float y, float radius);
    
    // Precise narrow-phase entity collision detection
    std::vector<std::string> getNarrowPhaseEntities(float x, float y, float radius);
    
    // Combined hierarchical entity lookup (REPLACES getNearbyEntities)
    std::vector<std::string> getEntitiesHierarchical(float x, float y, float radius);
    
    void addEntity(const std::string& instanceName, float x, float y);
    void removeEntity(const std::string& instanceName);
    void clear();
    bool isEmpty() const;
    bool isInitializedState() const;
    size_t getEntityCount() const;
    
private:
    int getCoarseGridIndex(float x, float y) const;
    int getFineGridIndex(float x, float y) const;
    void addEntityToGrid(const std::string& instanceName, float x, float y);
};

// Global hierarchical entity grid instance
extern HierarchicalEntityGrid g_hierarchicalEntityGrid;
extern std::mutex g_hierarchicalEntityGridMutex;

// Enhanced collision detection functions using hierarchical grid
bool wouldCollideWithElementHierarchical(float x, float y, float playerRadius = 0.2f);
bool wouldEntityCollideWithElementHierarchical(float x, float y, const std::vector<std::pair<float, float>>& entityCollisionShapePoints, float entityScale = 1.0f, float entityRotation = 0.0f);

// Performance monitoring for collision system
struct CollisionPerformanceStats {
    std::atomic<int> broadPhaseChecks{0};
    std::atomic<int> narrowPhaseChecks{0};
    std::atomic<int> hierarchicalHits{0};
    std::atomic<int> totalCollisionQueries{0};
    std::atomic<double> totalTimeMs{0.0};
    
    void reset();
    void printStats() const;
};

extern CollisionPerformanceStats g_collisionStats;

#endif // COLLISION_H
