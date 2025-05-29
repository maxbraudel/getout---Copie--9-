#include "entityBehaviors.h"
#include "entities.h"
#include "elementsOnMap.h" // For global elementsManager
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
    
    // PRIORITY SYSTEM: Alert state overrides all other states
    // Check for alert state first (highest priority)
    if (config->alertState) {
        updateAlertStateBehavior(entity, deltaTime, entitiesManager, *config);
        
        // If entity is in alert state, skip other behaviors
        if (entity.isInAlertState) {
            return;
        }
    }
    
    // Update passive state behavior (lower priority - only if not in alert)
    if (config->passiveState && !entity.isInAlertState) {
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
    
    // Initialize alert state behavior
    if (config.alertState) {
        entity.isInAlertState = false;
        entity.alertTargetEntity.clear();
        entity.alertTargetX = 0.0f;
        entity.alertTargetY = 0.0f;
        entity.alertStateStartTime = 0.0;
        
        std::cout << "Initialized alert behavior for entity " << entity.instanceName 
                  << " - watching radius " << config.alertStateStartRadius << " to " << config.alertStateEndRadius;
        
        if (!config.alertStateEntitiesList.empty()) {
            std::cout << " for entities: ";
            for (size_t i = 0; i < config.alertStateEntitiesList.size(); ++i) {
                std::cout << config.alertStateEntitiesList[i];
                if (i < config.alertStateEntitiesList.size() - 1) std::cout << ", ";
            }
        } else {
            std::cout << " for all entities";
        }
        std::cout << std::endl;
    }
}

void EntityBehaviorManager::updateAlertStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config) {
    // Check if entity should enter alert state
    if (!entity.isInAlertState) {
        if (checkForAlertTriggers(entity, entitiesManager, config)) {
            // Entity just entered alert state - stop current movement
            if (entity.isWalking) {
                entity.isWalking = false;
                entity.path.clear();
                entity.currentPathIndex = 0;
                  // Stop animation and movement
                std::string elementName = EntitiesManager::getElementName(entity.instanceName);
                elementsManager.changeElementAnimationStatus(elementName, false);
                elementsManager.changeElementSpriteFrame(elementName, 0);
                
                std::cout << "Entity " << entity.instanceName << " entered alert state - stopping movement" << std::endl;
            }
        }
    } else {
        // Entity is already in alert state - check if target is still in range
        bool targetStillInRange = false;
          // Get current entity position
        std::string elementName = EntitiesManager::getElementName(entity.instanceName);
        float entityX, entityY;
        if (elementsManager.getElementPosition(elementName, entityX, entityY)) {
            
            // Check if alert target entity still exists and is in range
            if (!entity.alertTargetEntity.empty()) {
                std::string targetElementName = EntitiesManager::getElementName(entity.alertTargetEntity);
                float targetX, targetY;
                
                if (elementsManager.getElementPosition(targetElementName, targetX, targetY)) {
                    float distance = std::sqrt((targetX - entityX) * (targetX - entityX) + (targetY - entityY) * (targetY - entityY));
                    
                    if (distance >= config.alertStateStartRadius && distance <= config.alertStateEndRadius) {
                        targetStillInRange = true;
                        
                        // Update target position and look at it
                        entity.alertTargetX = targetX;
                        entity.alertTargetY = targetY;
                        lookAtTarget(entity, targetX, targetY, entitiesManager, config);
                    }
                }
            }
        }
          // If target is no longer in range, exit alert state
        if (!targetStillInRange) {
            entity.isInAlertState = false;
            entity.alertTargetEntity.clear();
            entity.alertTargetX = 0.0f;
            entity.alertTargetY = 0.0f;
            entity.alertStateStartTime = 0.0;
            
            std::cout << "Entity " << entity.instanceName << " exited alert state - target out of range, retaining direction " << entity.lastDirection << std::endl;
            
            // Keep the direction the entity was looking during alert state
            // Convert lastDirection to appropriate sprite phase
            std::string elementName = EntitiesManager::getElementName(entity.instanceName);
            int phase;
            switch (entity.lastDirection) {
                case 0: phase = config.spritePhaseWalkUp; break;
                case 1: phase = config.spritePhaseWalkDown; break;
                case 2: phase = config.spritePhaseWalkLeft; break;
                case 3: phase = config.spritePhaseWalkRight; break;
                default: phase = config.defaultSpriteSheetPhase; break;
            }
            elementsManager.changeElementSpritePhase(elementName, phase);
        }
    }
}

