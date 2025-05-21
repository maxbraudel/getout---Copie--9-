#include "entities.h"
#include "collision.h"
#include "map.h" // Adding for gameMap access
#include "pathfinding.h"
#include "globals.h" // For GRID_SIZE
#include <iostream>
#include <cmath>

// Global instance definition
EntitiesManager entitiesManager;

// Forward declaration for gameMap access (defined in main.cpp)
extern Map gameMap;

EntitiesManager::EntitiesManager() {
    // Constructor
}

EntitiesManager::~EntitiesManager() {
    // Destructor - nothing special to clean up
}

void EntitiesManager::addConfiguration(const EntityConfiguration& config) {
    // Add or replace the configuration
    configurations[config.typeName] = config;
    std::cout << "Added entity configuration: " << config.typeName << std::endl;
}

bool EntitiesManager::placeEntity(const std::string& instanceName, const std::string& typeName, float x, float y) {
    // Check if the configuration exists
    const EntityConfiguration* config = getConfiguration(typeName);
    if (!config) {
        std::cerr << "Entity configuration not found: " << typeName << std::endl;
        return false;
    }
    
    // Check for a safe starting position if this entity type has collisions enabled
    float safeX = x;
    float safeY = y;
    bool needsSafePosition = false;
    
    if (config->canCollide) {
        // Check if the target position is in a collision area
        if (wouldCollideWithElement(safeX, safeY, config->collisionRadius) || 
            wouldCollideWithMapBlock(safeX, safeY, gameMap)) {
            needsSafePosition = true;
            // Try to find a safe position nearby
            if (!findSafePosition(safeX, safeY, config->collisionRadius, gameMap)) {
                std::cerr << "WARNING: Could not find a safe starting position for entity near (" 
                        << x << ", " << y << "). Proceeding with original position." << std::endl;
                safeX = x;
                safeY = y;
            } else {
                std::cout << "Adjusted entity position from (" << x << ", " << y 
                        << ") to (" << safeX << ", " << safeY << ") to avoid collisions." << std::endl;
            }
        }
    }
    
    // Generate the element name
    std::string elementName = getElementName(instanceName);
    
    // Create an Entity object
    Entity entity;
    entity.instanceName = instanceName;
    entity.typeName = typeName;
    
    // Place the element on the map using ElementsOnMap
    elementsManager.placeElement(
        elementName,
        config->textureName,
        config->scale,
        safeX, safeY,  // Use the potentially adjusted safe position
        0.0f,  // rotation
        config->defaultSpriteSheetPhase,
        config->defaultSpriteSheetFrame,
        false,  // not animated initially
        config->defaultAnimationSpeed,
        AnchorPoint::BOTTOM_CENTER  // Default anchor for entities
    );
    
    // Add the entity to our entities map
    entities[instanceName] = entity;
    
    if (needsSafePosition && safeX != x && safeY != y) {
        std::cout << "Entity " << instanceName << " created at safe position (" << safeX << "," << safeY 
                << ") instead of requested (" << x << "," << y << ")" << std::endl;
    } else {
        std::cout << "Placed entity: " << instanceName << " (type: " << typeName << ") at (" 
                << safeX << ", " << safeY << ")" << std::endl;
    }
    return true;
}

bool EntitiesManager::moveEntity(const std::string& instanceName, float x, float y) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }
    
    // Get the configuration
    const EntityConfiguration* config = getConfiguration(entity->typeName);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entity->typeName << std::endl;
        return false;
    }
    
    // Get the element name
    std::string elementName = getElementName(instanceName);
    
    // Get current position
    float currentX, currentY;
    if (!elementsManager.getElementPosition(elementName, currentX, currentY)) {
        std::cerr << "Error getting position for entity: " << instanceName << std::endl;
        return false;
    }
    
    // Use walkEntityWithPathfinding instead of walkEntityToCoordinates for intelligent movement
    return walkEntityWithPathfinding(instanceName, x, y, entity->walkType);
}

