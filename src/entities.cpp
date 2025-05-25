#include "entities.h"
#include "collision.h"
#include "map.h" // Adding for gameMap access
#include "pathfinding.h"
#include "globals.h" // For GRID_SIZE
#include "debug.h" // For isShowingCollisionBoxes function
#include <iostream>
#include <cmath>

// Static vector of predefined entity types
static std::vector<EntityInfo> entityTypes;

// Function to initialize entity types
static void initializeEntityTypes() {
    if (!entityTypes.empty()) {
        return; // Already initialized
    }    // Antagonist entity configuration
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
    antagonist.sprintWalkingAnimationSpeed = 12.0f;    // Collision settings
    antagonist.canCollide = true;
    antagonist.collisionRadius = 0.4f;
    antagonist.collisionShapePoints = {
        {-2.3f, -2.3f}, {2.3f, -2.3f}, {2.3f, 2.3f}, {-2.3f, 2.3f}
    };    // Granular collision control - antagonist avoids trees but can move through player
    // For testing: Leave lists empty to allow movement through all elements
    antagonist.avoidanceElements = {
        // Empty - no elements to avoid for pathfinding
        ElementTextureName::COCONUT_TREE_1,
        ElementTextureName::COCONUT_TREE_2,
        ElementTextureName::COCONUT_TREE_2,
    };
    antagonist.collisionElements = {
        // Empty - no elements to collide with during movement
        ElementTextureName::COCONUT_TREE_1,
        ElementTextureName::COCONUT_TREE_2,
        ElementTextureName::COCONUT_TREE_2,
    };
      
    // Add to the list
    entityTypes.push_back(antagonist);


    EntityInfo player;
    player.typeName = "player";
    player.textureName = ElementTextureName::CHARACTER1;
    player.scale = 1.5f;
    
    // Default sprite configuration
    player.defaultSpriteSheetPhase = 2;
    player.defaultSpriteSheetFrame = 0;
    player.defaultAnimationSpeed = 11.0f;
    
    // Walking animation phases
    player.spritePhaseWalkUp = 0;
    player.spritePhaseWalkDown = 3;
    player.spritePhaseWalkLeft = 2;
    player.spritePhaseWalkRight = 1;
    
    // Movement speeds
    player.normalWalkingSpeed = 1.5f;
    player.normalWalkingAnimationSpeed = 4.0f;
    player.sprintWalkingSpeed = 10.0f;
    player.sprintWalkingAnimationSpeed = 12.0f;    // Collision settings
    player.canCollide = true;
    player.collisionRadius = 0.4f;
    player.collisionShapePoints = {
        {-2.3f, -2.3f}, {2.3f, -2.3f}, {2.3f, 2.3f}, {-2.3f, 2.3f}
    };    // Granular collision control - player avoids trees and other characters
    // For testing: Leave lists empty to allow movement through all elements
    player.avoidanceElements = {
        // Empty - no elements to avoid for pathfinding
    };
    player.collisionElements = {
        // Empty - no elements to collide with during movement
        // Empty - no elements to avoid for pathfinding
        ElementTextureName::COCONUT_TREE_1,
        ElementTextureName::COCONUT_TREE_2,
        ElementTextureName::COCONUT_TREE_2,
        
    };
      
    // Add to the list
    entityTypes.push_back(player);



    
    // Add to the list    
    // Add more entity types here as needed
}

// Global instance definition
EntitiesManager entitiesManager;

// Forward declaration for gameMap access (defined in main.cpp)
extern Map gameMap;