bool EntityBehaviorManager::checkForAlertTriggers(Entity& entity, EntitiesManager& entitiesManager, const EntityConfiguration& config) {
    // Get current entity position
    std::string elementName = EntitiesManager::getElementName(entity.instanceName);
    float entityX, entityY;
    if (!elementsManager.getElementPosition(elementName, entityX, entityY)) {
        return false; // Can't get position
    }
    
    // Check all entities in the alert watch list
    const auto& allEntities = entitiesManager.getEntities();
    
    for (const auto& pair : allEntities) {
        const Entity& otherEntity = pair.second;
        
        // Skip self
        if (otherEntity.instanceName == entity.instanceName) {
            continue;
        }
        
        // Check if this entity is in our watch list (empty list means watch all)
        bool shouldWatch = config.alertStateEntitiesList.empty();
        if (!shouldWatch) {
            for (const std::string& watchedEntity : config.alertStateEntitiesList) {
                if (otherEntity.instanceName == watchedEntity) {
                    shouldWatch = true;
                    break;
                }
            }
        }
        
        if (!shouldWatch) {
            continue; // Not watching this entity
        }
        
        // Get other entity position
        std::string otherElementName = EntitiesManager::getElementName(otherEntity.instanceName);
        float otherX, otherY;
        if (!elementsManager.getElementPosition(otherElementName, otherX, otherY)) {
            continue; // Can't get other entity position
        }
        
        // Calculate distance
        float distance = std::sqrt((otherX - entityX) * (otherX - entityX) + (otherY - entityY) * (otherY - entityY));
        
        // Check if other entity is within alert range
        if (distance >= config.alertStateStartRadius && distance <= config.alertStateEndRadius) {
            // Found an entity in alert range - enter alert state
            entity.isInAlertState = true;
            entity.alertTargetEntity = otherEntity.instanceName;
            entity.alertTargetX = otherX;
            entity.alertTargetY = otherY;
            entity.alertStateStartTime = entity.behaviorTimer;
            
            std::cout << "Entity " << entity.instanceName << " detected " << otherEntity.instanceName 
                      << " at distance " << distance << " - entering alert state" << std::endl;
            
            // Look at the target immediately
            lookAtTarget(entity, otherX, otherY, entitiesManager, config);
            
            return true;
        }
    }
    
    return false; // No entities found in alert range
}

void EntityBehaviorManager::lookAtTarget(Entity& entity, float targetX, float targetY, EntitiesManager& entitiesManager, const EntityConfiguration& config) {
    // Get current entity position
    std::string elementName = EntitiesManager::getElementName(entity.instanceName);
    float entityX, entityY;
    if (!elementsManager.getElementPosition(elementName, entityX, entityY)) {
        return; // Can't get position
    }
    
    // Calculate direction to target
    float dx = targetX - entityX;
    float dy = targetY - entityY;
    
    // Determine the primary direction for sprite updates
    int direction;
    if (std::abs(dx) >= std::abs(dy)) {
        // Horizontal movement dominates
        direction = (dx > 0) ? 3 : 2;  // 3 = right, 2 = left
    } else {
        // Vertical movement dominates
        direction = (dy > 0) ? 0 : 1;  // 0 = up, 1 = down
    }
    
    // Update entity direction
    entity.lastDirection = direction;
    
    // Set the appropriate sprite phase based on direction
    int phase;
    switch (direction) {
        case 0: phase = config.spritePhaseWalkUp; break;
        case 1: phase = config.spritePhaseWalkDown; break;
        case 2: phase = config.spritePhaseWalkLeft; break;
        case 3: phase = config.spritePhaseWalkRight; break;
        default: phase = config.defaultSpriteSheetPhase; break;
    }
      // Change the sprite phase to look at target
    elementsManager.changeElementSpritePhase(elementName, phase);
    
    std::cout << "Entity " << entity.instanceName << " looking at target (" << targetX << ", " << targetY 
              << ") - direction: " << direction << ", phase: " << phase << std::endl;
}