bool EntitiesManager::walkEntityToCoordinates(const std::string& instanceName, float x, float y, WalkType walkType) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }
    
    // Get the configuration
    const EntityConfiguration* config = getConfiguration(entity->typeName);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entity->typeName << std::endl;
        return false;
    }
    
    // Get the element name
    std::string elementName = getElementName(instanceName);
    
    // Set up the walking target
    entity->isWalking = true;
    entity->targetX = x;
    entity->targetY = y;
    entity->walkType = walkType;
    
    // Enable animation
    elementsManager.changeElementAnimationStatus(elementName, true);
    
    // Set the animation speed based on walk type
    float animationSpeed = (walkType == WalkType::NORMAL) ? 
                          config->normalWalkingAnimationSpeed : 
                          config->sprintWalkingAnimationSpeed;
    elementsManager.changeElementAnimationSpeed(elementName, animationSpeed);
    
    std::cout << "Entity " << instanceName << " starting to walk to (" << x << ", " << y << ") with " 
              << ((walkType == WalkType::NORMAL) ? "normal" : "sprint") << " speed" << std::endl;
    return true;
}

bool EntitiesManager::walkEntityWithPathfinding(const std::string& instanceName, float x, float y, WalkType walkType) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }
    
    // Get the configuration
    const EntityConfiguration* config = getConfiguration(entity->typeName);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entity->typeName << std::endl;
        return false;
    }
    
    // Get current position
    std::string elementName = getElementName(instanceName);
    float currentX, currentY;
    if (!elementsManager.getElementPosition(elementName, currentX, currentY)) {
        std::cerr << "Error getting position for entity: " << instanceName << std::endl;
        return false;
    }
    
    // Validate target coordinates are within grid bounds
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE ||
        currentX < 0 || currentX >= GRID_SIZE || currentY < 0 || currentY >= GRID_SIZE) {
        std::cerr << "Invalid movement coordinates. Start (" << currentX << "," << currentY 
                 << ") or target (" << x << "," << y << ") are out of bounds." << std::endl;
        return false;
    }
    
    // Generate a path using A* pathfinding
    std::vector<std::pair<float, float>> path = findPath(
        currentX, 
        currentY, 
        x, 
        y, 
        gameMap,
        config->collisionRadius
    );
    
    // Check if a path was found
    if (path.empty()) {
        std::cout << "No valid path found for entity " << instanceName << " to reach (" 
                  << x << ", " << y << ")" << std::endl;
        return false;
    }
    
    // Store the path in the entity
    entity->path = path;
    entity->currentPathIndex = 0;
    entity->isWalking = true;
    entity->walkType = walkType;
    entity->usePathfinding = true;
    
    // Set the final destination
    entity->targetX = x;
    entity->targetY = y;
    
    // Enable animation
    elementsManager.changeElementAnimationStatus(elementName, true);
    
    // Set the animation speed based on walk type
    float animationSpeed = (walkType == WalkType::NORMAL) ? 
                          config->normalWalkingAnimationSpeed : 
                          config->sprintWalkingAnimationSpeed;
    elementsManager.changeElementAnimationSpeed(elementName, animationSpeed);
    
    std::cout << "Entity " << instanceName << " found path with " << path.size() 
              << " waypoints to (" << x << ", " << y << ")" << std::endl;
    
    return true;
}

bool EntitiesManager::stopEntityWalk(const std::string& instanceName) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }
    
    // Stop walking
    entity->isWalking = false;
    
    // Clear any active path
    if (entity->usePathfinding) {
        entity->path.clear();
        entity->currentPathIndex = 0;
    }
    
    // Get the element name
    std::string elementName = getElementName(instanceName);
    
    // Disable animation
    elementsManager.changeElementAnimationStatus(elementName, false);
    
    // Reset to frame 0 (standing frame)
    elementsManager.changeElementSpriteFrame(elementName, 0);
    
    std::cout << "Entity " << instanceName << " stopped walking" << std::endl;
    return true;
}

