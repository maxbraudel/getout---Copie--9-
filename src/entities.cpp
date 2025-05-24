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
    antagonist.sprintWalkingAnimationSpeed = 12.0f;      // Collision settings
    antagonist.canCollide = true;
    antagonist.collisionRadius = 0.4f;
    antagonist.collisionShapePoints = {
        {-2.3f, -2.3f}, {2.3f, -2.3f}, {2.3f, 2.3f}, {-2.3f, 2.3f}
    };
      // Define non-traversable blocks for antagonist
    // Antagonists cannot walk on water blocks or through coconut trees
    antagonist.nonTraversableBlocks = {
        TextureName::WATER_0,
        TextureName::WATER_1,
        TextureName::WATER_2,
        TextureName::WATER_3,
        TextureName::WATER_4
        // Note: Coconut trees are elements, not map blocks, so they're handled via element collision detection
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
    player.sprintWalkingAnimationSpeed = 12.0f;
      // Collision settings
    player.canCollide = true;
    player.collisionRadius = 0.4f;
    player.collisionShapePoints = {
        {-2.3f, -2.3f}, {2.3f, -2.3f}, {2.3f, 2.3f}, {-2.3f, 2.3f}
    };
      // Define non-traversable blocks for player
    // Antagonists cannot walk on water blocks or through coconut trees
    player.nonTraversableBlocks = {
        TextureName::WATER_0,
        TextureName::WATER_1,
        TextureName::WATER_2,
        TextureName::WATER_3,
        TextureName::WATER_4
        // Note: Coconut trees are elements, not map blocks, so they're handled via element collision detection
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
    }
    
    // Check for a safe starting position if this entity type has collisions enabled
    float safeX = x;
    float safeY = y;
    bool needsSafePosition = false;
    
    if (config->canCollide) {
        // Check if the target position is in a collision area
        if (wouldEntityCollideWithElement(*config, safeX, safeY) ||        wouldCollideWithMapBlock(safeX, safeY, gameMap, config->nonTraversableBlocks)) {
            needsSafePosition = true;
            // Try to find a safe position nearby
            if (!findSafePosition(safeX, safeY, config->collisionRadius, gameMap, config->nonTraversableBlocks)) {
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
    }

    // Pathfinding logic
    std::vector<std::pair<float, float>> path = findPath(
        startPathX, startPathY,
        x, y,
        gameMap,
        config->collisionRadius,
        config->nonTraversableBlocks
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
            
            // Draw filled polygon
            glColor4f(0.0f, 1.0f, 0.0f, 0.3f); // Semi-transparent green
            glBegin(GL_POLYGON);
            for (const auto& screenPoint : screenShapePoints) {
                glVertex2f(screenPoint.first, screenPoint.second);
            }
            glEnd();

            // Draw polygon outline
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
            float top = entityScreenY + halfSide;

            // Draw filled square for collision radius
            glColor4f(0.0f, 1.0f, 0.0f, 0.3f); // Semi-transparent green
            glBegin(GL_QUADS);
            glVertex2f(left, bottom);   // Bottom-left
            glVertex2f(right, bottom);  // Bottom-right
            glVertex2f(right, top);     // Top-right
            glVertex2f(left, top);      // Top-left
            glEnd();

            // Draw square outline
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
        updateDirectionAndSprite(entity, elementName, config, dx, dy);
    }
    // For pathfinding, sprite direction is handled by handleWaypointArrival.

    // Check for collisions before moving the entity
    bool canMove = true;      if (config.canCollide) {
        // Calculate the new position after movement
        float newX = currentActualX + moveDx;
        float newY = currentActualY + moveDy;
          // Check if the new position would collide with any collidable element or map block
        bool collisionWithElement = wouldEntityCollideWithElement(config, newX, newY);
        bool collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap, config.nonTraversableBlocks);
        
        if (collisionWithElement || collisionWithMapBlock) {
            // Collision detected, don't move but keep trying to reach the target
            canMove = false;
            
            // Try to find a safe position directly
            float safeX = newX;
            float safeY = newY;
            
            // Add some safety buffer for better collision avoidance
            float safetyBuffer = 0.2f;
            float safeRadius = config.collisionRadius + safetyBuffer;
            
            // Try to find a safe position that takes into account entity-specific blocks
            if (findSafePosition(safeX, safeY, safeRadius, gameMap, config.nonTraversableBlocks)) {
                // Found a safe position, use it instead
                moveDx = safeX - currentActualX;
                moveDy = safeY - currentActualY;
                canMove = true;
                
                // Update direction based on actual movement
                float actualDx = safeX - currentActualX;
                float actualDy = safeY - currentActualY;
                // Update sprite if moving directly (not pathfinding) or if pathfinding sprite logic needs this
                if (!entity.usePathfinding) { // Or some other condition if pathfinding needs it
                    updateDirectionAndSprite(entity, elementName, config, actualDx, actualDy);
                }
            } else {
                // If we couldn't find a safe position, try alternative movement directions
                
                // Try moving only in Y direction (vertical movement)                newX = currentActualX;
                newY = currentActualY + moveDy;
                collisionWithElement = wouldEntityCollideWithElement(config, newX, newY);
                collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap, config.nonTraversableBlocks);
                
                if (!collisionWithElement && !collisionWithMapBlock) {
                    moveDx = 0; // Only move in Y
                    canMove = true;
                    // Force update sprite direction based on Y movement only
                    entity.lastDirection = (moveDy > 0) ? 0 : 1; // 0 = up, 1 = down
                    
                    // Set sprite phase directly based on direction
                    int phase = (entity.lastDirection == 0) ? config.spritePhaseWalkUp : config.spritePhaseWalkDown;
                    elementsManager.changeElementSpritePhase(elementName, phase);
                } else {                    // Then check moving only in X direction (horizontal movement)
                    newX = currentActualX + moveDx;
                    newY = currentActualY;
                    collisionWithElement = wouldEntityCollideWithElement(config, newX, newY);
                    collisionWithMapBlock = wouldCollideWithMapBlock(newX, newY, gameMap, config.nonTraversableBlocks);
                    
                    if (!collisionWithElement && !collisionWithMapBlock) {
                        moveDy = 0; // Only move in X
                        canMove = true;
                        // Force update sprite direction based on X movement only
                        entity.lastDirection = (moveDx > 0) ? 3 : 2; // 3 = right, 2 = left
                        
                        // Set sprite phase directly based on direction
                        int phase = (entity.lastDirection == 3) ? config.spritePhaseWalkRight : config.spritePhaseWalkLeft;
                        elementsManager.changeElementSpritePhase(elementName, phase);
                    }
                }            }
            
            if (!canMove) {
                // Entity is stuck, let's try to recalculate the path if using pathfinding
                if (entity.usePathfinding) {
                    std::cout << "Entity " << entity.instanceName << " encountered obstacle at (" 
                              << newX << ", " << newY << "), recalculating path..." << std::endl;
                      // Identify what's blocking the entity
                    bool hasElementCollision = wouldEntityCollideWithElement(config, newX, newY);
                    bool hasMapBlockCollision = wouldCollideWithMapBlock(newX, newY, gameMap, config.nonTraversableBlocks);
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
                        }
                    }
                    
                    if (hasMapBlockCollision) {
                        std::cout << "  - Map block collision detected with entity-specific non-traversable blocks" << std::endl;
                    }
                    
                    // Try to find a safe position first before recalculating
                    float safeX = currentActualX;
                    float safeY = currentActualY;
                    float safetyBuffer = 0.3f;
                    float safeRadius = config.collisionRadius + safetyBuffer;
                    
                    // Try with the entity-specific non-traversable blocks
                    bool foundSafePos = findSafePosition(safeX, safeY, safeRadius, gameMap, config.nonTraversableBlocks);
                    
                    if (foundSafePos) {
                        // Found a safe position, teleport there
                        elementsManager.changeElementCoordinates(elementName, safeX, safeY);
                        std::cout << "Found safe position at (" << safeX << ", " << safeY 
                                  << "), moving entity there before recalculating path" << std::endl;
                        
                        // Update current position
                        currentActualX = safeX;
                        currentActualY = safeY;
                    }
                    
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
                        }
                        // Recalculate the path from the current position
                        // Use an enlarged collision radius for safer paths
                        float pathPlanningRadius = config.collisionRadius + 0.3f;
                        std::vector<std::pair<float, float>> newPath = findPath(
                            curX, curY,
                            entity.targetX, entity.targetY,
                            gameMap,
                            pathPlanningRadius,
                            config.nonTraversableBlocks
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
                        } else {
                            // No path found, try with an alternative approach
                            // Find a safe position near the target first
                            float safeTargetX = entity.targetX;
                            float safeTargetY = entity.targetY;
                            
                            // Try to find a safe position near the target
                            if (findSafePosition(safeTargetX, safeTargetY, pathPlanningRadius, gameMap, config.nonTraversableBlocks)) {
                                // Found a safe position near the target, try pathfinding to it
                                newPath = findPath(
                                    curX, curY,
                                    safeTargetX, safeTargetY,
                                    gameMap,
                                    pathPlanningRadius,
                                    config.nonTraversableBlocks
                                );
                                
                                if (!newPath.empty()) {
                                    // Found a path to the safe target position
                                    entity.path = newPath;
                                    entity.currentPathIndex = 0;
                                    entity.targetX = safeTargetX;
                                    entity.targetY = safeTargetY;
                                    std::cout << "Found path to safe target with " << newPath.size() << " waypoints" << std::endl;
                                    
                                    // Update direction
                                    if (entity.path.size() > 0) {
                                        float nextX = entity.path[0].first;
                                        float nextY = entity.path[0].second;
                                        float dirX = nextX - curX;
                                        float dirY = nextY - curY;
                                        
                                        // Update direction and sprite
                                        // This call was missing arguments
                                        // updateDirectionAndSprite(dirX, dirY);
                                        // Sprite for pathfinding is set by handleWaypointArrival
                                    }
                                    
                                    // Make sure we're moving next frame
                                    canMove = true;
                                    return; // Skip the rest of this update cycle
                                }
                            }
                        }
                    }
                    // If no path found or not using pathfinding, just stop and try again next frame
                    std::cout << "Entity " << entity.instanceName << " encountered obstacle at (" 
                              << newX << ", " << newY << ")" << std::endl;
                }
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
        }
    } else { // canMove is false (collision)
        // We have a collision but we want to keep trying to reach the target
        // Update the sprite direction to match the intended movement even if we can\'t actually move
        // This call was missing arguments
        // updateDirectionAndSprite(dx, dy);
        // For pathfinding, sprite is set by handleWaypointArrival. For direct, it was set earlier.
        // If it's a direct movement and we are here, it means the initial check passed, but findSafePosition failed or wasn't used.
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

// Helper method to get an entity's type name by its instance name
std::string EntitiesManager::getEntityType(const std::string& instanceName) const {
    auto it = entities.find(instanceName);
    if (it != entities.end()) {
        return it->second.typeName;
    }
    return ""; // Empty string if entity not found
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
    }    // Check if entity is in a collision state
    bool collisionWithElement = wouldEntityCollideWithElement(*config, x, y);
    bool collisionWithMapBlock = wouldCollideWithMapBlock(x, y, gameMap, config->nonTraversableBlocks);
        
    if (collisionWithElement || collisionWithMapBlock) {        // Add a safety buffer for better collision avoidance
        float safetyBuffer = 0.2f;
        float safeRadius = config->collisionRadius + safetyBuffer;
        
        // Entity is stuck, try to find a safe position with entity-specific non-traversable blocks
        if (findSafePosition(x, y, safeRadius, gameMap, config->nonTraversableBlocks)) {
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
    if (config->canCollide) {        // Check for collision at the teleport destination
        bool collisionWithElement = wouldEntityCollideWithElement(*config, targetX, targetY);
        bool collisionWithMapBlock = wouldCollideWithMapBlock(targetX, targetY, gameMap, config->nonTraversableBlocks);
        
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
            if (findSafePosition(targetX, targetY, config->collisionRadius, gameMap, config->nonTraversableBlocks)) {
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
bool EntitiesManager::handleWaypointArrival(Entity& entity, const std::string& elementName, const EntityConfiguration& config, float segmentStartX, float segmentStartY) {
    if (entity.currentPathIndex >= entity.path.size()) {
        // No valid next target waypoint in the path array.
        // This might occur if path is exhausted or index is otherwise invalid.
        // std::cout << "hWA: currentPathIndex " << entity.currentPathIndex << " is invalid for path size " << entity.path.size() << ". No sprite update." << std::endl;
        return false;
    }

    const auto& targetWaypoint = entity.path[entity.currentPathIndex]; // This is the END of the current segment
    float dxToWaypoint = targetWaypoint.first - segmentStartX;
    float dyToWaypoint = targetWaypoint.second - segmentStartY;

    const float epsilon = 0.001f; // A small epsilon for floating point comparisons

    if (std::abs(dxToWaypoint) < epsilon && std::abs(dyToWaypoint) < epsilon) {
        // Segment is zero length (segmentStart is effectively identical to targetWaypoint).
        // This means we can't determine a direction from it.
        // This could happen if path contains duplicate points, or if start/goal were same.
        // std::cout << "hWA: Zero length segment detected for " << elementName << " from (" << segmentStartX << "," << segmentStartY << ") to (" << targetWaypoint.first << "," << targetWaypoint.second << "). No sprite change." << std::endl;
        // Keep current sprite, don't change phase.
        return false; 
    }

    // Determine sprite direction based on the segment from segmentStartX,Y to targetWaypoint
    float dirX = dxToWaypoint;
    float dirY = dyToWaypoint;
    
    // Normalize direction vector
    float length = std::sqrt(dirX * dirX + dirY * dirY);
    if (length > 0.0001f) {
        dirX /= length;
        dirY /= length;
    }
    
    // Store this as our current segment direction
    entity.lastSegmentDirection = {dirX, dirY};
    
    // Update sprite direction immediately based on the new segment direction
    int newDirection;
    if (std::abs(dirX) > std::abs(dirY)) {
        // Horizontal movement dominates
        newDirection = (dirX > 0) ? 3 : 2;  // 3 = right, 2 = left
    } else {
        // Vertical movement dominates
        newDirection = (dirY > 0) ? 0 : 1;  // 0 = up, 1 = down
    }
    
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
    
    return true;
}