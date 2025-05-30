#include "entityBehaviors.h"
#include "entities.h"
#include "elementsOnMap.h" // For global elementsManager
#include "collision.h" // For collision functions
#include <iostream>
#include <random>
#include <chrono>
#include <limits> // For numeric_limits
#include <cmath> // For sqrt, atan2
#include "enumDefinitions.h"

// Forward declaration
bool findAccessibleFleePoint(const std::string& entityInstanceName, EntitiesManager& entitiesManager,
                            float currentX, float currentY, float threatX, float threatY,
                            float minDistance, float maxDistance,
                            float idealX, float idealY, float& resultX, float& resultY);

// Helper function to check if a position is accessible for an entity in flee state
bool isFleePositionAccessible(const std::string& entityInstanceName, EntitiesManager& entitiesManager,
                             float x, float y) {
    // Get the entity and its configuration
    Entity* entity = entitiesManager.getEntity(entityInstanceName);
    if (!entity) {
        return false;
    }
    
    const EntityConfiguration* config = entitiesManager.getConfiguration(entity->type);
    if (!config) {
        return false;
    }
      // Check map boundaries
    if (config->offMapCollision && wouldEntityCollideWithMapBounds(*config, x, y)) {
        return false;
    }
    
    // Check collision with elements and blocks
    if (config->canCollide && 
        (wouldEntityCollideWithElementsGranular(*config, x, y, true) || 
         wouldEntityCollideWithBlocksGranular(*config, x, y, true))) {
        return false;
    }
    
    return true;
}