bool EntitiesManager::changeEntityWalkingState(const std::string& instanceName, WalkType walkType) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }
    
    // Change the walk type
    entity->walkType = walkType;
    
    // If the entity is currently walking, update the animation speed
    if (entity->isWalking) {
        // Get the configuration
        const EntityConfiguration* config = getConfiguration(entity->typeName);
        if (!config) {
            std::cerr << "Entity configuration not found: " << entity->typeName << std::endl;
            return false;
        }
        
        // Get the element name
        std::string elementName = getElementName(instanceName);
        
        // Set the animation speed based on the new walk type
        float animationSpeed = (walkType == WalkType::NORMAL) ? 
                              config->normalWalkingAnimationSpeed : 
                              config->sprintWalkingAnimationSpeed;
        elementsManager.changeElementAnimationSpeed(elementName, animationSpeed);
    }
    
    std::cout << "Entity " << instanceName << " walk type changed to " 
              << ((walkType == WalkType::NORMAL) ? "normal" : "sprint") << std::endl;
    return true;
}

void EntitiesManager::update(double deltaTime) {
    // First, ensure all entities are not stuck in collision areas
    ensureAllEntitiesNotStuck();
    
    // Update all walking entities
    for (auto& pair : entities) {
        Entity& entity = pair.second;
        
        // Skip entities that are not walking
        if (!entity.isWalking) {
            continue;
        }
        
        // Get the configuration for this entity
        const EntityConfiguration* config = getConfiguration(entity.typeName);
        if (!config) {
            std::cerr << "Error: Cannot find configuration for entity: " << entity.instanceName << std::endl;
            entity.isWalking = false; // Stop walking due to error
            continue;
        }
        
        // Update the entity's walking animation
        updateEntityWalking(entity, *config, deltaTime);
    }
}

// Private helper methods

const EntityConfiguration* EntitiesManager::getConfiguration(const std::string& typeName) const {
    auto it = configurations.find(typeName);
    if (it != configurations.end()) {
        return &(it->second);
    }
    return nullptr;
}

