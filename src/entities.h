#pragma once

#include "elementsOnMap.h"
#include <string>
#include <vector>
#include <map>
#include <cmath> // For distance calculations

// Enum for walk types
enum class WalkType {
    NORMAL,
    SPRINT
};

// Struct to hold entity configuration
struct EntityConfiguration {
    std::string typeName;
    ElementTextureName textureName;
    float scale = 1.0f;
    
    // Default sprite configuration
    int defaultSpriteSheetPhase = 0;
    int defaultSpriteSheetFrame = 0;
    float defaultAnimationSpeed = 5.0f;
    
    // Walking animation phases for different directions
    int spritePhaseWalkUp = 0;
    int spritePhaseWalkDown = 1;
    int spritePhaseWalkLeft = 2;
    int spritePhaseWalkRight = 3;
    
    // Movement speeds
    float normalWalkingSpeed = 2.0f;
    float normalWalkingAnimationSpeed = 8.0f;
    float sprintWalkingSpeed = 4.0f;
    float sprintWalkingAnimationSpeed = 12.0f;
    
    // Collision settings
    bool canCollide = true;
    float collisionRadius = 0.4f;
};

// Struct to hold entity instance data
struct Entity {
    std::string instanceName;
    std::string typeName;
    
    // Movement target - only used when walking to a location
    bool isWalking = false;
    float targetX = 0.0f;
    float targetY = 0.0f;
    WalkType walkType = WalkType::NORMAL;
    
    // Direction tracking
    int lastDirection = 1; // 0 = Up, 1 = Down, 2 = Left, 3 = Right
};

// EntitiesManager - Manages all entities in the game
class EntitiesManager {
public:
    EntitiesManager();
    ~EntitiesManager();
    
    // Add a new entity configuration
    void addConfiguration(const EntityConfiguration& config);
    
    // Place a new entity on the map
    bool placeEntity(const std::string& instanceName, const std::string& typeName, float x, float y);
    
    // Move an entity by a delta
    bool moveEntity(const std::string& instanceName, float deltaX, float deltaY);
    
    // Walk an entity to specific coordinates
    bool walkEntityToCoordinates(const std::string& instanceName, float x, float y, WalkType walkType = WalkType::NORMAL);
    
    // Stop an entity's walking
    bool stopEntityWalk(const std::string& instanceName);
    
    // Change an entity's walking state
    bool changeEntityWalkingState(const std::string& instanceName, WalkType walkType);
    
    // Update all entities (called once per frame)
    void update(double deltaTime);
    
private:
    std::map<std::string, EntityConfiguration> configurations;
    std::map<std::string, Entity> entities;
    
    // Helper methods
    const EntityConfiguration* getConfiguration(const std::string& typeName) const;
    Entity* getEntity(const std::string& instanceName);
    
    // Update entity walking (internal method)
    void updateEntityWalking(Entity& entity, const EntityConfiguration& config, double deltaTime);
    
    // Calculate element name for an entity instance
    std::string getElementName(const std::string& instanceName) const {
        return "entity_" + instanceName;
    }
};

// Global instance
extern EntitiesManager entitiesManager;