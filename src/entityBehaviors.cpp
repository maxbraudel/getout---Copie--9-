#include "entityBehaviors.h"
#include "entities.h"
#include "elementsOnMap.h" // For global elementsManager
#include <iostream>
#include <random>
#include <chrono>
#include "enumDefinitions.h"
#include "enumDefinitions.h"


// Global behavior manager instance
EntityBehaviorManager entityBehaviorManager;

EntityBehaviorManager::EntityBehaviorManager() {
    // Constructor
}

EntityBehaviorManager::~EntityBehaviorManager() {
    // Destructor
}

void EntityBehaviorManager::update(double deltaTime, EntitiesManager& entitiesManager) {
    // CRASH FIX: Safe entity iteration with proper reference handling
    try {
        // Get reference to entities map and collect instance names first
        const auto& entitiesMap = entitiesManager.getEntities();
        std::vector<std::string> entityNames;
        entityNames.reserve(entitiesMap.size());
        
        for (const auto& pair : entitiesMap) {
            entityNames.push_back(pair.first);
        }
        
        // Process each entity by name to avoid iterator invalidation
        for (const std::string& instanceName : entityNames) {
            // CRASH FIX: Verify entity still exists before processing
            if (!entitiesManager.entityExists(instanceName)) {
                std::cout << "WARNING: Entity " << instanceName << " no longer exists during behavior update" << std::endl;
                continue;
            }
            
            // Get fresh reference to the actual entity (not a copy)
            Entity* entity = entitiesManager.getEntity(instanceName);
            if (!entity) {
                std::cout << "WARNING: Could not get entity reference for " << instanceName << std::endl;
                continue;
            }
            
            // Update behavior using reference to actual entity
            updateEntityBehavior(*entity, deltaTime, entitiesManager);
        }
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception in entity behavior update: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "CRITICAL: Unknown exception in entity behavior update!" << std::endl;
    }
}

void EntityBehaviorManager::updateEntityBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager) {
    // Get entity configuration
    const EntityConfiguration* config = entitiesManager.getConfiguration(entity.type);
    if (!config || !config->automaticBehaviors) {
        return; // No automatic behaviors enabled
    }

    // PASSIVE STATE - only behavior state remaining after alert and flee removal
    if (config->passiveState) {
        updatePassiveStateBehavior(entity, deltaTime, entitiesManager, *config);
    }
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