Entity* EntitiesManager::getEntity(const std::string& instanceName) {
    auto it = entities.find(instanceName);
    if (it != entities.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool EntitiesManager::getNextPathWaypoint(Entity& entity, float& nextX, float& nextY) {
    // If the entity has no path or has reached the end of its path
    if (entity.path.empty() || entity.currentPathIndex >= entity.path.size()) {
        return false;
    }
    
    // Get the next waypoint in the path
    const auto& waypoint = entity.path[entity.currentPathIndex];
    nextX = waypoint.first;
    nextY = waypoint.second;
    
    return true;
}

void EntitiesManager::updateEntityWalking(Entity& entity, const EntityConfiguration& config, double deltaTime) {
    // Get the element name
    std::string elementName = getElementName(entity.instanceName);
    
    // Get current position
    float currentX, currentY;
    if (!elementsManager.getElementPosition(elementName, currentX, currentY)) {
        std::cerr << "Error getting position for entity: " << entity.instanceName << std::endl;
        entity.isWalking = false; // Stop walking due to error
        return;
    }
    
    // First, check if the entity is already in a collision state and try to fix it
    // This handles the case where the entity somehow got stuck in a collision area
    float safeX = currentX;
    float safeY = currentY;
    bool wasStuck = false;
    
    // Only check collisions if this entity type can collide
    if (config.canCollide) {
        // Try to find a safe position for the entity if it's currently stuck
        if (wouldCollideWithElement(safeX, safeY, config.collisionRadius) || 
            wouldCollideWithMapBlock(safeX, safeY, gameMap)) {
            wasStuck = findSafePosition(safeX, safeY, config.collisionRadius, gameMap);
            
            if (wasStuck) {
                // Entity was stuck and we found a safe position, teleport it there
                elementsManager.changeElementCoordinates(elementName, safeX, safeY);
                std::cout << "Entity " << entity.instanceName << " was stuck in collision, moved to safe position: (" 
                        << safeX << ", " << safeY << ")" << std::endl;
                
                // Update our local position variable
                currentX = safeX;
                currentY = safeY;
            }
        }
    }
    
    // Variables for movement
    float targetX, targetY;
    float dx, dy, distance;
    
    // Handle pathfinding movement if enabled for this entity
    if (entity.usePathfinding && !entity.path.empty()) {
        // Get the next waypoint from the path
        if (entity.currentPathIndex >= entity.path.size()) {
            // We've reached the end of the path, target the final destination
            targetX = entity.targetX;
            targetY = entity.targetY;
        } else {
            // Get the next waypoint
            const auto& waypoint = entity.path[entity.currentPathIndex];
            targetX = waypoint.first;
            targetY = waypoint.second;
        }
        
        // Calculate distance to current waypoint
        dx = targetX - currentX;
        dy = targetY - currentY;
        distance = std::sqrt(dx * dx + dy * dy);
        
        // Check if we've reached the current waypoint
        float waypointThreshold = 0.1f;
        if (distance <= waypointThreshold && entity.currentPathIndex < entity.path.size()) {
            // Move to the next waypoint
            entity.currentPathIndex++;
            
            // Check if we've reached the end of the path
            if (entity.currentPathIndex >= entity.path.size()) {
                // We've completed all waypoints, now target the exact final destination
                targetX = entity.targetX;
                targetY = entity.targetY;
                // Recalculate distance
                dx = targetX - currentX;
                dy = targetY - currentY;
                distance = std::sqrt(dx * dx + dy * dy);
            } else {
                // Update to the new waypoint
                const auto& newWaypoint = entity.path[entity.currentPathIndex];
                targetX = newWaypoint.first;
                targetY = newWaypoint.second;
                // Recalculate distance
                dx = targetX - currentX;
                dy = targetY - currentY;
                distance = std::sqrt(dx * dx + dy * dy);
            }
        }
    } else {
        // No pathfinding, move directly toward the target
        targetX = entity.targetX;
        targetY = entity.targetY;
        
        // Calculate distance to target
        dx = targetX - currentX;
        dy = targetY - currentY;
        distance = std::sqrt(dx * dx + dy * dy);
    }
    
    // Stop if we're close enough to the target
    float stopThreshold = 0.1f; // Stop when within 0.1 distance units
    if (distance <= stopThreshold && 
        ((!entity.usePathfinding) || 
        (entity.usePathfinding && entity.currentPathIndex >= entity.path.size()))) {
        // We've reached the final target
        entity.isWalking = false;
        
        // Snap to exact target position
        elementsManager.changeElementCoordinates(elementName, entity.targetX, entity.targetY);
        
        // Disable animation
        elementsManager.changeElementAnimationStatus(elementName, false);
        
        // Reset to frame 0 (standing frame)
        elementsManager.changeElementSpriteFrame(elementName, 0);
        
        // Clear the path
        if (entity.usePathfinding) {
            entity.path.clear();
            entity.currentPathIndex = 0;
        }
        
        std::cout << "Entity " << entity.instanceName << " reached target (" 
                  << entity.targetX << ", " << entity.targetY << ")" << std::endl;
        return;
    }
    
    // Calculate movement speed
    float speed = (entity.walkType == WalkType::NORMAL) ? 
                 config.normalWalkingSpeed : 
                 config.sprintWalkingSpeed;
    
    // Calculate movement delta for this frame
    float moveDistance = speed * static_cast<float>(deltaTime);
    
    // If the move distance would exceed the remaining distance, clamp it
    if (moveDistance > distance) {
        moveDistance = distance;
    }
    
    // Normalize direction vector
    float normalizeFactor = moveDistance / distance;
    float moveDx = dx * normalizeFactor;
    float moveDy = dy * normalizeFactor;
      // Determine the primary direction (up, down, left, right)
    if (std::abs(dx) > std::abs(dy)) {
        // Horizontal movement dominates
        entity.lastDirection = (dx > 0) ? 3 : 2;  // 3 = right, 2 = left
    } else {
        // Vertical movement dominates
        entity.lastDirection = (dy > 0) ? 0 : 1;  // 0 = up, 1 = down
    }
    
    // Set the appropriate sprite phase based on direction
    int phase;
    switch (entity.lastDirection) {
        case 0: phase = config.spritePhaseWalkUp; break;
        case 1: phase = config.spritePhaseWalkDown; break;
        case 2: phase = config.spritePhaseWalkLeft; break;
        case 3: phase = config.spritePhaseWalkRight; break;
        default: phase = config.defaultSpriteSheetPhase; break;
    }
    
    // Change the sprite phase
    elementsManager.changeElementSpritePhase(elementName, phase);
    
    // Check for collisions before moving the entity
    bool canMove = true;
    
    if (config.canCollide) {
        // Calculate the new position after movement
        float newX = currentX + moveDx;
        float newY = currentY + moveDy;
        
        // Check if the new position would collide with any collidable element or map block
        bool collisionWithElement = wouldCollideWithElement(newX, newY, config.collisionRadius);
        bool collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap);
        
        if (collisionWithElement || collisionWithMapBlock) {
            // Collision detected, don't move but keep trying to reach the target
            canMove = false;
            
            // Try to find a safe path by testing intermediate positions
            // First try moving only in X direction
            if (!collisionWithElement && !collisionWithMapBlock) {
                moveDx = 0; // Only move in Y
                canMove = true;
            }
            
            // Then try moving only in Y direction
            newX = currentX;
            newY = currentY + moveDy;
            collisionWithElement = wouldCollideWithElement(newX, newY, config.collisionRadius);
            collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap);
            
            if (!collisionWithElement && !collisionWithMapBlock) {
                moveDx = 0; // Only move in Y
                canMove = true;
            } else {
                newX = currentX + moveDx;
                newY = currentY;
                collisionWithElement = wouldCollideWithElement(newX, newY, config.collisionRadius);
                collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap);
                
                if (!collisionWithElement && !collisionWithMapBlock) {
                    moveDy = 0; // Only move in X
                    canMove = true;
                }
            }              if (!canMove) {
                // Entity is stuck, let's try to recalculate the path if using pathfinding
                if (entity.usePathfinding) {
                    std::cout << "Entity " << entity.instanceName << " encountered obstacle at (" 
                              << newX << ", " << newY << "), recalculating path..." << std::endl;
                    
                    // Get current position
                    float curX, curY;
                    if (elementsManager.getElementPosition(elementName, curX, curY)) {
                        // Validate coordinates before pathfinding
                        if (curX < 0 || curX >= GRID_SIZE || curY < 0 || curY >= GRID_SIZE ||
                            entity.targetX < 0 || entity.targetX >= GRID_SIZE || 
                            entity.targetY < 0 || entity.targetY >= GRID_SIZE) {
                            std::cout << "Invalid path recalculation coordinates: from (" 
                                     << curX << "," << curY << ") to (" 
                                     << entity.targetX << "," << entity.targetY << ")" << std::endl;
                            
                            // Stop movement as coordinates are invalid
                            entity.isWalking = false;
                            elementsManager.changeElementAnimationStatus(elementName, false);
                            elementsManager.changeElementSpriteFrame(elementName, 0);
                            return;
                        }
                        
                        // Recalculate the path from the current position
                        std::vector<std::pair<float, float>> newPath = findPath(
                            curX, curY,
                            entity.targetX, entity.targetY,
                            gameMap,
                            config.collisionRadius
                        );
                        
                        // Check if a new path was found
                        if (!newPath.empty()) {
                            entity.path = newPath;
                            entity.currentPathIndex = 0;
                            std::cout << "Found new path with " << newPath.size() << " waypoints" << std::endl;
                        } else {
                            // No path found, stop walking
                            entity.isWalking = false;
                            elementsManager.changeElementAnimationStatus(elementName, false);
                            elementsManager.changeElementSpriteFrame(elementName, 0);
                            std::cout << "No valid path found for entity " << entity.instanceName 
                                      << ", stopping movement" << std::endl;
                        }
                    }
                } else {
                    // Not using pathfinding, just stop and try again next frame
                    std::cout << "Entity " << entity.instanceName << " encountered obstacle at (" 
                              << newX << ", " << newY << ")" << std::endl;
                }
            }
        }
    }
    
    // Move the entity if no collision
    if (canMove && !elementsManager.moveElement(elementName, moveDx, moveDy)) {
        // If movement failed (e.g. due to collision), stop walking
        entity.isWalking = false;
        elementsManager.changeElementAnimationStatus(elementName, false);
        elementsManager.changeElementSpriteFrame(elementName, 0);
        std::cout << "Entity " << entity.instanceName << " stopped walking due to movement failure" << std::endl;
    }
}