// Static method to get element name from instance name
std::string EntitiesManager::getElementName(const std::string& instanceName) {
    // return "entity_" + instanceName;
    return instanceName;

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
    }      // COLLISION RESOLUTION MECHANISMS DISABLED
    // Entities will no longer be automatically moved to "safe positions" during placement
    float safeX = x;
    float safeY = y;
    bool needsSafePosition = false;
    // Safe position checking removed - entities will be placed at requested coordinates
    
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
        safeX, safeY,  // Use the requested position (no automatic adjustment)
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
        // This block should never execute since safe position checking is disabled
        std::cout << "Entity " << instanceName << " created at requested position (" << safeX << "," << safeY 
                << ") - safe position adjustment disabled" << std::endl;
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
    entity->usePathfinding = false; // Explicitly set usePathfinding to false
    entity->path.clear(); // Ensure path is empty for direct movement
      // Enable animation
    elementsManager.changeElementAnimationStatus(elementName, true);
    
    // Set the animation speed based on walk type
    float animationSpeed = (walkType == WalkType::NORMAL) ? 
                          config->normalWalkingAnimationSpeed : 
                          config->sprintWalkingAnimationSpeed;
    elementsManager.changeElementAnimationSpeed(elementName, animationSpeed);
    
    // Set initial sprite direction based on movement direction
    float currentX, currentY;
    if (elementsManager.getElementPosition(elementName, currentX, currentY)) {
        float dx = x - currentX;
        float dy = y - currentY;
        
        // Determine the primary direction
        int direction;
        if (std::abs(dx) > std::abs(dy)) {
            // Horizontal movement dominates
            direction = (dx > 0) ? 3 : 2;  // 3 = right, 2 = left
        } else {
            // Vertical movement dominates or is equal
            direction = (dy > 0) ? 0 : 1;  // 0 = up, 1 = down
        }
        
        // Set sprite direction
        entity->lastDirection = direction;
        
        // Set the appropriate sprite phase based on direction
        int phase;
        switch (direction) {
            case 0: phase = config->spritePhaseWalkUp; break;
            case 1: phase = config->spritePhaseWalkDown; break;
            case 2: phase = config->spritePhaseWalkLeft; break;
            case 3: phase = config->spritePhaseWalkRight; break;
            default: phase = config->defaultSpriteSheetPhase; break;
        }
        
        // Change the sprite phase
        elementsManager.changeElementSpritePhase(elementName, phase);
    }
    
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

    // Get the element name
    std::string elementName = getElementName(instanceName);

    // Get current position from entity if available, otherwise from element
    float startPathX, startPathY;
    if (!elementsManager.getElementPosition(elementName, startPathX, startPathY)) {
        std::cerr << "Error getting position for entity: " << instanceName << std::endl;
        return false;
    }    // Pathfinding logic
    std::vector<std::pair<float, float>> path = findPath(
        startPathX, startPathY,
        x, y,
        gameMap,
        *config
    );

    entity->path = path; // Store the raw path first

    if (entity->path.size() < 2) {
        // Path is too short (0 or 1 point), no movement possible or already at destination.
        // This can happen if start and goal are the same or no path found and findPath returns just start.
        entity->isWalking = false;
        elementsManager.changeElementAnimationStatus(elementName, false);
        if (config) {
            elementsManager.changeElementSpriteFrame(elementName, config->defaultSpriteSheetFrame);
            elementsManager.changeElementSpritePhase(elementName, config->defaultSpriteSheetPhase);
        }
        // std::cout << "Entity " << instanceName << " path too short or invalid. Not walking. Path size: " << entity->path.size() << std::endl;
        // If path has one point, and it's different from target, it's a failure. If same, it's success (already there).
        // findPath should return empty on failure, or path with start/goal.
        // If path has 1 point, it's the start point. If start != goal, then this is effectively a "no path found" scenario.
        bool targetIsDifferentFromStart = (entity->path.empty() || (std::abs(entity->path[0].first - x) > 0.01f || std::abs(entity->path[0].second - y) > 0.01f));
        if (entity->path.size() < 2 && targetIsDifferentFromStart) {
             std::cout << "No valid path found for entity " << instanceName << " to reach ("
                      << x << ", " << y << ") or path too short. Path size: " << entity->path.size() << std::endl;
            return false; // Indicate failure to start walking
        }
        // If path has 1 point and it IS the target, then we are good.
        return true; // Successfully "walked" (by not moving)
    }
    
    // Path is valid and has at least 2 points.
    entity->currentPathIndex = 1; // currentPathIndex is the index of the NEXT waypoint to target. path[0] is start.
    entity->isWalking = true;
    entity->walkType = walkType;
    entity->usePathfinding = true;
    entity->lastSegmentDirection = {0.0f, 0.0f}; // Reset last segment direction

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

    // Immediately determine and set sprite for the first segment (path[0] to path[1])
    // The 'currentX, currentY' passed to handleWaypointArrival are the coordinates of the segment's start.
    handleWaypointArrival(*entity, elementName, *config, entity->path[0].first, entity->path[0].second);

    std::cout << "Entity " << instanceName << " starting pathfinding to (" << x << ", " << y << ") with "
              << ((walkType == WalkType::NORMAL) ? "normal" : "sprint") << " speed, path size: " << entity->path.size() 
              << ", initial target index: " << entity->currentPathIndex << std::endl;
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
    // COLLISION RESOLUTION MECHANISMS DISABLED
    // Stuck entity checking removed - entities will no longer be automatically repositioned
    // ensureAllEntitiesNotStuck(); // DISABLED
    
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

