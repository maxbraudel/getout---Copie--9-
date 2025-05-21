#include "entities.h"
#include "collision.h"
#include "map.h" // Adding for gameMap access
#include "pathfinding.h"
#include "globals.h" // For GRID_SIZE
#include <iostream>
#include <cmath>

// Static vector of predefined entity types
static std::vector<EntityInfo> entityTypes;

// Function to initialize entity types
static void initializeEntityTypes() {
    if (!entityTypes.empty()) {
        return; // Already initialized
    }

    // Antagonist entity configuration
    EntityInfo antagonist;
    antagonist.typeName = "antagonist";
    antagonist.textureName = ElementTextureName::ANTAGONIST1;
    antagonist.scale = 1.5f;
    
    // Default sprite configuration
    antagonist.defaultSpriteSheetPhase = 2;
    antagonist.defaultSpriteSheetFrame = 0;
    antagonist.defaultAnimationSpeed = 11.0f;
    
    // Walking animation phases
    antagonist.spritePhaseWalkUp = 0;
    antagonist.spritePhaseWalkDown = 3;
    antagonist.spritePhaseWalkLeft = 2;
    antagonist.spritePhaseWalkRight = 1;
    
    // Movement speeds
    antagonist.normalWalkingSpeed = 1.5f;
    antagonist.normalWalkingAnimationSpeed = 4.0f;
    antagonist.sprintWalkingSpeed = 10.0f;
    antagonist.sprintWalkingAnimationSpeed = 12.0f;
    
    // Collision settings
    antagonist.canCollide = true;
    antagonist.collisionRadius = 0.4f;
      // Add to the list
    entityTypes.push_back(antagonist);

    
    // Add to the list    
    // Add more entity types here as needed
}

// Global instance definition
EntitiesManager entitiesManager;

// Forward declaration for gameMap access (defined in main.cpp)
extern Map gameMap;

// Static method to get element name from instance name
std::string EntitiesManager::getElementName(const std::string& instanceName) {
    return "entity_" + instanceName;
}

EntitiesManager::EntitiesManager() {
    // Constructor
}

EntitiesManager::~EntitiesManager() {
    // Destructor - nothing special to clean up
}

void EntitiesManager::initializeEntityConfigurations() {
    // Initialize the predefined entity types if they haven't been initialized yet
    initializeEntityTypes();
    
    // Add all predefined entity configurations
    for (const auto& entityInfo : entityTypes) {
        EntityConfiguration config(entityInfo);
        addConfiguration(config);
    }
    std::cout << "Initialized " << entityTypes.size() << " predefined entity configurations" << std::endl;
}

const EntityConfiguration* EntitiesManager::getConfiguration(const std::string& typeName) const {
    auto it = configurations.find(typeName);
    if (it != configurations.end()) {
        return &(it->second);
    }
    return nullptr;
}

void EntitiesManager::addConfiguration(const EntityConfiguration& config) {
    // Add or replace the configuration
    configurations[config.typeName] = config;
    std::cout << "Added entity configuration: " << config.typeName << std::endl;
}