// Function to ensure entity is not stuck in any collision areas
bool EntitiesManager::ensureEntityNotStuck(const std::string& instanceName) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }
    
    // Get the configuration for this entity type
    const EntityConfiguration* config = getConfiguration(entity->typeName);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entity->typeName << std::endl;
        return false;
    }
    
    // Skip if entity doesn't have collisions enabled
    if (!config->canCollide) {
        return false;
    }
    
    // Get the element name
    std::string elementName = getElementName(instanceName);
    
    // Get current position
    float x, y;
    if (!elementsManager.getElementPosition(elementName, x, y)) {
        std::cerr << "Error getting position for entity: " << instanceName << std::endl;
        return false;
    }
    
    // Check if entity is in a collision state
    if (wouldCollideWithElement(x, y, config->collisionRadius) || 
        wouldCollideWithMapBlock(x, y, gameMap)) {
        
        // Entity is stuck, try to find a safe position
        if (findSafePosition(x, y, config->collisionRadius, gameMap)) {
            // Found a safe position, move entity there
            elementsManager.changeElementCoordinates(elementName, x, y);
            std::cout << "Safety check: Entity " << instanceName << " was stuck in collision, moved to safe position: (" 
                      << x << ", " << y << ")" << std::endl;
            return true; // Position was adjusted
        } else {
            // Could not find a safe position
            std::cout << "Warning: Entity " << instanceName << " is stuck in collision at (" << x << ", " << y 
                      << ") and no safe position could be found!" << std::endl;
        }
    }
    
    return false; // No adjustment needed or possible
}