void EntitiesManager::drawDebugPaths(float startX, float endX, float startY, float endY, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop) {
    if (!DEBUG_SHOW_PATHS) {
        return;
    }

    glLineWidth(2.0f); // Set line width for paths
    
    for (const auto& pair : entities) {
        const Entity& entity = pair.second;
        if (entity.isWalking) {
            float currentX, currentY;
            std::string elementName = getElementName(entity.instanceName);
            if (!elementsManager.getElementPosition(elementName, currentX, currentY)) {
                continue; // Skip if we can't get current position
            }

            // Convert current entity position to screen coordinates
            float entityScreenX = startX + (currentX - cameraLeft) / (cameraRight - cameraLeft) * (endX - startX);
            float entityScreenY = startY + (currentY - cameraBottom) / (cameraTop - cameraBottom) * (endY - startY);

            if (entity.usePathfinding && !entity.path.empty()) {
                glColor3f(0.0f, 0.0f, 1.0f); // Blue color for pathfinding paths
                glBegin(GL_LINE_STRIP);
                glVertex2f(entityScreenX, entityScreenY); // Start line from current entity position

                for (size_t i = entity.currentPathIndex; i < entity.path.size(); ++i) {
                    const auto& waypoint = entity.path[i];
                    // Convert world to screen coordinates
                    float screenX = startX + (waypoint.first - cameraLeft) / (cameraRight - cameraLeft) * (endX - startX);
                    float screenY = startY + (waypoint.second - cameraBottom) / (cameraTop - cameraBottom) * (endY - startY);
                    glVertex2f(screenX, screenY);
                }
                glEnd();
            } else if (!entity.usePathfinding) {
                glColor3f(1.0f, 0.0f, 0.0f); // Red color for direct paths (walkEntityToCoordinates)
                glBegin(GL_LINES);
                glVertex2f(entityScreenX, entityScreenY); // Line start

                // Convert target world to screen coordinates
                float targetScreenX = startX + (entity.targetX - cameraLeft) / (cameraRight - cameraLeft) * (endX - startX);
                float targetScreenY = startY + (entity.targetY - cameraBottom) / (cameraTop - cameraBottom) * (endY - startY);
                glVertex2f(targetScreenX, targetScreenY); // Line end
                glEnd();
            }
        }
    }
}

