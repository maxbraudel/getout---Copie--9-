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
    
    // Initialize behavior for a newly created entity
    void initializeEntityBehavior(Entity& entity, const EntityConfiguration& config);

private:
    // Update behavior for a specific entity
    void updateEntityBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager);
      // Update passive state behavior (random walking)
    void updatePassiveStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config);
};

// Global behavior manager instance
extern EntityBehaviorManager entityBehaviorManager;

#endif // ENTITY_BEHAVIORS_H
