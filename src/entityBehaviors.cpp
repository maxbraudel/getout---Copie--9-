#include "entityBehaviors.h"
#include "entities.h"
#include "elementsOnMap.h" // For global elementsManager
#include <iostream>
#include <random>
#include <chrono>
#include <limits> // For numeric_limits
#include <cmath> // For sqrt, atan2
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
    }    // FLEE STATE - highest priority behavior that can interrupt all others
    if (config->fleeState) {
        updateFleeStateBehavior(entity, deltaTime, entitiesManager, *config);
    }

    // ALERT STATE - priority behavior that can interrupt passive state (but not flee state)
    if (config->alertState && !entity.isInFleeState) {
        updateAlertStateBehavior(entity, deltaTime, entitiesManager, *config);
    }

    // PASSIVE STATE - only triggered when not in alert or flee state
    if (config->passiveState && !entity.isInAlertState && !entity.isInFleeState) {
        updatePassiveStateBehavior(entity, deltaTime, entitiesManager, *config);
    } else if (config->passiveState && (entity.isInAlertState || entity.isInFleeState)) {
        // Debug: Log when passive state is skipped due to higher priority states
        std::cout << "DEBUG: Skipping passive state for " << entity.instanceName 
                  << " because isInAlertState = " << entity.isInAlertState 
                  << ", isInFleeState = " << entity.isInFleeState << std::endl;
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

void EntityBehaviorManager::updateAlertStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config) {
    // Get current entity position
    std::string elementName = EntitiesManager::getElementName(entity.instanceName);
    float currentX, currentY;
    if (!elementsManager.getElementPosition(elementName, currentX, currentY)) {
        return; // Can't get position, skip alert behavior
    }
    
    // Find the nearest trigger entity within the alert range
    std::string nearestTriggerEntity = "";
    float nearestDistance = std::numeric_limits<float>::max();
    bool foundTriggerEntity = false;
    
    // Check all entities to find trigger entities
    const auto& allEntities = entitiesManager.getEntities();
    for (const auto& pair : allEntities) {
        const Entity& otherEntity = pair.second;
        
        // Skip self
        if (otherEntity.instanceName == entity.instanceName) {
            continue;
        }
        
        // Check if this entity type is in the trigger list
        bool isTriggerEntity = false;
        for (const EntityName& triggerType : config.alertStateTriggerEntitiesList) {
            if (otherEntity.type == triggerType) {
                isTriggerEntity = true;
                break;
            }
        }
        
        if (!isTriggerEntity) {
            continue;
        }
        
        // Get other entity position
        std::string otherElementName = EntitiesManager::getElementName(otherEntity.instanceName);
        float otherX, otherY;
        if (!elementsManager.getElementPosition(otherElementName, otherX, otherY)) {
            continue; // Can't get position, skip this entity
        }
        
        // Calculate distance
        float dx = otherX - currentX;
        float dy = otherY - currentY;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // Check if within alert range
        if (distance >= config.alertStateStartRadius && distance <= config.alertStateEndRadius) {
            foundTriggerEntity = true;
            
            // Check if this is the nearest trigger entity
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestTriggerEntity = otherEntity.instanceName;
            }
        }
    }
      // Update alert state
    bool wasInAlertState = entity.isInAlertState;
    entity.isInAlertState = foundTriggerEntity;
    
    // Debug: Always log alert state changes
    if (wasInAlertState != entity.isInAlertState) {
        std::cout << "DEBUG: Alert state changed for " << entity.instanceName 
                  << " - was: " << wasInAlertState << ", now: " << entity.isInAlertState << std::endl;
    }
    
    if (foundTriggerEntity) {
        // Enter or continue alert state
        entity.alertTargetEntityName = nearestTriggerEntity;
        entity.alertTargetDistance = nearestDistance;
        
        if (!wasInAlertState) {
            std::cout << "Entity " << entity.instanceName << " entering alert state - triggered by " 
                      << nearestTriggerEntity << " at distance " << nearestDistance << std::endl;
            
            // Stop current movement when entering alert state
            entitiesManager.stopEntityMovement(entity.instanceName);
        }
        
        // Face the trigger entity
        std::string triggerElementName = EntitiesManager::getElementName(nearestTriggerEntity);
        float triggerX, triggerY;
        if (elementsManager.getElementPosition(triggerElementName, triggerX, triggerY)) {
            // Calculate direction to trigger entity
            float dx = triggerX - currentX;
            float dy = triggerY - currentY;
            
            // Determine sprite phase based on direction using improved 4-direction logic
            int spritePhase = config.defaultSpriteSheetPhase;
            if (dx != 0.0f || dy != 0.0f) {
                float angle = std::atan2(dy, dx) * 180.0f / M_PI;
                if (angle < 0) angle += 360.0f;
                
                if (angle >= 315.0f || angle < 45.0f) {
                    spritePhase = config.spritePhaseWalkRight;  // East (0°)
                } else if (angle >= 45.0f && angle < 135.0f) {
                    spritePhase = config.spritePhaseWalkDown;   // South (90°)
                } else if (angle >= 135.0f && angle < 225.0f) {
                    spritePhase = config.spritePhaseWalkLeft;   // West (180°)
                } else {
                    spritePhase = config.spritePhaseWalkUp;     // North (270°)
                }
                
                // Update sprite to face the trigger entity
                elementsManager.changeElementSpritePhase(elementName, spritePhase);
                
                std::cout << "Entity " << entity.instanceName << " facing " << nearestTriggerEntity 
                          << " - angle: " << angle << "° -> sprite phase: " << spritePhase << std::endl;
            }
        }
    } else if (wasInAlertState) {
        // Exit alert state
        std::cout << "Entity " << entity.instanceName << " exiting alert state - no trigger entities in range" << std::endl;
        entity.alertTargetEntityName = "";
        entity.alertTargetDistance = 0.0f;
        
    }
}