// Intelligent function to find an accessible flee point when facing dead ends
bool findAccessibleFleePoint(const std::string& entityInstanceName, EntitiesManager& entitiesManager,
                            float currentX, float currentY, float threatX, float threatY,
                            float minDistance, float maxDistance,
                            float idealX, float idealY, float& resultX, float& resultY) {
    
    // Strategy 1: Try the ideal position first (original behavior)
    if (isFleePositionAccessible(entityInstanceName, entitiesManager, idealX, idealY)) {
        // Check distance from threat to ensure it's safe
        float distanceFromThreat = std::sqrt((idealX - threatX) * (idealX - threatX) + 
                                           (idealY - threatY) * (idealY - threatY));
        if (distanceFromThreat >= minDistance) {
            resultX = idealX;
            resultY = idealY;
            return true;
        }
    }
    
    // Strategy 2: Try alternative directions if ideal point is blocked
    // Calculate direction away from threat
    float dx = currentX - threatX;
    float dy = currentY - threatY;
    float threatDistance = std::sqrt(dx * dx + dy * dy);
    
    if (threatDistance > 0.0f) {
        dx /= threatDistance;
        dy /= threatDistance;
        
        // Try different angles around the ideal direction
        const float angleStep = 30.0f * M_PI / 180.0f; // 30 degrees in radians
        const int maxAngles = 12; // Try up to 360 degrees
        
        for (int i = 1; i <= maxAngles; i++) {
            // Alternate between positive and negative angles
            float angle = (i % 2 == 1) ? (i / 2) * angleStep : -(i / 2) * angleStep;
            
            // Rotate the direction vector
            float rotatedDx = dx * std::cos(angle) - dy * std::sin(angle);
            float rotatedDy = dx * std::sin(angle) + dy * std::cos(angle);
            
            // Try different distances within the flee range
            for (float testDistance = minDistance; testDistance <= maxDistance; testDistance += 1.0f) {
                float testX = currentX + rotatedDx * testDistance;
                float testY = currentY + rotatedDy * testDistance;
                
                if (isFleePositionAccessible(entityInstanceName, entitiesManager, testX, testY)) {
                    // Verify this position maintains safe distance from threat
                    float distanceFromThreat = std::sqrt((testX - threatX) * (testX - threatX) + 
                                                       (testY - threatY) * (testY - threatY));
                    if (distanceFromThreat >= minDistance) {
                        resultX = testX;
                        resultY = testY;
                        std::cout << "Found alternative flee direction for " << entityInstanceName 
                                  << " at angle " << (angle * 180.0f / M_PI) << " degrees" << std::endl;
                        return true;
                    }
                }
            }
        }
    }
    
    // Strategy 3: If no good direction away from threat, find ANY accessible position in flee zone
    // Use spiral search pattern around current position
    const float searchStep = 1.0f;
    const float maxSearchRadius = maxDistance;
    
    for (float radius = 1.0f; radius <= maxSearchRadius; radius += searchStep) {
        const int numDirections = std::max(8, static_cast<int>(radius * 4));
        
        for (int i = 0; i < numDirections; i++) {
            float angle = (i * 2.0f * M_PI) / numDirections;
            float testX = currentX + radius * std::cos(angle);
            float testY = currentY + radius * std::sin(angle);
            
            if (isFleePositionAccessible(entityInstanceName, entitiesManager, testX, testY)) {
                // Check if this position is far enough from threat
                float distanceFromThreat = std::sqrt((testX - threatX) * (testX - threatX) + 
                                                   (testY - threatY) * (testY - threatY));
                
                // Accept any position that's accessible, even if not optimal distance from threat
                // This handles cases where the entity is truly trapped
                if (distanceFromThreat >= minDistance * 0.5f) { // Relaxed distance requirement
                    resultX = testX;
                    resultY = testY;
                    std::cout << "Found emergency flee position for " << entityInstanceName 
                              << " at distance " << radius << " from current position" << std::endl;
                    return true;
                }
            }
        }
    }
    
    // Strategy 4: Last resort - try to move to any nearby accessible position
    // This handles extreme cases where entity is completely trapped
    for (float radius = 0.5f; radius <= 3.0f; radius += 0.5f) {
        const int numDirections = 8;
        
        for (int i = 0; i < numDirections; i++) {
            float angle = (i * 2.0f * M_PI) / numDirections;
            float testX = currentX + radius * std::cos(angle);
            float testY = currentY + radius * std::sin(angle);
            
            if (isFleePositionAccessible(entityInstanceName, entitiesManager, testX, testY)) {
                resultX = testX;
                resultY = testY;
                std::cout << "Found last-resort position for trapped entity " << entityInstanceName 
                          << " at distance " << radius << " from current position" << std::endl;
                return true;
            }
        }
    }
    
    // If we get here, the entity is completely trapped
    std::cout << "WARNING: Entity " << entityInstanceName << " is completely trapped with no accessible flee positions!" << std::endl;
    return false;
}


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

    // ATTACK STATE - high priority behavior that can interrupt alert and passive states (but not flee state)
    if (config->attackState && !entity.isInFleeState) {
        updateAttackStateBehavior(entity, deltaTime, entitiesManager, *config);
    }

    // ALERT STATE - priority behavior that can interrupt passive state (but not flee or attack state)
    if (config->alertState && !entity.isInFleeState && !entity.isInAttackState) {
        updateAlertStateBehavior(entity, deltaTime, entitiesManager, *config);
    }

    // PASSIVE STATE - only triggered when not in alert, flee, or attack state
    if (config->passiveState && !entity.isInAlertState && !entity.isInFleeState && !entity.isInAttackState) {
        updatePassiveStateBehavior(entity, deltaTime, entitiesManager, *config);
    } else if (config->passiveState && (entity.isInAlertState || entity.isInFleeState || entity.isInAttackState)) {
        // Debug: Log when passive state is skipped due to higher priority states
        /* std::cout << "DEBUG: Skipping passive state for " << entity.instanceName 
                  << " because isInAlertState = " << entity.isInAlertState 
                  << ", isInFleeState = " << entity.isInFleeState 
                  << ", isInAttackState = " << entity.isInAttackState << std::endl; */
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
        for (const EntityName& threatType : config.fleeStateTriggerEntitiesList) {
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
                      // Calculate ideal safe point away from threat
                    float idealSafeX = currentX + dx * targetFleeDistance;
                    float idealSafeY = currentY + dy * targetFleeDistance;
                    
                    // Find the best accessible safe point using intelligent dead-end handling
                    float finalSafeX, finalSafeY;
                    if (findAccessibleFleePoint(entity.instanceName, entitiesManager, 
                                               currentX, currentY, threatX, threatY,
                                               config.fleeStateMinDistance, config.fleeStateMaxDistance,
                                               idealSafeX, idealSafeY, finalSafeX, finalSafeY)) {
                        
                        std::cout << "Entity " << entity.instanceName << " fleeing from " << nearestThreatEntity 
                                  << " - moving to accessible safe point (" << finalSafeX << ", " << finalSafeY 
                                  << ") at distance " << std::sqrt((finalSafeX-currentX)*(finalSafeX-currentX) + (finalSafeY-currentY)*(finalSafeY-currentY)) << std::endl;
                          // Move to safe point using appropriate walk type
                        WalkType walkType = config.fleeStateRunning ? WalkType::SPRINT : WalkType::NORMAL;
                        entitiesManager.walkEntityWithPathfinding(
                            entity.instanceName, 
                            finalSafeX, 
                            finalSafeY, 
                            walkType
                        );
                    } else {
                        std::cout << "Entity " << entity.instanceName << " is trapped - no accessible flee destination found!" << std::endl;
                        // Entity is truly trapped, just try to move to any nearby safe spot
                        // This will be handled by the pathfinding system's fallback mechanisms
                    }
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

void EntityBehaviorManager::updateAttackStateBehavior(Entity& entity, double deltaTime, EntitiesManager& entitiesManager, const EntityConfiguration& config) {
    // Get current entity position
    std::string elementName = EntitiesManager::getElementName(entity.instanceName);
    float currentX, currentY;
    if (!elementsManager.getElementPosition(elementName, currentX, currentY)) {
        return; // Can't get position, skip attack behavior
    }
    
    // Update attack state timer
    entity.attackStateTimer += deltaTime;
    
    // Update wait timer if waiting before charge
    if (entity.isWaitingBeforeCharge) {
        entity.attackStateWaitTimer += deltaTime;
        
        // Check if wait time is over
        if (entity.attackStateWaitTimer >= entity.nextChargeTime) {
            entity.isWaitingBeforeCharge = false;
            entity.attackStateWaitTimer = 0.0;
            std::cout << "Entity " << entity.instanceName << " finished waiting - ready to charge again" << std::endl;
        }
    }
    
    // Find the nearest target entity within the attack range
    std::string nearestTargetEntity = "";
    float nearestTargetDistance = std::numeric_limits<float>::max();
    bool foundTargetEntity = false;
    
    // Check all entities to find target entities
    const auto& allEntities = entitiesManager.getEntities();
    for (const auto& pair : allEntities) {
        const Entity& otherEntity = pair.second;
        
        // Skip self
        if (otherEntity.instanceName == entity.instanceName) {
            continue;
        }
        
        // Check if this entity type is in the attack trigger list
        bool isTargetEntity = false;
        for (const EntityName& targetType : config.attackStateTriggerEntitiesList) {
            if (otherEntity.type == targetType) {
                isTargetEntity = true;
                break;
            }
        }
          if (!isTargetEntity) {
            continue;
        }
        
        // Get other entity position
        std::string otherElementName = EntitiesManager::getElementName(otherEntity.instanceName);
        float otherX, otherY;
        if (!elementsManager.getElementPosition(otherElementName, otherX, otherY)) {
            continue; // Can't get position, skip this entity
        }
        
        // Calculate distance between collision boundaries instead of centers
        float distance = calculateDistanceBetweenEntityCollisionBoundaries(
            entity.instanceName, currentX, currentY,
            otherEntity.instanceName, otherX, otherY,
            entitiesManager
        );
        
        // Check if within attack range
        if (distance >= config.attackStateStartRadius && distance <= config.attackStateEndRadius) {
            foundTargetEntity = true;
            
            // Check if this is the nearest target entity
            if (distance < nearestTargetDistance) {
                nearestTargetDistance = distance;
                nearestTargetEntity = otherEntity.instanceName;
            }
        }
    }
    
    // Update attack state
    bool wasInAttackState = entity.isInAttackState;
    entity.isInAttackState = foundTargetEntity;
    
    // Debug: Log attack state changes
    if (wasInAttackState != entity.isInAttackState) {
        std::cout << "DEBUG: Attack state changed for " << entity.instanceName 
                  << " - was: " << wasInAttackState << ", now: " << entity.isInAttackState << std::endl;
    }
    
    if (foundTargetEntity) {
        // Enter or continue attack state
        entity.attackTargetEntityName = nearestTargetEntity;
        entity.attackTargetDistance = nearestTargetDistance;
        
        if (!wasInAttackState) {
            std::cout << "Entity " << entity.instanceName << " entering attack state - targeting " 
                      << nearestTargetEntity << " at distance " << nearestTargetDistance << std::endl;
            
            // Stop current movement when entering attack state
            entitiesManager.stopEntityMovement(entity.instanceName);
            
            // Reset attack timer to force immediate pathfinding
            entity.attackStateTimer = 0.5;
        }
        
        // Only attack if not waiting before charge
        if (!entity.isWaitingBeforeCharge) {
            // Get target entity position for attack calculation
            std::string targetElementName = EntitiesManager::getElementName(nearestTargetEntity);
            float targetX, targetY;
            if (elementsManager.getElementPosition(targetElementName, targetX, targetY)) {                // Check if entity is touching the target's collision boundary
                const float touchingDistance = 0.1f; // Consider "touching" when collision boundaries are within 0.1 unit
                
                if (nearestTargetDistance <= touchingDistance) {
                    // Entity is touching the target - start waiting before next charge
                    if (!entity.isWaitingBeforeCharge) {
                        std::cout << "Entity " << entity.instanceName << " reached target " 
                                  << nearestTargetEntity << " - starting wait period" << std::endl;
                        
                        // Generate random wait time between min and max
                        static std::random_device rd;
                        static std::mt19937 gen(rd());
                        std::uniform_real_distribution<float> waitDist(
                            config.attackStateWaitBeforeChargeMin, 
                            config.attackStateWaitBeforeChargeMax
                        );
                        
                        entity.isWaitingBeforeCharge = true;
                        entity.attackStateWaitTimer = 0.0;
                        entity.nextChargeTime = waitDist(gen);
                        
                        std::cout << "Entity " << entity.instanceName << " will wait " 
                                  << entity.nextChargeTime << " seconds before charging again" << std::endl;
                        
                        // Stop movement during wait period
                        entitiesManager.stopEntityMovement(entity.instanceName);
                    }
                } else {
                    // Entity is not touching target - charge towards it
                    // Recalculate attack path every 0.5 seconds or when first entering attack state
                    if (entity.attackStateTimer >= 0.5) {
                        entity.attackStateTimer = 0.0; // Reset timer
                        
                        // Determine walk type based on configuration
                        WalkType walkType = config.attackStateRunning ? WalkType::SPRINT : WalkType::NORMAL;
                        
                        std::cout << "Entity " << entity.instanceName << " charging towards " 
                                  << nearestTargetEntity << " at (" << targetX << ", " << targetY 
                                  << ") - distance: " << nearestTargetDistance << std::endl;
                        
                        // Walk directly towards the target entity
                        entitiesManager.walkEntityWithPathfinding(entity.instanceName, targetX, targetY, walkType);
                    }
                }
            }
        }
    } else if (wasInAttackState) {
        // Exit attack state
        std::cout << "Entity " << entity.instanceName << " exiting attack state - no target entities in range" << std::endl;
        entity.attackTargetEntityName = "";
        entity.attackTargetDistance = 0.0f;
        entity.attackStateTimer = 0.0;
        entity.isWaitingBeforeCharge = false;
        entity.attackStateWaitTimer = 0.0;
        entity.nextChargeTime = 0.0;
        
        // Stop attacking movement when exiting attack state
        // entitiesManager.stopEntityMovement(entity.instanceName);
    }
}

// Helper function to calculate distance between collision boundaries of two entities
float calculateDistanceBetweenEntityCollisionBoundaries(
    const std::string& entityInstanceName1, float x1, float y1,
    const std::string& entityInstanceName2, float x2, float y2,
    EntitiesManager& entitiesManager
) {
    // Get entity configurations to access collision shapes
    Entity* entity1 = entitiesManager.getEntity(entityInstanceName1);
    Entity* entity2 = entitiesManager.getEntity(entityInstanceName2);
    
    if (!entity1 || !entity2) {
        // Fallback to center-to-center distance if entities not found
        float dx = x2 - x1;
        float dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    const EntityConfiguration* config1 = entitiesManager.getConfiguration(entity1->type);
    const EntityConfiguration* config2 = entitiesManager.getConfiguration(entity2->type);
    
    if (!config1 || !config2 || config1->collisionShapePoints.empty() || config2->collisionShapePoints.empty()) {
        // Fallback to center-to-center distance if no collision shapes
        float dx = x2 - x1;
        float dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    // Transform collision shapes to world coordinates
    std::vector<std::pair<float, float>> worldShape1, worldShape2;
    
    // Entity 1 collision shape (no rotation for entities currently)
    for (const auto& point : config1->collisionShapePoints) {
        worldShape1.push_back({x1 + point.first, y1 + point.second});
    }
    
    // Entity 2 collision shape (no rotation for entities currently)
    for (const auto& point : config2->collisionShapePoints) {
        worldShape2.push_back({x2 + point.first, y2 + point.second});
    }
    
    // Calculate minimum distance between the two polygons
    float minDistance = std::numeric_limits<float>::max();
    
    // Check distance from each point in shape1 to each edge in shape2
    for (const auto& point1 : worldShape1) {
        for (size_t i = 0; i < worldShape2.size(); ++i) {
            size_t nextI = (i + 1) % worldShape2.size();
            const auto& edgeStart = worldShape2[i];
            const auto& edgeEnd = worldShape2[nextI];
            
            // Calculate distance from point to line segment
            float distance = pointToLineSegmentDistance(point1.first, point1.second,
                                                       edgeStart.first, edgeStart.second,
                                                       edgeEnd.first, edgeEnd.second);
            minDistance = std::min(minDistance, distance);
        }
    }
    
    // Check distance from each point in shape2 to each edge in shape1
    for (const auto& point2 : worldShape2) {
        for (size_t i = 0; i < worldShape1.size(); ++i) {
            size_t nextI = (i + 1) % worldShape1.size();
            const auto& edgeStart = worldShape1[i];
            const auto& edgeEnd = worldShape1[nextI];
            
            // Calculate distance from point to line segment
            float distance = pointToLineSegmentDistance(point2.first, point2.second,
                                                       edgeStart.first, edgeStart.second,
                                                       edgeEnd.first, edgeEnd.second);
            minDistance = std::min(minDistance, distance);
        }
    }
    
    // If polygons are overlapping, distance should be 0
    if (polygonPolygonCollision(worldShape1, worldShape2)) {
        return 0.0f;
    }
    
    return minDistance;
}

// Helper function to calculate distance from a point to a line segment
float pointToLineSegmentDistance(float px, float py, float x1, float y1, float x2, float y2) {
    // Vector from line start to point
    float dx = px - x1;
    float dy = py - y1;
    
    // Vector from line start to line end
    float ldx = x2 - x1;
    float ldy = y2 - y1;
    
    // Length squared of the line segment
    float lineLength2 = ldx * ldx + ldy * ldy;
    
    if (lineLength2 == 0.0f) {
        // Line segment is actually a point
        return std::sqrt(dx * dx + dy * dy);
    }
    
    // Project point onto line (parameter t)
    float t = (dx * ldx + dy * ldy) / lineLength2;
    
    // Clamp t to line segment
    t = std::max(0.0f, std::min(1.0f, t));
    
    // Find closest point on line segment
    float closestX = x1 + t * ldx;
    float closestY = y1 + t * ldy;
    
    // Return distance from point to closest point on line segment
    float distX = px - closestX;
    float distY = py - closestY;
    return std::sqrt(distX * distX + distY * distY);
}