void EntitiesManager::drawDebugCollisionRadii(float startX, float endX, float startY, float endY, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop) {
    // Only draw if collision visualization is enabled
    if (!isShowingCollisionBoxes()) {
        return;
    }

    glLineWidth(2.0f); // Set line width for collision shapes
    
    for (const auto& pair : entities) {
        const Entity& entity = pair.second;
        
        // Get the configuration for this entity to access collision shape
        const EntityConfiguration* config = getConfiguration(entity.typeName);
        if (!config || !config->canCollide) {
            continue; // Skip entities without collision or configuration
        }
        
        // Get current entity position
        float currentX, currentY;
        std::string elementName = getElementName(entity.instanceName);
        if (!elementsManager.getElementPosition(elementName, currentX, currentY)) {
            continue; // Skip if we can't get current position
        }

        // Check if entity has collision shape points
        if (!config->collisionShapePoints.empty()) {
            // Draw polygon collision shape
            std::vector<std::pair<float, float>> worldShapePoints;
            
            // Transform collision shape points to world coordinates (assuming no rotation for now)
            for (const auto& localPoint : config->collisionShapePoints) {
                worldShapePoints.push_back({currentX + localPoint.first, currentY + localPoint.second});
            }
            
            // Convert world coordinates to screen coordinates
            std::vector<std::pair<float, float>> screenShapePoints;
            for (const auto& worldPoint : worldShapePoints) {
                float screenX = startX + (worldPoint.first - cameraLeft) / (cameraRight - cameraLeft) * (endX - startX);
                float screenY = startY + (worldPoint.second - cameraBottom) / (cameraTop - cameraBottom) * (endY - startY);
                screenShapePoints.push_back({screenX, screenY});
            }
              // Draw polygon outline only (no fill)
            glColor4f(0.0f, 0.8f, 0.0f, 0.8f); // More solid green for the outline
            glBegin(GL_LINE_LOOP);
            for (const auto& screenPoint : screenShapePoints) {
                glVertex2f(screenPoint.first, screenPoint.second);
            }
            glEnd();
        } else {
            // Fallback to drawing collision radius as a square (for entities without collision shape points)
            float viewWidth = cameraRight - cameraLeft;
            float screenWidth = endX - startX;
            float radiusInScreenCoords = (config->collisionRadius / viewWidth) * screenWidth;

            // Convert entity position to screen coordinates
            float entityScreenX = startX + (currentX - cameraLeft) / (cameraRight - cameraLeft) * (endX - startX);
            float entityScreenY = startY + (currentY - cameraBottom) / (cameraTop - cameraBottom) * (endY - startY);

            // Calculate square dimensions (square with side length = 2 * radius)
            float halfSide = radiusInScreenCoords;
            float left = entityScreenX - halfSide;
            float right = entityScreenX + halfSide;
            float bottom = entityScreenY - halfSide;
            float top = entityScreenY + halfSide;            // Draw square outline only (no fill)
            glColor4f(0.0f, 0.8f, 0.0f, 0.8f); // More solid green for the outline
            glBegin(GL_LINE_LOOP);
            glVertex2f(left, bottom);   // Bottom-left
            glVertex2f(right, bottom);  // Bottom-right
            glVertex2f(right, top);     // Top-right
            glVertex2f(left, top);      // Top-left
            glEnd();
        }
    }
    
    glLineWidth(1.0f); // Reset line width
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
    float currentActualX, currentActualY;
    if (!elementsManager.getElementPosition(elementName, currentActualX, currentActualY)) {
        std::cerr << "Error getting position for entity: " << entity.instanceName << std::endl;
        entity.isWalking = false; // Stop walking due to error
        return;
    }

    auto updateDirectionAndSprite = [&](Entity& ent, const std::string& elName, const EntityConfiguration& cfg, float dirX, float dirY) {
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
        elementsManager.changeElementSpritePhase(elName, phase);
    };
    
    float targetX = 0.0f, targetY = 0.0f;
    float dx = 0.0f, dy = 0.0f, distance = 0.0f;

    if (entity.usePathfinding && !entity.path.empty()) {
        // Ensure currentPathIndex is valid before accessing path
        if (entity.currentPathIndex < entity.path.size()) { // Path index must be < size to be a valid target
            const auto& waypoint = entity.path[entity.currentPathIndex]; // Target is path[currentPathIndex]
            targetX = waypoint.first;
            targetY = waypoint.second;
        } else {
            // Path ended (currentPathIndex == path.size()), entity should be stopping or at final target.
            // Target the final destination stored in entity.targetX/Y
            targetX = entity.targetX;
            targetY = entity.targetY;
        }

        dx = targetX - currentActualX;
        dy = targetY - currentActualY;
        distance = std::sqrt(dx * dx + dy * dy);

        float waypointThreshold = 0.05f; 
        if (distance <= waypointThreshold) { // Reached current target waypoint or final destination
            if (entity.currentPathIndex < entity.path.size()) { // If we were heading to a valid waypoint path[currentPathIndex]
                // This was the waypoint we just reached.
                float reachedWaypointX = entity.path[entity.currentPathIndex].first;
                float reachedWaypointY = entity.path[entity.currentPathIndex].second;

                entity.currentPathIndex++; // Advance to next target index (or path.size() if this was the last)

                if (entity.currentPathIndex < entity.path.size()) { // If there\'s a new waypoint to move towards
                    // Set sprite for the new segment: from reachedWaypointX,Y to path[new currentPathIndex]
                    handleWaypointArrival(entity, elementName, config, reachedWaypointX, reachedWaypointY);
                    
                    // Update targetX, targetY, dx, dy, distance for the new segment immediately for this frame\'s move
                    const auto& nextWaypoint = entity.path[entity.currentPathIndex];
                    targetX = nextWaypoint.first;
                    targetY = nextWaypoint.second;
                    dx = targetX - currentActualX; // dx from current actual pos to new target
                    dy = targetY - currentActualY; // dy from current actual pos to new target
                    distance = std::sqrt(dx * dx + dy * dy);

                } else {
                    // Reached end of path (currentPathIndex is now == path.size())
                    // Sprite remains as per the last segment.
                    // Final stop logic will be handled by distance to entity.targetX/Y (which is current targetX/Y).
                }
            }
            // If currentPathIndex was already >= path.size/, it means we are checking distance to final entity.targetX/Y.
            // If distance <= waypointThreshold, the main stop logic later will handle it.
        }
        // Note: The \'else if (entity.currentPathIndex == 0 && ...)\' block for initial call is removed
        // as walkEntityWithPathfinding now handles the first segment\'s sprite.

    } else { // Not using pathfinding or path is empty (but isWalking is true)
        targetX = entity.targetX;
        targetY = entity.targetY;
        
        // Calculate distance to target
        dx = targetX - currentActualX;
        dy = targetY - currentActualY;
        distance = std::sqrt(dx * dx + dy * dy);
    }
    
    // Stop if we\'re close enough to the target
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
        // We\'re close enough to snap directly to the target
        moveDistance = distance;
    }
    
    // Normalize direction vector
    float moveDx = 0.0f, moveDy = 0.0f;
    if (distance > 0.001f) { // Avoid division by zero
        float normalizeFactor = moveDistance / distance;
        moveDx = dx * normalizeFactor;
        moveDy = dy * normalizeFactor;
    }

    // If not using pathfinding, update sprite direction based on direct movement to targetX, targetY
    if (!entity.usePathfinding) {
        updateDirectionAndSprite(entity, elementName, config, dx, dy);    }
    // For pathfinding, sprite direction is handled by handleWaypointArrival.
    
    // Check for collisions before moving the entity using axis-separated collision detection
    bool canMove = true;
    float actualMoveDx = moveDx;    float actualMoveDy = moveDy;
    
    if (config.canCollide) {
        // Calculate the new position after movement
        float newX = currentActualX + moveDx;
        float newY = currentActualY + moveDy;
        
        // Check if the combined movement would collide with collision elements (hard collision only)
        // Note: We only check collision elements here, not avoidance elements
        // This allows entities to move through avoidance elements during direct movement
        bool collisionWithElement = wouldEntityCollideWithElementsGranular(config, newX, newY, false);
        
        if (collisionWithElement && (moveDx != 0 || moveDy != 0)) {
            // If diagonal movement fails, try axis-separated movement (like player system)
            actualMoveDx = 0;
            actualMoveDy = 0;
            canMove = false;
            
            // If we're trying to move diagonally, test each axis separately
            if (moveDx != 0 && moveDy != 0) {
                // Try moving only horizontally
                float testX = currentActualX + moveDx;
                float testY = currentActualY; // Keep Y the same
                bool horizontalCollision = wouldEntityCollideWithElementsGranular(config, testX, testY, false);
                
                // Try moving only vertically
                float testX2 = currentActualX; // Keep X the same
                float testY2 = currentActualY + moveDy;
                bool verticalCollision = wouldEntityCollideWithElementsGranular(config, testX2, testY2, false);
                
                // If horizontal movement is possible
                if (!horizontalCollision) {
                    actualMoveDx = moveDx;
                    canMove = true;
                }
                
                // If vertical movement is possible
                if (!verticalCollision) {
                    actualMoveDy = moveDy;
                    canMove = true;
                }
            } else {
                // Single-axis movement that's blocked - no alternative
                canMove = false;
            }
            
            // Update movement deltas based on what's actually possible
            moveDx = actualMoveDx;
            moveDy = actualMoveDy;
        } else if (!collisionWithElement) {
            // No collision, can move normally
            canMove = true;
        } else {
            // No movement requested, but still blocked (shouldn't happen normally)
            canMove = false;
        }
        
        // If still can't move after axis separation, try pathfinding recalculation
        if (!canMove) {
                // Entity is stuck, let's try to recalculate the path if using pathfinding
                if (entity.usePathfinding) {
                    std::cout << "Entity " << entity.instanceName << " encountered obstacle at (" 
                              << newX << ", " << newY << "), recalculating path..." << std::endl;                    // Identify what's blocking the entity
                    bool hasElementCollision = wouldEntityCollideWithElementsGranular(config, newX, newY, false);
                      if (hasElementCollision) {
                        std::cout << "  - Element collision detected" << std::endl;
                        // Get nearby elements for debugging
                        std::vector<std::string> nearbyElements = getNearbyElements(newX, newY, config.collisionRadius + 0.5f);
                        if (!nearbyElements.empty()) {
                            std::cout << "  - Nearby elements: ";
                            for (const auto& name : nearbyElements) {
                                std::cout << name << " ";
                            }
                            std::cout << std::endl;
                        }                    }
                      // COLLISION RESOLUTION MECHANISMS DISABLED
                    // Entities will no longer be automatically moved to "safe positions" when stuck
                    // Try to find a safe position first before recalculating - DISABLED
                    // float safeX = currentActualX;
                    // float safeY = currentActualY;
                    // float safetyBuffer = 0.3f;
                    // float safeRadius = config.collisionRadius + safetyBuffer;                      // COLLISION RESOLUTION MECHANISMS DISABLED
                    // Safe position teleportation removed - entities will remain in current position
                    // bool foundSafePos = findSafePosition(safeX, safeY, safeRadius, gameMap);
                    
                    // if (foundSafePos) {
                    //     // Found a safe position, teleport there
                    //     elementsManager.changeElementCoordinates(elementName, safeX, safeY);
                    //     std::cout << "Found safe position at (" << safeX << ", " << safeY 
                    //               << "), moving entity there before recalculating path" << std::endl;
                    //     
                    //     // Update current position
                    //     currentActualX = safeX;
                    //     currentActualY = safeY;
                    // }
                    
                    // Get current position (which might have been updated)
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
                        }                        // Recalculate the path from the current position
                        // Use an enlarged collision radius for safer paths
                        float pathPlanningRadius = config.collisionRadius + 0.3f;                        std::vector<std::pair<float, float>> newPath = findPath(
                            curX, curY,
                            entity.targetX, entity.targetY,
                            gameMap,
                            config
                        );
                        
                        // Check if a new path was found
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
                                
                                // Set direction based on movement vector to first waypoint
                                // This call was missing arguments
                                // updateDirectionAndSprite(dirX_recalc, dirY_recalc); 
                                // Sprite for pathfinding is set by handleWaypointArrival
                            }
                            
                            // Make sure we're moving next frame
                            canMove = true;
                            return; // Skip the rest of this update cycle, we'll calculate movement next frame
                        } else {                            // COLLISION RESOLUTION MECHANISMS DISABLED
                            // Safe position finding near target removed - entities will use original target
                            // No path found, try with an alternative approach - DISABLED
                            // Find a safe position near the target first
                            // float safeTargetX = entity.targetX;
                            // float safeTargetY = entity.targetY;

                            // Try to find a safe position near the target - DISABLED
                            // if (findSafePosition(safeTargetX, safeTargetY, pathPlanningRadius, gameMap)) {                                // COLLISION RESOLUTION MECHANISMS DISABLED
                                // Found a safe position near the target, try pathfinding to it - DISABLED
                                // newPath = findPath(
                                //     curX, curY,
                                //     safeTargetX, safeTargetY,
                                //     gameMap,
                                //     config
                                // );
                                
                                // if (!newPath.empty()) {
                                //     // Found a path to the safe target position
                                //     entity.path = newPath;
                                //     entity.currentPathIndex = 0;
                                //     entity.targetX = safeTargetX;
                                //     entity.targetY = safeTargetY;
                                //     std::cout << "Found path to safe target with " << newPath.size() << " waypoints" << std::endl;
                                //     
                                //     // Update direction
                                //     if (entity.path.size() > 0) {
                                //         float nextX = entity.path[0].first;
                                //                                         float nextY = entity.path[0].second;
                                //         float dirX = nextX - curX;
                                //         float dirY = nextY - curY;
                                //         
                                //         // Update direction and sprite
                                //         // This call was missing arguments
                                //         // updateDirectionAndSprite(dirX, dirY);
                                //         // Sprite for pathfinding is set by handleWaypointArrival
                                //     }
                                //     
                                //     // Make sure we're moving next frame
                                //     canMove = true;
                                //     return; // Skip the rest of this update cycle
                                // }
                            // }
                        }
                    }
                    // If no path found or not using pathfinding, just stop and try again next frame
                    std::cout << "Entity " << entity.instanceName << " encountered obstacle at (" 
                              << newX << ", " << newY << ")" << std::endl;
                }
            }
        }
    
    // Move the entity if no collision
    if (canMove) {
        if (!elementsManager.moveElement(elementName, moveDx, moveDy)) {
            // Movement failed for some other reason
            // entity.isWalking = false; // Consider stopping if moveElement fails
            // elementsManager.changeElementAnimationStatus(elementName, false);
            // elementsManager.changeElementSpriteFrame(elementName, 0);
            // std::cout << "Entity " << entity.instanceName << " stopped walking due to movement failure by elementsManager" << std::endl;
        } else {
            // Successfully moved
            elementsManager.changeElementAnimationStatus(elementName, true); // Keep animation running
            // Animation speed is set when walking starts/changes type, not needed every frame unless speed can change mid-segment.
            // float animationSpeed = (entity.walkType == WalkType::NORMAL) ? 
            //                   config.normalWalkingAnimationSpeed : 
            //                   config.sprintWalkingAnimationSpeed;
            // elementsManager.changeElementAnimationSpeed(elementName, animationSpeed);
        }    } else { // canMove is false (collision)
        // We have a collision but we want to keep trying to reach the target
        // Update the sprite direction to match the intended movement even if we can't actually move
        // This call was missing arguments
        // updateDirectionAndSprite(dx, dy);
        // For pathfinding, sprite is set by handleWaypointArrival. For direct, it was set earlier.
        // If it's a direct movement and we are here, it means collision was detected
        // COLLISION RESOLUTION DISABLED - findSafePosition no longer used
        // The sprite direction should reflect the intended dx, dy if not using pathfinding.
        if (!entity.usePathfinding) {
            updateDirectionAndSprite(entity, elementName, config, dx, dy);
        }
    }
}