void EntityBehaviorManager::updateFleeStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config) {
    // Get current entity position
    std::string elementName = EntitiesManager::getElementName(entity.instanceName);
    float currentX, currentY;
    if (!elementsManager.getElementPosition(elementName, currentX, currentY)) {
        return; // Can't get position, skip flee behavior
    }
    
    // Update flee state timer
    entity.fleeStateTimer += deltaTime;
    
    // Find the nearest threat entity within the flee range
    std::string nearestThreatEntity = "";
    float nearestThreatDistance = std::numeric_limits<float>::max();
    bool foundThreatEntity = false;
    
    // Check all entities to find threat entities
    const auto& allEntities = entitiesManager.getEntities();
    for (const auto& pair : allEntities) {
        const Entity& otherEntity = pair.second;
        
        // Skip self
        if (otherEntity.instanceName == entity.instanceName) {
            continue;
        }
        
        // Check if this entity type is in the flee trigger list
        bool isThreatEntity = false;
        for (const EntityName& threatType : config.fleeStateEntitiesList) {
            if (otherEntity.type == threatType) {
                isThreatEntity = true;
                break;
            }
        }
        
        if (!isThreatEntity) {
            continue;
        }
        
        // Get other entity position
        std::string otherElementName = EntitiesManager::getElementName(otherEntity.instanceName);
        float otherX, otherY;
        if (!elementsManager.getElementPosition(otherElementName, otherX, otherY)) {
            continue; // Can't get position, skip this entity
        }
        
        // Calculate distance
        float dx = otherX - currentX;
        float dy = otherY - currentY;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // Check if within flee range
        if (distance >= config.fleeStateStartRadius && distance <= config.fleeStateEndRadius) {
            foundThreatEntity = true;
            
            // Check if this is the nearest threat entity
            if (distance < nearestThreatDistance) {
                nearestThreatDistance = distance;
                nearestThreatEntity = otherEntity.instanceName;
            }
        }
    }
    
    // Update flee state
    bool wasInFleeState = entity.isInFleeState;
    entity.isInFleeState = foundThreatEntity;
    
    // Debug: Log flee state changes
    if (wasInFleeState != entity.isInFleeState) {
        std::cout << "DEBUG: Flee state changed for " << entity.instanceName 
                  << " - was: " << wasInFleeState << ", now: " << entity.isInFleeState << std::endl;
    }
    
    if (foundThreatEntity) {
        // Enter or continue flee state
        entity.fleeTargetEntityName = nearestThreatEntity;
        entity.fleeTargetDistance = nearestThreatDistance;
        
        if (!wasInFleeState) {
            std::cout << "Entity " << entity.instanceName << " entering flee state - threatened by " 
                      << nearestThreatEntity << " at distance " << nearestThreatDistance << std::endl;
            
            // Stop current movement when entering flee state
            entitiesManager.stopEntityMovement(entity.instanceName);
            
            // Reset flee timer to force immediate pathfinding
            entity.fleeStateTimer = 0.5;
        }
        
        // Recalculate flee path every 0.5 seconds or when first entering flee state
        if (entity.fleeStateTimer >= 0.5) {
            entity.fleeStateTimer = 0.0; // Reset timer
            
            // Get threat entity position for safe point calculation
            std::string threatElementName = EntitiesManager::getElementName(nearestThreatEntity);
            float threatX, threatY;
            if (elementsManager.getElementPosition(threatElementName, threatX, threatY)) {
                // Calculate direction away from threat
                float dx = currentX - threatX;
                float dy = currentY - threatY;
                float distance = std::sqrt(dx * dx + dy * dy);
                
                if (distance > 0.0f) {
                    // Normalize direction vector
                    dx /= distance;
                    dy /= distance;
                    
                    // Calculate target flee distance (between min and max)
                    float targetFleeDistance = config.fleeStateMinDistance;
                    if (nearestThreatDistance < config.fleeStateMinDistance) {
                        // If too close, flee to minimum safe distance
                        targetFleeDistance = config.fleeStateMinDistance;
                    } else {
                        // If within acceptable range, flee to maximum distance
                        targetFleeDistance = config.fleeStateMaxDistance;
                    }
                    
                    // Calculate safe point
                    float safeX = currentX + dx * targetFleeDistance;
                    float safeY = currentY + dy * targetFleeDistance;
                    
                    std::cout << "Entity " << entity.instanceName << " fleeing from " << nearestThreatEntity 
                              << " - moving to safe point (" << safeX << ", " << safeY << ") at distance " 
                              << targetFleeDistance << std::endl;
                      // Move to safe point using appropriate walk type
                    WalkType walkType = config.fleeStateRunning ? WalkType::SPRINT : WalkType::NORMAL;
                    entitiesManager.walkEntityWithPathfinding(
                        entity.instanceName, 
                        safeX, 
                        safeY, 
                        walkType
                    );
                }
            }
        }
    } else if (wasInFleeState) {
        // Exit flee state
        std::cout << "Entity " << entity.instanceName << " exiting flee state - no threat entities in range" << std::endl;
        entity.fleeTargetEntityName = "";
        entity.fleeTargetDistance = 0.0f;
        entity.fleeStateTimer = 0.0;
        
        // Stop fleeing movement when exiting flee state
        // entitiesManager.stopEntityMovement(entity.instanceName);
    }
}