// Function to check if all entities are in safe positions, moving them if needed
void EntitiesManager::ensureAllEntitiesNotStuck() {
    // Iterate through all entities and check if they're stuck
    for (auto& pair : entities) {
        ensureEntityNotStuck(pair.first);
    }
}

bool EntitiesManager::teleportEntity(const std::string& instanceName, float x, float y) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }
    
    // Get the configuration
    const EntityConfiguration* config = getConfiguration(entity->typeName);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entity->typeName << std::endl;
        return false;
    }
    
    // Get the element name
    std::string elementName = getElementName(instanceName);
    
    float targetX = x;
    float targetY = y;
    
    // Only check for collisions if this entity type can collide
    if (config->canCollide) {
        // Check for collision at the teleport destination
        bool collisionWithElement = wouldCollideWithElement(targetX, targetY, config->collisionRadius);
        bool collisionWithMapBlock = wouldCollideWithMapBlock(targetX, targetY, gameMap);
        
        if (collisionWithElement || collisionWithMapBlock) {
            // Log the collision type for debugging
            std::cout << "Teleport destination has collision: ";
            if (collisionWithMapBlock) {
                int gridX = static_cast<int>(targetX);
                int gridY = static_cast<int>(targetY);
                TextureName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
                std::cout << "Map block type: " << static_cast<int>(blockType) << " ";
            }
            if (collisionWithElement) {
                std::cout << "Element collision";
            }
            std::cout << std::endl;
            
            // Try to find a safe position near the requested coordinates
            if (findSafePosition(targetX, targetY, config->collisionRadius, gameMap)) {
                // Found a safe position near the requested coordinates
                std::cout << "Adjusted entity teleport position from (" << x << ", " << y << ") to ("
                        << targetX << ", " << targetY << ") to avoid collisions." << std::endl;
            } else {
                // Could not find a safe position
                std::cout << "Cannot teleport entity to (" << x << ", " << y << ") - ";
                if (collisionWithElement) {
                    std::cout << "position is occupied by a collidable element";
                }
                if (collisionWithMapBlock) {
                    std::cout << (collisionWithElement ? " and " : "") << "position has a non-traversable map block";
                }
                std::cout << "." << std::endl;
                return false;
            }
        }
    }
    
    // Directly set entity position
    elementsManager.changeElementCoordinates(elementName, targetX, targetY);
    
    // If this entity was walking, stop it
    if (entity->isWalking) {
        entity->isWalking = false;
        elementsManager.changeElementAnimationStatus(elementName, false);
        elementsManager.changeElementSpriteFrame(elementName, 0);
    }
    
    // Update entity's target position in case it starts walking again
    entity->targetX = targetX;
    entity->targetY = targetY;
    
    std::cout << "Entity " << instanceName << " teleported to (" << targetX << ", " << targetY << ")" << std::endl;
    return true;
}