bool EntitiesManager::placeEntityByType(const std::string& instanceName, const std::string& typeName, float x, float y) {
    // Find the entity type in our predefined list
    for (const auto& entityInfo : entityTypes) {
        if (entityInfo.typeName == typeName) {
            // Create configuration from the entity info
            EntityConfiguration config(entityInfo);
            // Add the configuration if it doesn't exist yet
            if (!getConfiguration(typeName)) {
                addConfiguration(config);
            }
            // Place the entity
            return placeEntity(instanceName, typeName, x, y);
        }
    }
    
    std::cerr << "Entity type not found: " << typeName << std::endl;
    return false;
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
        AnchorPoint::USE_TEXTURE_DEFAULT
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
    }      std::cout << "Entity " << instanceName << " walk type changed to " 
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
      // Function to update entity direction and sprite phase - defined at top so it's available throughout the function
    auto updateDirectionAndSprite = [&](float dirX, float dirY) {
        // Only process if there's actual movement
        if (dirX == 0 && dirY == 0) {
            return; // No movement, keep current direction
        }
        
        // Use a smaller threshold to be more responsive to direction changes
        const float directionThreshold = 0.05f;
        
        // Determine the primary direction (up, down, left, right)
        int newDirection;
        if (std::abs(dirX) > std::abs(dirY)) {
            // Horizontal movement dominates
            newDirection = (dirX > 0) ? 3 : 2;  // 3 = right, 2 = left
        } else {
            // Vertical movement dominates or is equal
            newDirection = (dirY > 0) ? 0 : 1;  // 0 = up, 1 = down
        }
        
        // Always update the sprite when moving
        entity.lastDirection = newDirection;
        
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
    };
    
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
        distance = std::sqrt(dx * dx + dy * dy);        // Check if we've reached the current waypoint
        float waypointThreshold = 0.1f;
        if (distance <= waypointThreshold && entity.currentPathIndex < entity.path.size()) {
            // Move to the next waypoint
            entity.currentPathIndex++;
            
            // Check if we've reached the end of the path
            if (entity.currentPathIndex >= entity.path.size()) {
                // We've completed all waypoints, now target the exact final destination
                targetX = entity.targetX;
                targetY = entity.targetY;                // Recalculate distance
                dx = targetX - currentX;
                dy = targetY - currentY;
                distance = std::sqrt(dx * dx + dy * dy);
                
                // Handle the waypoint arrival with the smooth direction transition
                handleWaypointArrival(entity, elementName, config, currentX, currentY);
            } else {
                // Update to the new waypoint
                const auto& newWaypoint = entity.path[entity.currentPathIndex];
                targetX = newWaypoint.first;
                targetY = newWaypoint.second;                // Recalculate distance
                dx = targetX - currentX;
                dy = targetY - currentY;
                distance = std::sqrt(dx * dx + dy * dy);
                
                // Handle the waypoint arrival with smooth direction transition
                handleWaypointArrival(entity, elementName, config, currentX, currentY);
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
    // Use a more precise comparison to prevent overshooting
    if (moveDistance > distance - 0.01f) {
        // We're close enough to snap directly to the target
        moveDistance = distance;
    }
    
    // Normalize direction vector
    float normalizeFactor = moveDistance / distance;
    float moveDx = dx * normalizeFactor;
    float moveDy = dy * normalizeFactor;
    
    // Set the direction explicitly based on primary movement direction
    // This is more reliable than using the lambda
    if (std::abs(dx) > std::abs(dy)) {
        // Horizontal movement
        entity.lastDirection = (dx > 0) ? 3 : 2;  // 3 = right, 2 = left
    } else {
        // Vertical movement
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
            canMove = false;            // Try to find a safe path by testing intermediate positions
            // First check moving only in Y direction (vertical movement)
            newX = currentX;
            newY = currentY + moveDy;
            collisionWithElement = wouldCollideWithElement(newX, newY, config.collisionRadius);
            collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap);
            
            if (!collisionWithElement && !collisionWithMapBlock) {
                moveDx = 0; // Only move in Y
                canMove = true;
                // Force update sprite direction based on Y movement only
                // Use direct value to ensure correct sprite direction
                int verticalDirection = (moveDy > 0) ? 0 : 1; // 0 = up, 1 = down
                entity.lastDirection = verticalDirection;
                
                // Set sprite phase directly based on direction
                int phase = (verticalDirection == 0) ? config.spritePhaseWalkUp : config.spritePhaseWalkDown;
                elementsManager.changeElementSpritePhase(elementName, phase);
            } else {
                // Then check moving only in X direction (horizontal movement)
                newX = currentX + moveDx;
                newY = currentY;
                collisionWithElement = wouldCollideWithElement(newX, newY, config.collisionRadius);
                collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap);
                
                if (!collisionWithElement && !collisionWithMapBlock) {
                    moveDy = 0; // Only move in X
                    canMove = true;
                    // Force update sprite direction based on X movement only
                    // Use direct value to ensure correct sprite direction
                    int horizontalDirection = (moveDx > 0) ? 3 : 2; // 3 = right, 2 = left
                    entity.lastDirection = horizontalDirection;
                    
                    // Set sprite phase directly based on direction
                    int phase = (horizontalDirection == 3) ? config.spritePhaseWalkRight : config.spritePhaseWalkLeft;
                    elementsManager.changeElementSpritePhase(elementName, phase);
                }
            }if (!canMove) {
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
                        );                        // Check if a new path was found
                        if (!newPath.empty()) {
                            entity.path = newPath;
                            entity.currentPathIndex = 0;
                            std::cout << "Found new path with " << newPath.size() << " waypoints" << std::endl;
                            
                            // Get the first waypoint to update direction immediately
                            if (entity.path.size() > 0) {
                                float nextX = entity.path[0].first;
                                float nextY = entity.path[0].second;
                                float dirX = nextX - curX;
                                float dirY = nextY - curY;
                                
                                // Set direction directly based on actual movement vector to first waypoint
                                if (std::abs(dirX) > std::abs(dirY)) {
                                    // Horizontal movement
                                    entity.lastDirection = (dirX > 0) ? 3 : 2;  // 3 = right, 2 = left
                                } else {
                                    // Vertical movement
                                    entity.lastDirection = (dirY > 0) ? 0 : 1;  // 0 = up, 1 = down
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
                                
                                elementsManager.changeElementSpritePhase(elementName, phase);
                            }
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
    if (canMove) {
        if (!elementsManager.moveElement(elementName, moveDx, moveDy)) {
            // If movement failed (e.g. due to collision), stop walking
            entity.isWalking = false;
            elementsManager.changeElementAnimationStatus(elementName, false);
            elementsManager.changeElementSpriteFrame(elementName, 0);
            std::cout << "Entity " << entity.instanceName << " stopped walking due to movement failure" << std::endl;
        } else {
            // Make sure the animation is running since we're moving
            elementsManager.changeElementAnimationStatus(elementName, true);
            
            // Set animation speed based on walk type
            float animationSpeed = (entity.walkType == WalkType::NORMAL) ? 
                               config.normalWalkingAnimationSpeed : 
                               config.sprintWalkingAnimationSpeed;
            elementsManager.changeElementAnimationSpeed(elementName, animationSpeed);
        }
    } else {
        // We have a collision but we want to keep trying to reach the target
        // Update the sprite direction to match the intended movement even if we can't actually move
        updateDirectionAndSprite(dx, dy);
    }
}

// Function to ensure entity is not stuck in any collision areas
bool EntitiesManager::ensureEntityNotStuck(const std::string& instanceName) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        return false;
    }
    
    // Get the configuration for this entity type
    const EntityConfiguration* config = getConfiguration(entity->typeName);
    if (!config) {
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
        return false;
    }
      // Check if entity is in a collision state
    if (wouldCollideWithElement(x, y, config->collisionRadius) || 
        wouldCollideWithMapBlock(x, y, gameMap)) {
        
        // Entity is stuck, try to find a safe position
        if (findSafePosition(x, y, config->collisionRadius, gameMap)) {
            // Found a safe position, move entity there
            elementsManager.changeElementCoordinates(elementName, x, y);
            
            // Only log this in debug mode once every 10 seconds to reduce spam
            static float lastStuckDebugTime = 0.0f;
            float currentTime = static_cast<float>(glfwGetTime());
            if (playerDebugMode && currentTime - lastStuckDebugTime > 10.0f) {
                lastStuckDebugTime = currentTime;
                std::cout << "Entity " << instanceName << " was stuck, moved to safe position: (" 
                        << x << ", " << y << ")" << std::endl;
            }
            return true; // Position was adjusted
        } else {
            // Could not find a safe position - only log in debug mode with throttling
            static float lastFailedDebugTime = 0.0f;
            float currentTime = static_cast<float>(glfwGetTime());
            if (playerDebugMode && currentTime - lastFailedDebugTime > 15.0f) {
                lastFailedDebugTime = currentTime;
                std::cout << "Warning: Entity " << instanceName << " is stuck at (" << x << ", " << y 
                          << ") and no safe position could be found!" << std::endl;
            }
        }
    }
    
    return false; // No adjustment needed or possible
}

