#include "entities.h"
#include "collision.h"
#include <iostream>
#include <cmath>

// Global instance definition
EntitiesManager entitiesManager;

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
        x, y,
        0.0f,  // rotation
        config->defaultSpriteSheetPhase,
        config->defaultSpriteSheetFrame,
        false,  // not animated initially
        config->defaultAnimationSpeed,
        AnchorPoint::BOTTOM_CENTER  // Default anchor for entities
    );
    
    // Add the entity to our entities map
    entities[instanceName] = entity;
    
    std::cout << "Placed entity: " << instanceName << " (type: " << typeName << ") at (" 
              << x << ", " << y << ")" << std::endl;
    return true;
}

bool EntitiesManager::moveEntity(const std::string& instanceName, float deltaX, float deltaY) {
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
    
    // Calculate new position
    float newX = currentX + deltaX;
    float newY = currentY + deltaY;
    
    // Check collision if enabled
    if (config->canCollide && wouldCollideWithElement(newX, newY, config->collisionRadius)) {
        std::cout << "Entity " << instanceName << " would collide at (" << newX << ", " << newY << ")" << std::endl;
        return false;
    }
    
    // Update direction based on movement
    if (deltaX != 0.0f || deltaY != 0.0f) {
        // Determine the primary direction (up, down, left, right)
        if (std::abs(deltaX) > std::abs(deltaY)) {
            // Horizontal movement dominates
            entity->lastDirection = (deltaX > 0) ? 3 : 2;  // 3 = right, 2 = left
        } else {
            // Vertical movement dominates
            entity->lastDirection = (deltaY > 0) ? 0 : 1;  // 0 = up, 1 = down
        }
        
        // Set the appropriate sprite phase based on direction
        int phase;
        switch (entity->lastDirection) {
            case 0: phase = config->spritePhaseWalkUp; break;
            case 1: phase = config->spritePhaseWalkDown; break;
            case 2: phase = config->spritePhaseWalkLeft; break;
            case 3: phase = config->spritePhaseWalkRight; break;
            default: phase = config->defaultSpriteSheetPhase; break;
        }
        
        // Change the sprite phase
        elementsManager.changeElementSpritePhase(elementName, phase);
    }
    
    // Move the element
    return elementsManager.moveElement(elementName, deltaX, deltaY);
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

bool EntitiesManager::stopEntityWalk(const std::string& instanceName) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }
    
    // Stop walking
    entity->isWalking = false;
    
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
    
    // Calculate distance to target
    float dx = entity.targetX - currentX;
    float dy = entity.targetY - currentY;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Stop if we're close enough to the target
    float stopThreshold = 0.1f; // Stop when within 0.1 distance units
    if (distance <= stopThreshold) {
        // We've reached the target
        entity.isWalking = false;
        
        // Snap to exact target position
        elementsManager.changeElementCoordinates(elementName, entity.targetX, entity.targetY);
        
        // Disable animation
        elementsManager.changeElementAnimationStatus(elementName, false);
        
        // Reset to frame 0 (standing frame)
        elementsManager.changeElementSpriteFrame(elementName, 0);
        
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
    
    // Move the entity
    if (!elementsManager.moveElement(elementName, moveDx, moveDy)) {
        // If movement failed (e.g. due to collision), stop walking
        entity.isWalking = false;
        elementsManager.changeElementAnimationStatus(elementName, false);
        elementsManager.changeElementSpriteFrame(elementName, 0);
        std::cout << "Entity " << entity.instanceName << " stopped walking due to movement failure" << std::endl;
    }
}