#ifndef ENTITY_BEHAVIORS_H
#define ENTITY_BEHAVIORS_H

#include "entities.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <random>
#include <chrono>

// Forward declaration
class EntityBehaviorManager;

// Structure to hold behavior state for a specific entity instance
struct EntityBehaviorState {
    std::string instanceName;
    std::string typeName;
    float lastActionTime;
    float nextActionDelay;
    bool isActive;
    
    // Additional state data specific to behaviors
    float targetX;
    float targetY;
    
    // Default constructor (required for std::map)
    EntityBehaviorState() 
        : instanceName("")
        , typeName("")
        , lastActionTime(0.0f)
        , nextActionDelay(0.0f)
        , isActive(true)
        , targetX(0.0f)
        , targetY(0.0f) {}
    
    // Parameterized constructor
    EntityBehaviorState(const std::string& instance, const std::string& type) 
        : instanceName(instance)
        , typeName(type)
        , lastActionTime(0.0f)
        , nextActionDelay(0.0f)
        , isActive(true)
        , targetX(0.0f)
        , targetY(0.0f) {}
};

// Type alias for behavior function
using BehaviorFunction = std::function<void(EntityBehaviorState&, float, EntityBehaviorManager&)>;

// Class to manage entity behaviors
class EntityBehaviorManager {
public:
    EntityBehaviorManager();
    ~EntityBehaviorManager();
    
    // Initialize behaviors
    void initialize();
    
    // Register an entity to be managed by the behavior system
    void registerEntity(const std::string& instanceName, const std::string& typeName);
    
    // Unregister an entity from the behavior system
    void unregisterEntity(const std::string& instanceName);
    
    // Update all entity behaviors (called each frame)
    void update(float deltaTime);
    
    // Get the current position of an entity
    bool getEntityPosition(const std::string& instanceName, float& x, float& y);
    
    // Make an entity move to a given position
    bool moveEntityTo(const std::string& instanceName, float x, float y);
    
    // Check if an entity has reached its target
    bool hasEntityReachedTarget(const std::string& instanceName);
      // Get a random position near an entity
    bool getRandomPositionNear(const std::string& instanceName, float& x, float& y, float radius = 10.0f);
    
    // Get a random position near an entity that is on a specific block type
    bool getRandomPositionNearOnBlockTypes(const std::string& instanceName, float& x, float& y, float radius = 10.0f);
    
    // Generate a random delay between min and max seconds
    float getRandomDelay(float minSeconds = 1.0f, float maxSeconds = 4.0f);

private:
    // Map of behavior functions by entity type
    std::map<std::string, BehaviorFunction> behaviorFunctions;
    
    // Map of entity behavior states
    std::map<std::string, EntityBehaviorState> entityStates;
    
    // Random number generator
    std::mt19937 rng;
    
    // Register built-in behaviors
    void registerBuiltInBehaviors();
    
    // Define the antagonist behavior
    static void antagonistBehavior(EntityBehaviorState& state, float deltaTime, EntityBehaviorManager& manager);
};

// Global instance
extern EntityBehaviorManager entityBehaviorManager;

#endif // ENTITY_BEHAVIORS_H