// Function to check entity collision with elements (uses collision shape points if available, otherwise fallback to radius)
bool wouldEntityCollideWithElement(const EntityConfiguration& config, float x, float y) {
    if (!config.collisionShapePoints.empty()) {
        // Use polygon-based collision detection
        return wouldEntityCollideWithElement(x, y, config.collisionShapePoints, 1.0f, 0.0f);
    } else {
        // Fallback to radius-based collision detection
        return wouldCollideWithElement(x, y, config.collisionRadius);
    }
}

// Enhanced collision function that respects granular collision settings
bool wouldEntityCollideWithElementsGranular(const EntityConfiguration& config, float x, float y, bool useAvoidanceList) {
    if (!config.canCollide) {
        return false; // Entity doesn't have collision enabled
    }
      // Determine which element list to check based on collision type
    const std::vector<ElementTextureName>& elementsToCheck = useAvoidanceList ? config.avoidanceElements : config.collisionElements;
    
    // If the list is empty, don't check any elements (granular collision control)
    // This allows entities to have fine-grained control over what they collide with
    if (elementsToCheck.empty()) {
        return false; // No elements to check = no collision
    }
    
    // Make sure the spatial grid is up to date
    if (!spatialGridInitialized) {
        updateSpatialGrid();
    }
    
    // Calculate approximate radius for nearby element search
    float searchRadius = config.collisionRadius;
    if (!config.collisionShapePoints.empty()) {
        float maxRadius = 0.0f;
        for (const auto& point : config.collisionShapePoints) {
            float dist = std::sqrt(point.first * point.first + point.second * point.second);
            maxRadius = std::max(maxRadius, dist);
        }
        searchRadius = maxRadius;
    }
    
    // Get only nearby elements instead of checking all elements
    std::vector<std::string> nearbyElements = getNearbyElements(x, y, searchRadius + MAX_COLLISION_CHECK_RANGE);
    
    // Check collision only with elements that match the specified texture types
    for (const auto& elementName : nearbyElements) {
        // Find the element to get its texture type
        const auto& elements = elementsManager.getElements();
        const PlacedElement* currentElement = nullptr;
        for (const auto& el : elements) {
            if (el.instanceName == elementName) {
                currentElement = &el;
                break;
            }
        }
        
        if (!currentElement || !currentElement->hasCollision) {
            continue; // Skip elements without collision enabled
        }
        
        // Check if this element's texture type is in our list to check
        bool shouldCheckThisElement = false;
        for (const auto& textureType : elementsToCheck) {
            if (currentElement->textureName == textureType) {
                shouldCheckThisElement = true;
                break;
            }
        }
        
        if (!shouldCheckThisElement) {
            continue; // Skip elements not in our collision/avoidance list
        }
        
        // Perform the actual collision check for this specific element
        if (!config.collisionShapePoints.empty() && !currentElement->collisionShapePoints.empty()) {
            // Use polygon-polygon collision detection
            std::vector<std::pair<float, float>> entityWorldShapePoints;
            std::vector<std::pair<float, float>> elementWorldShapePoints;
            
            // Transform entity polygon points to world coordinates
            float entityAngleRad = 0.0f; // Entities don't rotate currently
            float entityCosA = cos(entityAngleRad);
            float entitySinA = sin(entityAngleRad);
            
            for (const auto& localPoint : config.collisionShapePoints) {
                float scaledX = localPoint.first;
                float scaledY = localPoint.second;
                
                float rotatedX = scaledX * entityCosA - scaledY * entitySinA;
                float rotatedY = scaledX * entitySinA + scaledY * entityCosA;
                
                entityWorldShapePoints.push_back({x + rotatedX, y + rotatedY});
            }
            
            // Transform element polygon points to world coordinates
            float elementAngleRad = currentElement->rotation * M_PI / 180.0f;
            float elementCosA = cos(elementAngleRad);
            float elementSinA = sin(elementAngleRad);
            
            for (const auto& localPoint : currentElement->collisionShapePoints) {
                float scaledX = localPoint.first * currentElement->scale;
                float scaledY = localPoint.second * currentElement->scale;
                
                float rotatedX = scaledX * elementCosA - scaledY * elementSinA;
                float rotatedY = scaledX * elementSinA + scaledY * elementCosA;
                
                elementWorldShapePoints.push_back({currentElement->x + rotatedX, currentElement->y + rotatedY});
            }
            
            // Perform polygon-polygon collision detection using Separating Axis Theorem (SAT)
            if (polygonPolygonCollision(entityWorldShapePoints, elementWorldShapePoints)) {
                return true; // Collision detected
            }
        } else {
            // Fallback to circle-circle collision detection
            float dx = x - currentElement->x;
            float dy = y - currentElement->y;
            float distance = std::sqrt(dx * dx + dy * dy);
            float combinedRadius = config.collisionRadius + 0.3f; // Assume default element radius
            
            if (distance < combinedRadius) {
                return true; // Collision detected
            }
        }
    }
    
    return false; // No collision detected with specified element types
}

