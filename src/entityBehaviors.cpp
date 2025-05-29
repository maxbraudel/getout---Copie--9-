#include "entityBehaviors.h"
#include "entities.h"
#include <iostream>
#include <random>
#include <chrono>

// Global behavior manager instance
EntityBehaviorManager entityBehaviorManager;

EntityBehaviorManager::EntityBehaviorManager() {
    // Constructor
}

EntityBehaviorManager::~EntityBehaviorManager() {
    // Destructor
}

void EntityBehaviorManager::update(double deltaTime, EntitiesManager& entitiesManager) {
    // Update all entity behaviors
    for (auto& pair : entitiesManager.getEntities()) {
        Entity& entity = pair.second;
        updateEntityBehavior(entity, deltaTime, entitiesManager);
    }
}

void EntityBehaviorManager::updateEntityBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager) {
    // Get entity configuration
    const EntityConfiguration* config = entitiesManager.getConfiguration(entity.typeName);
    if (!config || !config->automaticBehaviors) {
        return; // No automatic behaviors enabled
    }
    
    // Update passive state behavior
    if (config->passiveState) {
        updatePassiveStateBehavior(entity, deltaTime, entitiesManager, *config);
    }
    
    // Additional behavior types can be added here in the future
}

void EntityBehaviorManager::updatePassiveStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config) {
    // Update the behavior timer
    entity.behaviorTimer += deltaTime;
    
    // Check if it's time to trigger a random walk
    if (entity.behaviorTimer >= entity.nextBehaviorTriggerTime) {
        // Reset timer
        entity.behaviorTimer = 0.0;
        
        // Generate next trigger time randomly between min and max
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> timeDist(config.passiveStateRandomWalkTriggerTimeIntervalMin, 
                                                       config.passiveStateRandomWalkTriggerTimeIntervalMax);
        entity.nextBehaviorTriggerTime = timeDist(gen);
        
        // Only trigger random walk if entity is not currently moving or waiting for path
        if (!entity.isWalking && !entity.isWaitingForPath) {
            std::cout << "Triggering passive behavior for entity " << entity.instanceName 
                      << " - walking to random target within radius " << config.passiveStateWalkingRadius << std::endl;
            
            // Trigger random walk within specified radius
            entitiesManager.walkEntityWithPathFindingToRandomRadiusTarget(
                entity.instanceName, 
                config.passiveStateWalkingRadius, 
                WalkType::NORMAL
            );
        } else {
            std::cout << "Entity " << entity.instanceName 
                      << " is busy (walking or waiting), skipping passive behavior trigger" << std::endl;
        }
    }
}

void EntityBehaviorManager::initializeEntityBehavior(Entity& entity, const EntityConfiguration& config) {
    if (!config.automaticBehaviors) {
        return; // No automatic behaviors to initialize
    }
    
    // Initialize passive state behavior
    if (config.passiveState) {
        // Initialize behavior timer and next trigger time
        entity.behaviorTimer = 0.0;
        
        // Generate initial trigger time randomly between min and max
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> timeDist(config.passiveStateRandomWalkTriggerTimeIntervalMin, 
                                                       config.passiveStateRandomWalkTriggerTimeIntervalMax);
        entity.nextBehaviorTriggerTime = timeDist(gen);
        
        std::cout << "Initialized passive behavior for entity " << entity.instanceName 
                  << " - first trigger in " << entity.nextBehaviorTriggerTime << " seconds" << std::endl;
    }
}
