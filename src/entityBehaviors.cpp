#include "entityBehaviors.h"
#include "entities.h"
#include "entityNameOps.h" // For entityNameToString function
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
    
    // PRIORITY SYSTEM: Higher priority states override lower priority ones
    // 1. Flee state (highest priority - immediate danger response)
    if (config->fleeState) {
        updateFleeStateBehavior(entity, deltaTime, entitiesManager, *config);
        
        // If entity is in flee state, skip other behaviors
        if (entity.isInFleeState) {
            return;
        }
    }
    
    // 2. Alert state (medium priority - watch for threats)
    if (config->alertState) {
        updateAlertStateBehavior(entity, deltaTime, entitiesManager, *config);
        
        // If entity is in alert state, skip other behaviors
        if (entity.isInAlertState) {
            return;
        }
    }
    
    // 3. Passive state behavior (lowest priority - only if not in alert or flee)
    if (config->passiveState && !entity.isInAlertState && !entity.isInFleeState) {
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
            std::cout << " for entity types: ";
            for (size_t i = 0; i < config.alertStateEntitiesList.size(); ++i) {
                std::cout << entityNameToString(config.alertStateEntitiesList[i]);
                if (i < config.alertStateEntitiesList.size() - 1) std::cout << ", ";
            }
        } else {
            std::cout << " for all entity types";}
        std::cout << std::endl;
    }
    
    // Initialize flee state behavior
    if (config.fleeState) {
        entity.isInFleeState = false;
        entity.fleeTargetEntity.clear();
        entity.fleeTargetX = 0.0f;
        entity.fleeTargetY = 0.0f;
        entity.fleeStateStartTime = 0.0;
        entity.fleeTargetDistance = 0.0f;
        
        std::cout << "Initialized flee behavior for entity " << entity.instanceName 
                  << " - flee radius " << config.fleeStateStartRadius << " to " << config.fleeStateEndRadius
                  << " - distance " << config.fleeStateMinDistance << " to " << config.fleeStateMaxDistance
                  << " - " << (config.fleeStateRunning ? "running" : "walking");
          if (!config.fleeStateEntitiesList.empty()) {
            std::cout << " from entity types: ";
            for (size_t i = 0; i < config.fleeStateEntitiesList.size(); ++i) {
                std::cout << entityNameToString(config.fleeStateEntitiesList[i]);
                if (i < config.fleeStateEntitiesList.size() - 1) std::cout << ", ";
            }
        } else {
            std::cout << " from all entity types";
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
            for (const EntityName& watchedEntityType : config.alertStateEntitiesList) {
                if (otherEntity.type == watchedEntityType) {
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

void EntityBehaviorManager::updateFleeStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config) {
    // CRASH FIX: Validate entity state before processing
    try {
        // Validate entity instance name
        if (entity.instanceName.empty()) {
            std::cerr << "ERROR: Empty entity instance name in flee behavior" << std::endl;
            return;
        }
        
        // Check if entity still exists in elements manager
        std::string elementName = EntitiesManager::getElementName(entity.instanceName);
        if (!elementsManager.elementExists(elementName)) {
            std::cerr << "ERROR: Entity " << entity.instanceName << " no longer exists during flee behavior" << std::endl;
            return;
        }
        
        // Check if entity should enter flee state
        if (!entity.isInFleeState) {
            if (checkForFleeTriggers(entity, entitiesManager, config)) {
                // Entity just entered flee state - stop current movement and start fleeing
                if (entity.isWalking) {
                    entity.isWalking = false;
                    entity.path.clear();
                    entity.currentPathIndex = 0;
                    // Stop animation temporarily
                    elementsManager.changeElementAnimationStatus(elementName, false);
                    elementsManager.changeElementSpriteFrame(elementName, 0);
                    
                    std::cout << "Entity " << entity.instanceName << " entered flee state - stopping current movement" << std::endl;
                }
                
                // Calculate flee destination and start moving
                float fleeX, fleeY;
            if (calculateFleeDestination(entity, entity.fleeTargetX, entity.fleeTargetY, config, fleeX, fleeY)) {
                // Start fleeing to calculated destination
                WalkType walkType = config.fleeStateRunning ? WalkType::SPRINT : WalkType::NORMAL;
                entitiesManager.walkEntityWithPathfinding(entity.instanceName, fleeX, fleeY, walkType);
                
                std::cout << "Entity " << entity.instanceName << " fleeing to (" << fleeX << ", " << fleeY 
                          << ") distance " << entity.fleeTargetDistance << " with " 
                          << (config.fleeStateRunning ? "sprint" : "normal") << " speed" << std::endl;
            }
        }
    } else {
        // Entity is already in flee state - check if threat is still present
        bool threatStillPresent = false;
        
        // Get current entity position
        std::string elementName = EntitiesManager::getElementName(entity.instanceName);
        float entityX, entityY;
        if (elementsManager.getElementPosition(elementName, entityX, entityY)) {
            
            // Check if flee target entity still exists and is still a threat
            if (!entity.fleeTargetEntity.empty()) {
                std::string targetElementName = EntitiesManager::getElementName(entity.fleeTargetEntity);
                float targetX, targetY;
                
                if (elementsManager.getElementPosition(targetElementName, targetX, targetY)) {
                    float distance = std::sqrt((targetX - entityX) * (targetX - entityX) + (targetY - entityY) * (targetY - entityY));
                    
                    // Threat is still present if within flee detection range
                    if (distance >= config.fleeStateStartRadius && distance <= config.fleeStateEndRadius) {
                        threatStillPresent = true;
                        
                        // Update target position
                        entity.fleeTargetX = targetX;
                        entity.fleeTargetY = targetY;
                        
                        // If entity finished fleeing but threat is still there, flee again
                        if (!entity.isWalking && !entity.isWaitingForPath) {
                            float newFleeX, newFleeY;
                            if (calculateFleeDestination(entity, targetX, targetY, config, newFleeX, newFleeY)) {
                                WalkType walkType = config.fleeStateRunning ? WalkType::SPRINT : WalkType::NORMAL;
                                entitiesManager.walkEntityWithPathfinding(entity.instanceName, newFleeX, newFleeY, walkType);
                                
                                std::cout << "Entity " << entity.instanceName << " continues fleeing to (" << newFleeX << ", " << newFleeY 
                                          << ") - threat still present" << std::endl;
                            }
                        }
                    }
                }
            }
        }
        
        // If threat is no longer present, exit flee state
        if (!threatStillPresent) {
            entity.isInFleeState = false;
            entity.fleeTargetEntity.clear();
            entity.fleeTargetX = 0.0f;
            entity.fleeTargetY = 0.0f;
            entity.fleeStateStartTime = 0.0;
            entity.fleeTargetDistance = 0.0f;
              std::cout << "Entity " << entity.instanceName << " exited flee state - threat no longer present" << std::endl;
        }
    }
    
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception in flee behavior for entity " << entity.instanceName 
                  << ": " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "CRITICAL: Unknown exception in flee behavior for entity " << entity.instanceName << std::endl;
    }
}

bool EntityBehaviorManager::checkForFleeTriggers(Entity& entity, EntitiesManager& entitiesManager, const EntityConfiguration& config) {
    // CRASH FIX: Validate entity state before processing
    try {
        // Validate entity instance name
        if (entity.instanceName.empty()) {
            std::cerr << "ERROR: Empty entity instance name in flee trigger check" << std::endl;
            return false;
        }
        
        // Get current entity position
        std::string elementName = EntitiesManager::getElementName(entity.instanceName);
        float entityX, entityY;
        if (!elementsManager.getElementPosition(elementName, entityX, entityY)) {
            return false; // Can't get position
        }
        
        // Check all entities in the flee watch list
    const auto& allEntities = entitiesManager.getEntities();
    
    for (const auto& pair : allEntities) {
        const Entity& otherEntity = pair.second;
        
        // Skip self
        if (otherEntity.instanceName == entity.instanceName) {
            continue;
        }
          // Check if this entity is in our flee list (empty list means flee from all)
        bool shouldFleeFrom = config.fleeStateEntitiesList.empty();
        if (!shouldFleeFrom) {
            for (const EntityName& fleeFromEntityType : config.fleeStateEntitiesList) {
                if (otherEntity.type == fleeFromEntityType) {
                    shouldFleeFrom = true;
                    break;
                }
            }
        }
        
        if (!shouldFleeFrom) {
            continue; // Not fleeing from this entity
        }
        
        // Get other entity position
        std::string otherElementName = EntitiesManager::getElementName(otherEntity.instanceName);
        float otherX, otherY;
        if (!elementsManager.getElementPosition(otherElementName, otherX, otherY)) {
            continue; // Can't get other entity position
        }
        
        // Calculate distance
        float distance = std::sqrt((otherX - entityX) * (otherX - entityX) + (otherY - entityY) * (otherY - entityY));
        
        // Check if other entity is within flee range
        if (distance >= config.fleeStateStartRadius && distance <= config.fleeStateEndRadius) {
            // Found an entity in flee range - enter flee state
            entity.isInFleeState = true;
            entity.fleeTargetEntity = otherEntity.instanceName;
            entity.fleeTargetX = otherX;
            entity.fleeTargetY = otherY;            entity.fleeStateStartTime = entity.behaviorTimer;
            
            std::cout << "Entity " << entity.instanceName << " detected threat " << otherEntity.instanceName 
                      << " at distance " << distance << " - entering flee state" << std::endl;
            
            return true;
        }
    }
    
    return false; // No threats found in flee range
    
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception in flee trigger check for entity " << entity.instanceName 
                  << ": " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "CRITICAL: Unknown exception in flee trigger check for entity " << entity.instanceName << std::endl;
        return false;
    }
}

bool EntityBehaviorManager::calculateFleeDestination(Entity& entity, float targetX, float targetY, const EntityConfiguration& config, float& fleeX, float& fleeY) {
    // CRASH FIX: Validate entity state before processing
    try {
        // Validate entity instance name
        if (entity.instanceName.empty()) {
            std::cerr << "ERROR: Empty entity instance name in flee destination calculation" << std::endl;
            return false;
        }
        
        // Get current entity position
        std::string elementName = EntitiesManager::getElementName(entity.instanceName);
        float entityX, entityY;
        if (!elementsManager.getElementPosition(elementName, entityX, entityY)) {
            return false; // Can't get position
        }
        
        // Calculate direction away from target
        float dx = entityX - targetX;  // Opposite direction from alert (away from target)
        float dy = entityY - targetY;
    
    // Normalize direction vector
    float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.001f) {
        // Target is too close, use random direction
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
        float angle = angleDist(gen);
        dx = std::cos(angle);
        dy = std::sin(angle);
    } else {
        dx /= length;
        dy /= length;
    }
    
    // Generate random flee distance
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distanceDist(config.fleeStateMinDistance, config.fleeStateMaxDistance);
    entity.fleeTargetDistance = distanceDist(gen);
    
    // Calculate flee destination
    fleeX = entityX + dx * entity.fleeTargetDistance;
    fleeY = entityY + dy * entity.fleeTargetDistance;
    
    // Try to find a safe accessible destination using the entities manager
    float safeFleeX, safeFleeY;
    if (entitiesManager.findNearestSafePlaceFromCoordinatesForEntity(entity.instanceName, fleeX, fleeY, safeFleeX, safeFleeY)) {
        fleeX = safeFleeX;
        fleeY = safeFleeY;
        
        // Recalculate actual flee distance
        float actualDx = fleeX - entityX;
        float actualDy = fleeY - entityY;
        entity.fleeTargetDistance = std::sqrt(actualDx * actualDx + actualDy * actualDy);
        
        return true;
    }
      std::cout << "Warning: Could not find safe flee destination for entity " << entity.instanceName << std::endl;
    return false;
    
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception in flee destination calculation for entity " << entity.instanceName 
                  << ": " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "CRITICAL: Unknown exception in flee destination calculation for entity " << entity.instanceName << std::endl;
        return false;
    }
}