// Function to check if all entities are in safe positions, moving them if needed
void EntitiesManager::ensureAllEntitiesNotStuck() {
    // Track when we last checked each entity to avoid checking too frequently
    static std::map<std::string, float> lastCheckTimes;
    static float lastGlobalCheckTime = 0.0f;
    
    // Only run this check every 3.0 seconds to avoid performance impact
    float currentTime = static_cast<float>(glfwGetTime());
    if (currentTime - lastGlobalCheckTime < 3.0f) {
        return; // Skip this check if not enough time has passed
    }
    
    lastGlobalCheckTime = currentTime;
    
    // Count how many entities we actually check
    int entityChecksPerformed = 0;
    
    // Iterate through all entities and check if they're stuck, but throttle checks
    for (auto& pair : entities) {
        const std::string& entityName = pair.first;
        
        // Only check each entity at most once every several seconds
        auto it = lastCheckTimes.find(entityName);
        if (it != lastCheckTimes.end() && currentTime - it->second < 5.0f) {
            continue; // Skip this entity if checked recently
        }
        
        // Update the last check time for this entity
        lastCheckTimes[entityName] = currentTime;
        
        // Check and unstick if needed
        if (ensureEntityNotStuck(entityName)) {
            // Entity was adjusted, count it 
            entityChecksPerformed++;
        }
    }
      // Only print debug info if we performed any entity checks
    if (entityChecksPerformed > 0 && playerDebugMode) {
        std::cout << "Safety check: " << entityChecksPerformed 
                  << " entities repositioned during collision check" << std::endl;
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

// Handle waypoint arrival with direction smoothing
bool EntitiesManager::handleWaypointArrival(Entity& entity, const std::string& elementName, const EntityConfiguration& config, float currentX, float currentY) {
    // If entity has no remaining path, return
    if (entity.currentPathIndex >= entity.path.size()) {
        return false;
    }
    
    // Get the waypoint we're moving towards
    const auto& waypoint = entity.path[entity.currentPathIndex];
    float targetX = waypoint.first;
    float targetY = waypoint.second;
    
    // Calculate direction vector to next waypoint
    float dx = targetX - currentX;
    float dy = targetY - currentY;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Normalize direction vector
    float dirX = 0.0f;
    float dirY = 0.0f;
    if (distance > 0.0001f) {
        dirX = dx / distance;
        dirY = dy / distance;
    }
    
    // Store this as our current segment direction
    std::pair<float, float> currentDirection = {dirX, dirY};
    
    // Calculate if there's a significant direction change
    bool significantDirectionChange = false;
    
    // Check if we have a previous direction to compare with
    if (entity.lastSegmentDirection.first != 0.0f || entity.lastSegmentDirection.second != 0.0f) {
        // Use dot product to determine how different the directions are
        float dotProduct = entity.lastSegmentDirection.first * currentDirection.first +
                          entity.lastSegmentDirection.second * currentDirection.second;
        
        // A dot product < 0.7 indicates an angle > ~45 degrees
        significantDirectionChange = (dotProduct < 0.7f);
    }
    
    // Determine new sprite direction
    int newDirection;
    
    // If this is a significant change in direction, update sprite direction immediately
    if (significantDirectionChange) {
        // Use the main direction component for sprite direction
        if (std::abs(dirX) > std::abs(dirY)) {
            // Horizontal movement dominates
            newDirection = (dirX > 0) ? 3 : 2;  // 3 = right, 2 = left
        } else {
            // Vertical movement dominates
            newDirection = (dirY > 0) ? 0 : 1;  // 0 = up, 1 = down
        }
        
        // Update entity direction
        entity.lastDirection = newDirection;
        
        // Update the sprite phase
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
    }
    
    // Update the last segment direction
    entity.lastSegmentDirection = currentDirection;
    
    return true;
}