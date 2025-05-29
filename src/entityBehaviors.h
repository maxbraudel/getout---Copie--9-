#ifndef ENTITY_BEHAVIORS_H
#define ENTITY_BEHAVIORS_H

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
    
    // Initialize behavior for a newly created entity
    void initializeEntityBehavior(Entity& entity, const EntityConfiguration& config);

private:
    // Update behavior for a specific entity
    void updateEntityBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager);
    
    // Update passive state behavior (random walking)
    void updatePassiveStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config);
    
    // Update alert state behavior (look at nearby entities)
    void updateAlertStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config);
    
    // Check if entity should enter alert state
    bool checkForAlertTriggers(Entity& entity, EntitiesManager& entitiesManager, const EntityConfiguration& config);
    
    // Update entity direction to look at target
    void lookAtTarget(Entity& entity, float targetX, float targetY, EntitiesManager& entitiesManager, const EntityConfiguration& config);
};

// Global behavior manager instance
extern EntityBehaviorManager entityBehaviorManager;

#endif // ENTITY_BEHAVIORS_H
