#ifndef ENTITY_BEHAVIORS_H
#define ENTITY_BEHAVIORS_H
#include "enumDefinitions.h"


#include <string>

// Forward declarations
class EntitiesManager;
struct Entity;
struct EntityConfiguration;

// Entity Behavior Manager class
class EntityBehaviorManager {
public:
    EntityBehaviorManager();
    ~EntityBehaviorManager();
      // Main update function - called each frame to update all entity behaviors
    void update(double deltaTime, EntitiesManager& entitiesManager);
    
    // Main update function with view frustum culling - called each frame to update all entity behaviors
    void update(double deltaTime, EntitiesManager& entitiesManager, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop);
    
    // Initialize behavior for a newly created entity
    void initializeEntityBehavior(Entity& entity, const EntityConfiguration& config);

private:
    // Update behavior for a specific entity
    void updateEntityBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager);    // Update passive state behavior (random walking)
    void updatePassiveStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config);
      // Update alert state behavior (facing trigger entities)
    void updateAlertStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config);
      // Update flee state behavior (running away from trigger entities)
    void updateFleeStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config);
    
    // Update attack state behavior (charging at trigger entities)
    void updateAttackStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config);
};

// Helper function to calculate distance between collision boundaries of two entities
float calculateDistanceBetweenEntityCollisionBoundaries(
    const std::string& entityInstanceName1, float x1, float y1,
    const std::string& entityInstanceName2, float x2, float y2,
    EntitiesManager& entitiesManager
);

// Helper function to calculate distance from a point to a line segment
float pointToLineSegmentDistance(float px, float py, float x1, float y1, float x2, float y2);

// Global behavior manager instance
extern EntityBehaviorManager entityBehaviorManager;

#endif // ENTITY_BEHAVIORS_H