// Teleport an entity to specific coordinates immediately (handles collisions)
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
      // COLLISION RESOLUTION MECHANISMS DISABLED  
    // Entities will no longer be automatically moved to "safe positions" during teleportation
    float safeX = x;
    float safeY = y;
    bool needsSafePosition = false;
    // Safe position checking removed - entities will be teleported to requested coordinates
    
    // Stop any current walking
    entity->isWalking = false;
    entity->path.clear();
    entity->currentPathIndex = 0;
      // Teleport the element on the map
    if (!elementsManager.changeElementCoordinates(elementName, safeX, safeY)) {
        std::cerr << "Failed to teleport entity element: " << instanceName << std::endl;
        return false;
    }
    
    // Disable animation (entity is not walking after teleport)
    elementsManager.changeElementAnimationStatus(elementName, false);
    
    // Reset to default sprite
    if (config) {
        elementsManager.changeElementSpriteFrame(elementName, config->defaultSpriteSheetFrame);
        elementsManager.changeElementSpritePhase(elementName, config->defaultSpriteSheetPhase);
    }
    
    std::cout << "Teleported entity " << instanceName << " to (" << safeX << ", " << safeY << ")" << std::endl;
    return true;
}

// Collision resolution functions removed - entities will no longer be automatically moved
// Function to ensure no entities are stuck in collision areas (DISABLED)
bool EntitiesManager::ensureEntityNotStuck(const std::string& instanceName) {
    return true; // Always return true - no automatic position adjustment
}

// Function to check if all entities are in safe positions (DISABLED)
void EntitiesManager::ensureAllEntitiesNotStuck() {
    // Do nothing - collision resolution mechanisms disabled
}

// Handle waypoint arrival - added to improve movement precision
bool EntitiesManager::handleWaypointArrival(Entity& entity, const std::string& elementName, const EntityConfiguration& config, float currentX, float currentY) {
    // Check if we're using pathfinding
    if (!entity.usePathfinding || entity.path.empty() || entity.currentPathIndex >= entity.path.size()) {
        return false; // Not using pathfinding or no valid path
    }
    
    // Get the current target waypoint
    const auto& targetWaypoint = entity.path[entity.currentPathIndex];
    float targetX = targetWaypoint.first;
    float targetY = targetWaypoint.second;
    
    // Calculate direction to target
    float dx = targetX - currentX;
    float dy = targetY - currentY;
    
    // Determine the primary direction for sprite updates
    int direction;
    if (std::abs(dx) > std::abs(dy)) {
        // Horizontal movement dominates
        direction = (dx > 0) ? 3 : 2;  // 3 = right, 2 = left
    } else {
        // Vertical movement dominates or is equal
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
    
    // Change the sprite phase
    elementsManager.changeElementSpritePhase(elementName, phase);
    
    return true;
}