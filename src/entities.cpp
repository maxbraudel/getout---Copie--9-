#include "entities.h"
#include "entityNameOps.h"
#include "entityBehaviors.h"
#include "collision.h"
#include "map.h" // Adding for gameMap access
// #include "pathfinding.h" // Commented out to avoid conflicts with new async pathfinding system
#include "globals.h" // For GRID_SIZE
#include "debug.h" // For isShowingCollisionBoxes function
#include "asyncPathfinding.h"
#include <iostream>
#include <cmath>
#include <limits>
#include <random>

// Utility functions for EntityName enum
std::string entityNameToString(EntityName entityName) {
    switch (entityName) {
        case EntityName::ANTAGONIST: return "antagonist";
        case EntityName::PLAYER: return "player";
        default: return "unknown";
    }
}

EntityName stringToEntityName(const std::string& str) {
    if (str == "antagonist") return EntityName::ANTAGONIST;
    if (str == "player") return EntityName::PLAYER;
    return EntityName::ANTAGONIST; // Default fallback
}

// Global async pathfinder instance (separate from pathfinding.h's AsyncPathfinder)
static AsyncEntityPathfinder* g_entityAsyncPathfinder = nullptr;

// Static vector of predefined entity types
static std::vector<EntityInfo> entityTypes;

// Function to initialize entity types
static void initializeEntityTypes() {
    if (!entityTypes.empty()) {
        return; // Already initialized
    }
    
    // CRASH FIX: Reserve memory to prevent reallocations during initialization
    entityTypes.reserve(10); // Reasonable initial capacity
    
    try {        // Antagonist entity configuration
        EntityInfo antagonist;
        antagonist.type = EntityName::ANTAGONIST;
        antagonist.textureName = ElementName::ANTAGONIST1;
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
        antagonist.sprintWalkingSpeed = 3.0f;
        antagonist.sprintWalkingAnimationSpeed = 12.0f;    // Collision settings
        antagonist.canCollide = true;
        antagonist.collisionShapePoints = {
            {-0.2f, -0.1f}, {0.2f, -0.1f}, {0.2f, 0.1f}, {-0.2f, 0.1f}
        };    // Granular collision control - antagonist avoids trees but can move through player
        // For testing: Leave lists empty to allow movement through all elements
        antagonist.avoidanceElements = {
            // Empty - no elements to avoid for pathfinding
            ElementName::COCONUT_TREE_1,
            ElementName::COCONUT_TREE_2,
            ElementName::COCONUT_TREE_3,
        };    
        antagonist.collisionElements = {
            // Empty - no elements to collide with during movement
            ElementName::COCONUT_TREE_1,
            ElementName::COCONUT_TREE_2,
            ElementName::COCONUT_TREE_3,
        };
          // Block collision configuration - Antagonist avoids water during pathfinding and movement
        antagonist.avoidanceBlocks = {
            TextureName::WATER_0,
            TextureName::WATER_1,
            TextureName::WATER_2,
            TextureName::WATER_3,
            TextureName::WATER_4
        };
        
        antagonist.collisionBlocks = {
            TextureName::WATER_0,
            TextureName::WATER_1,
            TextureName::WATER_2,
            TextureName::WATER_3,
            TextureName::WATER_4
        };
          // Map boundary control settings
        antagonist.offMapAvoidance = true; // Antagonist pathfinding avoids map borders
        antagonist.offMapCollision = true; // Antagonist collides with map borders
          // Automatic behavior configuration
        antagonist.automaticBehaviors = true; // Enable automatic behaviors for antagonist
        antagonist.passiveState = true; // Enable passive state random walking
        antagonist.passiveStateWalkingRadius = 8.0f; // Walking radius for random walks
        antagonist.passiveStateRandomWalkTriggerTimeIntervalMin = 3.0f; // Min time between walks (seconds)
        antagonist.passiveStateRandomWalkTriggerTimeIntervalMax = 10.0f; // Max time between walks (seconds)        // Alert state configuration
        antagonist.alertState = false; // Enable alert state behavior
        antagonist.alertStateStartRadius = 0.0f; // Inner radius - immediate alert when entities get this close
        antagonist.alertStateEndRadius = 8.0f; // Outer radius - watch for entities within this range
        antagonist.alertStateEntitiesList = {EntityName::PLAYER}; // Watch for player entity type
        
        // Flee state configuration - antagonist flees from player
        antagonist.fleeState = true; // Enable flee state behavior
        antagonist.fleeStateStartRadius = 0.0f; // Inner radius - immediate flee when entities get this close
        antagonist.fleeStateEndRadius = 4.0f; // Outer radius - flee from entities within this range
        antagonist.fleeStateEntitiesList = {EntityName::PLAYER}; // Flee from player entity type
        antagonist.fleeStateRunning = true; // Use sprint speed when fleeing
        antagonist.fleeStateMinDistance = 8.0f; // Minimum distance to flee
        antagonist.fleeStateMaxDistance = 15.0f; // Maximum distance to flee
        
        // Add to the list
        entityTypes.push_back(antagonist);        EntityInfo player;
        player.type = EntityName::PLAYER;
        player.textureName = ElementName::CHARACTER1;
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
        player.normalWalkingSpeed = 3.5f;
        player.normalWalkingAnimationSpeed = 11.0f;
        player.sprintWalkingSpeed = 5.0f;
        player.sprintWalkingAnimationSpeed = 20.0f;    // Collision settings
        player.canCollide = true;
        player.collisionShapePoints = {
            {-0.2f, -0.1f}, {0.2f, -0.1f}, {0.2f, 0.1f}, {-0.2f, 0.1f}
        };    // Granular collision control - player avoids trees and other characters
        // For testing: Leave lists empty to allow movement through all elements
        player.avoidanceElements = {
            // Empty - no elements to avoid for pathfinding
        };    player.collisionElements = {
            // Empty - no elements to collide with during movement
            // Empty - no elements to avoid for pathfinding
            ElementName::COCONUT_TREE_1,
            ElementName::COCONUT_TREE_2,
            ElementName::COCONUT_TREE_3,
            
        };
          // Block collision configuration - Player demonstrates different behavior than antagonist
        // Player can walk through sand but avoids water for pathfinding
        // This shows how entities can have different block interaction behaviors
        player.avoidanceBlocks = {
            TextureName::WATER_0,
            TextureName::WATER_1,
            TextureName::WATER_2,
            TextureName::WATER_3,
            TextureName::WATER_4
        };
        
        // Player physically collides with deep water but can walk through shallow water
        player.collisionBlocks = {
            TextureName::WATER_0,
            TextureName::WATER_1,
            TextureName::WATER_2,
            TextureName::WATER_3,
            TextureName::WATER_4 // Only deep water blocks movement
        };          // Map boundary control settings
        player.offMapAvoidance = true; // Player pathfinding avoids map borders
        player.offMapCollision = true; // Player collides with map borders
        

        // Add to the list
        entityTypes.push_back(player);


    } catch (const std::exception& e) {
        std::cerr << "CRASH FIX: Exception during entity initialization: " << e.what() << std::endl;
        entityTypes.clear(); // Clear on error to prevent corruption
    }
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
    initializeAsyncPathfinding();
}

EntitiesManager::~EntitiesManager() {
    // Destructor
    shutdownAsyncPathfinding();
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
    EntityName entityType = stringToEntityName(typeName);
    return getConfiguration(entityType);
}

const EntityConfiguration* EntitiesManager::getConfiguration(EntityName entityType) const {
    auto it = configurations.find(entityType);
    if (it != configurations.end()) {
        return &(it->second);
    }
    return nullptr;
}

void EntitiesManager::addConfiguration(const EntityConfiguration& config) {
    // Add or replace the configuration
    configurations[config.type] = config;
    std::cout << "Added entity configuration: " << entityNameToString(config.type) << std::endl;
}

bool EntitiesManager::placeEntityByType(const std::string& instanceName, const std::string& typeName, float x, float y) {
    // Convert string to EntityName and call the enum version
    EntityName entityType = stringToEntityName(typeName);
    return placeEntityByType(instanceName, entityType, x, y);
}

bool EntitiesManager::placeEntityByType(const std::string& instanceName, EntityName entityType, float x, float y) {
    // Find the entity type in our predefined list
    for (const auto& entityInfo : entityTypes) {
        if (entityInfo.type == entityType) {
            // Create configuration from the entity info
            EntityConfiguration config(entityInfo);
            // Add the configuration if it doesn't exist yet
            if (!getConfiguration(entityType)) {
                addConfiguration(config);
            }
            // Place the entity
            return placeEntity(instanceName, entityType, x, y);
        }
    }
    
    std::cerr << "Entity type not found: " << entityNameToString(entityType) << std::endl;
    return false;
}

bool EntitiesManager::placeEntity(const std::string& instanceName, const std::string& typeName, float x, float y) {
    // Convert string to EntityName and call the enum version
    EntityName entityType = stringToEntityName(typeName);
    return placeEntity(instanceName, entityType, x, y);
}

bool EntitiesManager::placeEntity(const std::string& instanceName, EntityName entityType, float x, float y) {
    // Check if the configuration exists
    const EntityConfiguration* config = getConfiguration(entityType);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entityNameToString(entityType) << std::endl;
        return false;
    }// COLLISION RESOLUTION INTEGRATION
    // Check if entity spawns in a collision area and resolve it
    float safeX = x;
    float safeY = y;
    bool needsSafePosition = false;
    
    // Check if the entity would be stuck at the requested position
    if (config->canCollide && 
        (wouldEntityCollideWithElementsGranular(*config, x, y, false) || 
         wouldEntityCollideWithBlocksGranular(*config, x, y, false))) {
        
        std::cout << "Entity " << instanceName << " would spawn inside collision area at (" 
                  << x << ", " << y << ") - attempting collision resolution..." << std::endl;
        
        // Try to find a safe position nearby
        if (resolveEntityCollisionStuck(instanceName, safeX, safeY, *config, gameMap)) {
            needsSafePosition = true;
            std::cout << "Found safe spawn position for " << instanceName 
                      << " at (" << safeX << ", " << safeY << ")" << std::endl;
        } else {
            std::cout << "Warning: Could not find safe spawn position for " << instanceName 
                      << " - placing at requested coordinates (" << x << ", " << y << ")" << std::endl;
            safeX = x;
            safeY = y;
        }
    }
      // Generate the element name
    std::string elementName = getElementName(instanceName);
      // Create an Entity object
    Entity entity;
    entity.instanceName = instanceName;
    entity.type = entityType;
    
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
    
    // Initialize entity behaviors
    Entity& createdEntity = entities[instanceName];
    entityBehaviorManager.initializeEntityBehavior(createdEntity, *config);
      if (needsSafePosition && (safeX != x || safeY != y)) {
        std::cout << "Entity " << instanceName << " placed with collision resolution - moved from (" 
                  << x << ", " << y << ") to (" << safeX << ", " << safeY << ")" << std::endl;
    } else {
        std::cout << "Placed entity: " << instanceName << " (type: " << entityNameToString(entityType) << ") at (" 
                << safeX << ", " << safeY << ")" << std::endl;
    }
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
    const EntityConfiguration* config = getConfiguration(entity->type);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entityNameToString(entity->type) << std::endl;
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
      // Check if destination would cause entity collision shape to overlap with map boundaries
    if (config->offMapCollision && wouldEntityCollideWithMapBounds(*config, x, y)) {
        std::cout << "Cannot move entity " << instanceName << " to (" << x << ", " << y 
                  << ") - destination would cause collision with map boundaries" << std::endl;
        return false;
    }
    
    // Check if destination would cause entity collision shape to overlap with collision/avoidance blocks or elements
    if (config->canCollide && 
        (wouldEntityCollideWithElementsGranular(*config, x, y, true) || 
         wouldEntityCollideWithBlocksGranular(*config, x, y, true))) {
        std::cout << "Cannot move entity " << instanceName << " to (" << x << ", " << y 
                  << ") - inaccessible destination (would cause collision with blocks/elements)" << std::endl;
        return false;
    }
    
    // Check if we're already close to the destination
    float dx = x - startPathX;
    float dy = y - startPathY;
    float distance = std::sqrt(dx * dx + dy * dy);
    if (distance < 0.1f) {
        std::cout << "Entity " << instanceName << " is already at destination" << std::endl;
        return true;
    }
      // Check async pathfinding availability
    if (!g_entityAsyncPathfinder) {
        std::cerr << "Async pathfinder not initialized" << std::endl;
        return false;
    }
      // Cancel any existing pathfinding request for this entity
    if (entity->pathfindingRequestId > 0) {
        g_entityAsyncPathfinder->cancelPathfindingRequest(entity->instanceName);
        entity->pathfindingRequestId = 0;
    }
    
    // Keep current movement if entity is already walking, otherwise start waiting
    // This prevents the entity from stopping while new path is being calculated
    if (!entity->isWalking) {
        entity->isWaitingForPath = true;
        elementsManager.changeElementAnimationStatus(elementName, false);
    } else {
        // Entity keeps moving on current path while new path is calculated
        entity->isWaitingForPath = true;
    }
      // Submit async pathfinding request
    int requestId = g_entityAsyncPathfinder->requestPathfinding(instanceName, startPathX, startPathY, x, y, *config, walkType);
    
    if (requestId > 0) {
        entity->pathfindingRequestId = requestId;
        entity->isWaitingForPath = true;
        entity->targetX = x;
        entity->targetY = y;
        entity->walkType = walkType;
        entity->lastPathRequest = std::chrono::steady_clock::now();
        
        std::cout << "Entity " << instanceName << " submitted async pathfinding request (ID: " << requestId 
                  << ") to (" << x << ", " << y << ") with "
                  << ((walkType == WalkType::NORMAL) ? "normal" : "sprint") << " speed" << std::endl;
        return true;
    } else {
        std::cerr << "Failed to submit pathfinding request for entity: " << instanceName << std::endl;
        return false;
    }
}

bool EntitiesManager::findNearestSafePlaceFromCoordinatesForEntity(const std::string& instanceName, float x, float y, float& safeX, float& safeY) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }

    // Get the configuration
    const EntityConfiguration* config = getConfiguration(entity->type);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entityNameToString(entity->type) << std::endl;
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

    // First check if the target coordinates are already accessible
    // Check if destination would cause entity collision shape to overlap with map boundaries
    if (config->offMapCollision && wouldEntityCollideWithMapBounds(*config, x, y)) {
        std::cout << "Target coordinates (" << x << ", " << y << ") would cause collision with map boundaries" << std::endl;
    }
    // Check if destination would cause entity collision shape to overlap with collision/avoidance blocks or elements
    else if (config->canCollide && 
        (wouldEntityCollideWithElementsGranular(*config, x, y, true) || 
         wouldEntityCollideWithBlocksGranular(*config, x, y, true))) {
        std::cout << "Target coordinates (" << x << ", " << y << ") would cause collision with blocks/elements" << std::endl;
    } else {
        // Target coordinates are accessible
        safeX = x;
        safeY = y;
        std::cout << "Entity " << instanceName << " can reach target (" << x << ", " << y << ") - no adjustment needed" << std::endl;
        return true;
    }

    std::cout << "Entity " << instanceName << " cannot reach target (" << x << ", " << y << ") - searching for nearest accessible coordinates..." << std::endl;

    // Target is not accessible, search for nearest accessible coordinates using spiral pattern
    const float searchStep = 0.5f;
    const float maxSearchRadius = 50.0f;

    for (float radius = searchStep; radius <= maxSearchRadius; radius += searchStep) {
        // Try points in a circle around the target coordinates
        const int numDirections = std::max(8, static_cast<int>(radius * 4)); // More directions for larger radii

        for (int i = 0; i < numDirections; i++) {
            float angle = (i * 2.0f * M_PI) / numDirections;
            float testX = x + radius * std::cos(angle);
            float testY = y + radius * std::sin(angle);

            // Check if this position is within map bounds
            if (testX < 0 || testX >= GRID_SIZE || testY < 0 || testY >= GRID_SIZE) {
                continue;
            }

            // Check if this position is valid (not colliding)
            if (config->offMapCollision && wouldEntityCollideWithMapBounds(*config, testX, testY)) {
                continue;
            }

            if (config->canCollide && 
                (wouldEntityCollideWithElementsGranular(*config, testX, testY, true) || 
                 wouldEntityCollideWithBlocksGranular(*config, testX, testY, true))) {
                continue;
            }

            // Found an accessible safe position!
            safeX = testX;
            safeY = testY;
            float distance = std::sqrt((testX - x) * (testX - x) + (testY - y) * (testY - y));
            std::cout << "Found nearest safe accessible position for entity " << instanceName 
                      << " at (" << safeX << ", " << safeY << ") - distance from target: " << distance << std::endl;
            return true;
        }
    }

    // No safe position found, return current position as fallback
    safeX = currentX;
    safeY = currentY;
    std::cout << "No accessible position found within search radius - returning current position (" 
              << safeX << ", " << safeY << ") for entity " << instanceName << std::endl;
    return false;
}

bool EntitiesManager::findRandomSafePointAroundTheEntity(const std::string& instanceName, float radius, float& randomX, float& randomY) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found: " << instanceName << std::endl;
        return false;
    }

    // Get the configuration
    const EntityConfiguration* config = getConfiguration(entity->type);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entityNameToString(entity->type) << std::endl;
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

    // Set up random number generation
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> radiusDist(0.0f, radius);

    // Try to find a random accessible point within the radius
    const int maxAttempts = 100;
    
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        // Generate random angle and distance
        float angle = angleDist(gen);
        float distance = radiusDist(gen);
        
        // Calculate test position
        float testX = currentX + distance * std::cos(angle);
        float testY = currentY + distance * std::sin(angle);

        // Check if this position is within map bounds
        if (testX < 0 || testX >= GRID_SIZE || testY < 0 || testY >= GRID_SIZE) {
            continue;
        }

        // Check if this position is accessible (not colliding)
        if (config->offMapCollision && wouldEntityCollideWithMapBounds(*config, testX, testY)) {
            continue;
        }

        if (config->canCollide && 
            (wouldEntityCollideWithElementsGranular(*config, testX, testY, true) || 
             wouldEntityCollideWithBlocksGranular(*config, testX, testY, true))) {
            continue;
        }

        // Found an accessible random position!
        randomX = testX;
        randomY = testY;
        float actualDistance = std::sqrt((testX - currentX) * (testX - currentX) + (testY - currentY) * (testY - currentY));
        std::cout << "Found random accessible position for entity " << instanceName 
                  << " at (" << randomX << ", " << randomY << ") - distance: " << actualDistance 
                  << " (attempt " << (attempt + 1) << ")" << std::endl;
        return true;
    }

    // No accessible random position found within attempts
    std::cout << "No accessible random position found within " << maxAttempts 
              << " attempts for entity " << instanceName << " with radius " << radius << std::endl;
    return false;
}

bool EntitiesManager::walkEntityWithPathFindingToRandomRadiusTarget(const std::string& instanceName, float radius, WalkType walkType) {
    // Find a random safe point around the entity
    float randomX, randomY;
    if (!findRandomSafePointAroundTheEntity(instanceName, radius, randomX, randomY)) {
        std::cerr << "Failed to find random accessible target for entity: " << instanceName << std::endl;
        return false;
    }

    // Use the existing pathfinding function to walk to the random target
    std::cout << "Entity " << instanceName << " moving to random target (" << randomX << ", " << randomY << ")" << std::endl;
    return walkEntityWithPathfinding(instanceName, randomX, randomY, walkType);
}

void EntitiesManager::update(double deltaTime) {
    // CRASH FIX: Add comprehensive protection for entity updates
    try {
        // Process async pathfinding results first
        processAsyncPathfindingResults();
        
        // CRASH FIX: Collect entity names first to avoid iterator invalidation
        std::vector<std::string> walkingEntityNames;
        walkingEntityNames.reserve(entities.size());
        
        for (const auto& pair : entities) {
            if (pair.second.isWalking) {
                walkingEntityNames.push_back(pair.first);
            }
        }
        
        // Update all walking entities using safe name-based iteration
        for (const std::string& instanceName : walkingEntityNames) {
            // CRASH FIX: Verify entity still exists
            auto entityIt = entities.find(instanceName);
            if (entityIt == entities.end()) {
                std::cout << "WARNING: Walking entity " << instanceName << " no longer exists during update" << std::endl;
                continue;
            }
            
            Entity& entity = entityIt->second;
            
            // Skip if entity is no longer walking (state might have changed)
            if (!entity.isWalking) {
                continue;
            }
            
            // Get the configuration for this entity
            const EntityConfiguration* config = getConfiguration(entity.type);
            if (!config) {
                std::cerr << "Error: Cannot find configuration for entity: " << entity.instanceName << std::endl;
                entity.isWalking = false; // Stop walking due to error
                continue;
            }
            
            // Update the entity's walking animation with additional safety
            try {
                updateEntityWalking(entity, *config, deltaTime);
            } catch (const std::exception& e) {
                std::cerr << "CRITICAL: Exception updating entity " << instanceName << ": " << e.what() << std::endl;
                // Stop entity to prevent further crashes
                entity.isWalking = false;
            } catch (...) {
                std::cerr << "CRITICAL: Unknown exception updating entity " << instanceName << std::endl;
                // Stop entity to prevent further crashes
                entity.isWalking = false;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception in EntitiesManager::update: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "CRITICAL: Unknown exception in EntitiesManager::update!" << std::endl;
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
        const EntityConfiguration* config = getConfiguration(entity.type);
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

// CRASH FIX: Safe entity existence check
bool EntitiesManager::entityExists(const std::string& instanceName) const {
    return entities.find(instanceName) != entities.end();
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
    // CRASH FIX: Validate entity state before processing
    if (entity.instanceName.empty()) {
        std::cerr << "ERROR: Entity has empty instance name in updateEntityWalking" << std::endl;
        entity.isWalking = false;
        return;
    }
    
    // Get the element name
    std::string elementName = getElementName(entity.instanceName);
    
    // CRASH FIX: Verify element exists before processing
    if (!elementsManager.elementExists(elementName)) {
        std::cerr << "ERROR: Element " << elementName << " for entity " << entity.instanceName << " no longer exists" << std::endl;
        entity.isWalking = false;
        return;
    }
    
    // Get current position
    float currentActualX, currentActualY;
    if (!elementsManager.getElementPosition(elementName, currentActualX, currentActualY)) {
        std::cerr << "Error getting position for entity: " << entity.instanceName << std::endl;
        entity.isWalking = false; // Stop walking due to error
        return;
    }auto updateDirectionAndSprite = [&](Entity& ent, const std::string& elName, const EntityConfiguration& cfg, float dirX, float dirY) {
        // Only process if there's actual movement
        if (dirX == 0 && dirY == 0) {
            return; // No movement, keep current direction
        }
          // DEBUG: Log direction calculation for antagonist entities
        if (ent.type == EntityName::ANTAGONIST) {
            std::cout << "DEBUG: Non-pathfinding antagonist " << ent.instanceName 
                      << " direction calc: dirX=" << dirX << ", dirY=" << dirY 
                      << " (abs: " << std::abs(dirX) << ", " << std::abs(dirY) << ")" << std::endl;
        }
        
        // Use a smaller threshold to be more responsive to direction changes
        const float directionThreshold = 0.05f;
          // Determine the primary direction (up, down, left, right)
        int newDirection;
        if (std::abs(dirX) >= std::abs(dirY)) {
            // Horizontal movement dominates or is equal (prefer horizontal for diagonal movement)
            newDirection = (dirX > 0) ? 3 : 2;  // 3 = right, 2 = left
        } else {
            // Vertical movement dominates
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
          // DEBUG: Log sprite phase changes for antagonist entities
        if (ent.type == EntityName::ANTAGONIST) {
            std::cout << "DEBUG: Non-pathfinding antagonist " << ent.instanceName 
                      << " direction=" << newDirection << " -> phase=" << phase 
                      << " (right should be dir=3->phase=" << cfg.spritePhaseWalkRight << ")" << std::endl;
        }
        
        // Change the sprite phase
        elementsManager.changeElementSpritePhase(elName, phase);
    };
    
    float targetX = 0.0f, targetY = 0.0f;
    float dx = 0.0f, dy = 0.0f, distance = 0.0f;    if (entity.usePathfinding && !entity.path.empty()) {
        // CRASH FIX: Validate path state before accessing
        if (entity.currentPathIndex >= entity.path.size()) {
            std::cerr << "WARNING: Entity " << entity.instanceName << " has invalid path index "                      << entity.currentPathIndex << " >= " << entity.path.size() << std::endl;
            // Reset to valid state
            entity.currentPathIndex = std::min(entity.currentPathIndex, entity.path.size() > 0 ? entity.path.size() - 1 : 0);
        }
        
        // Ensure currentPathIndex is valid before accessing path
        if (entity.currentPathIndex < entity.path.size()) { // Path index must be < size to be a valid target
            const auto& waypoint = entity.path[entity.currentPathIndex]; // Target is path[currentPathIndex]
            targetX = waypoint.first;
            targetY = waypoint.second;
        } else {
            // Path ended (currentPathIndex == path.size()), entity should be stopping or at final target.
            // Target the final destination stored in entity.targetX,Y
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
    }    // Stop if we\'re close enough to the target
    float stopThreshold = 0.1f; // Stop when within 0.1 distance units
    if (distance <= stopThreshold && 
        ((!entity.usePathfinding) || 
        (entity.usePathfinding && entity.currentPathIndex >= entity.path.size()))) {
        // We've reached the final target
        entity.isWalking = false;
          // Always use current position as final position to prevent teleportation
        float finalX = currentActualX;
        float finalY = currentActualY;
        
        std::cout << "Entity " << entity.instanceName << " stopping naturally at (" 
                  << finalX << ", " << finalY << ") - target was (" 
                  << entity.targetX << ", " << entity.targetY << "), distance: " << distance << std::endl;
        
        // Move to the final position (usually current position, preventing teleportation)
        elementsManager.changeElementCoordinates(elementName, finalX, finalY);
        
        // Disable animation
        elementsManager.changeElementAnimationStatus(elementName, false);
        
        // Reset to frame 0 (standing frame)
        elementsManager.changeElementSpriteFrame(elementName, 0);
        
        // Clear the path
        if (entity.usePathfinding) {
            entity.path.clear();
            entity.currentPathIndex = 0;
        }
        
        std::cout << "Entity " << entity.instanceName << " reached target area - final position (" 
                  << finalX << ", " << finalY << ")" << std::endl;
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
        
        // Also check for collision with blocks using granular block collision control
        bool collisionWithBlock = wouldEntityCollideWithBlocksGranular(config, newX, newY, false);
        
        if ((collisionWithElement || collisionWithBlock) && (moveDx != 0 || moveDy != 0)) {
            // If diagonal movement fails, try axis-separated movement (like player system)
            actualMoveDx = 0;
            actualMoveDy = 0;
            canMove = false;
            
            // If we're trying to move diagonally, test each axis separately
            if (moveDx != 0 && moveDy != 0) {                // Try moving only horizontally
                float testX = currentActualX + moveDx;
                float testY = currentActualY; // Keep Y the same
                bool horizontalCollision = wouldEntityCollideWithElementsGranular(config, testX, testY, false) || 
                                           wouldEntityCollideWithBlocksGranular(config, testX, testY, false);
                
                // Try moving only vertically
                float testX2 = currentActualX; // Keep X the same
                float testY2 = currentActualY + moveDy;
                bool verticalCollision = wouldEntityCollideWithElementsGranular(config, testX2, testY2, false) || 
                                        wouldEntityCollideWithBlocksGranular(config, testX2, testY2, false);
                
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
        } else if (!collisionWithElement && !collisionWithBlock) {
            // No collision, can move normally
            canMove = true;
        } else {
            // No movement requested, but still blocked (shouldn't happen normally)
            canMove = false;
        }        // If still can't move after axis separation, try stuck detection and collision resolution
        if (!canMove) {            // STUCK DETECTION AND COLLISION RESOLUTION LOGIC
            const float stuckThreshold = 1.0f; // Reduced time - check sooner if entity is stuck
            const float positionChangeThreshold = 0.02f; // Smaller threshold for more sensitive detection
            
            // Check if entity position has changed significantly
            float positionChangeDistance = std::sqrt(
                (currentActualX - entity.lastPositionX) * (currentActualX - entity.lastPositionX) +
                (currentActualY - entity.lastPositionY) * (currentActualY - entity.lastPositionY)
            );
            
            // Update stuck detection timing
            entity.stuckCheckTime += static_cast<float>(deltaTime);
            
            if (positionChangeDistance > positionChangeThreshold) {
                // Entity has moved significantly, reset stuck detection
                entity.lastPositionX = currentActualX;
                entity.lastPositionY = currentActualY;
                entity.lastPositionChangeTime = entity.stuckCheckTime;
                entity.stuckCount = 0;            } else if (entity.stuckCheckTime - entity.lastPositionChangeTime >= stuckThreshold) {
                // Entity has been stuck for too long, try collision resolution
                entity.stuckCount++;
                
                // Check if entity is close to its destination - if so, don't consider it stuck
                float distanceToTarget = std::sqrt(
                    (currentActualX - entity.targetX) * (currentActualX - entity.targetX) +
                    (currentActualY - entity.targetY) * (currentActualY - entity.targetY)
                );
                
                const float arrivalThreshold = 0.5f; // If within 0.5 units of target, consider it arrived
                if (distanceToTarget <= arrivalThreshold) {
                    std::cout << "Entity " << entity.instanceName << " appears stuck but is close to target (" 
                              << distanceToTarget << " units away) - stopping naturally instead of teleporting" << std::endl;
                    
                    // Stop the entity naturally instead of teleporting
                    entity.isWalking = false;
                    entity.path.clear();
                    entity.currentPathIndex = 0;
                    
                    // Disable animation and reset to standing state
                    elementsManager.changeElementAnimationStatus(elementName, false);
                    elementsManager.changeElementSpriteFrame(elementName, 0);
                    elementsManager.changeElementSpritePhase(elementName, config.defaultSpriteSheetPhase);
                    
                    // Reset stuck detection
                    entity.lastPositionX = currentActualX;
                    entity.lastPositionY = currentActualY;
                    entity.lastPositionChangeTime = entity.stuckCheckTime;
                    entity.stuckCount = 0;
                } else {
                    std::cout << "Entity " << entity.instanceName << " is stuck (count: " << entity.stuckCount 
                              << ") at (" << currentActualX << ", " << currentActualY 
                              << ") - distance to target: " << distanceToTarget << " - attempting collision resolution..." << std::endl;
                    
                    float safeX = currentActualX;
                    float safeY = currentActualY;
                    
                    if (resolveEntityCollisionStuck(entity.instanceName, safeX, safeY, config, gameMap)) {
                        // Check if the safe position is too far from current position
                        float teleportDistance = std::sqrt(
                            (safeX - currentActualX) * (safeX - currentActualX) +
                            (safeY - currentActualY) * (safeY - currentActualY)
                        );
                        
                        const float maxTeleportDistance = 2.0f; // Don't teleport more than 2 units
                        if (teleportDistance <= maxTeleportDistance) {
                            // Safe position is close enough, teleport entity there
                            elementsManager.changeElementCoordinates(elementName, safeX, safeY);
                            
                            std::cout << "Successfully resolved stuck condition for entity " << entity.instanceName 
                                      << " - moved " << teleportDistance << " units to safe position (" << safeX << ", " << safeY << ")" << std::endl;
                            
                            // Reset stuck detection after successful resolution
                            entity.lastPositionX = safeX;
                            entity.lastPositionY = safeY;
                            entity.lastPositionChangeTime = entity.stuckCheckTime;
                            entity.stuckCount = 0;                            
                            // If using pathfinding, recalculate path from new position
                            if (entity.usePathfinding && entity.isWalking && 
                                g_entityAsyncPathfinder && !entity.isWaitingForPath) {
                                
                                std::cout << "Recalculating pathfinding for unstuck entity " << entity.instanceName 
                                          << " from new position (" << safeX << ", " << safeY << ")" << std::endl;
                                
                                // Cancel existing pathfinding request if any
                                if (entity.pathfindingRequestId > 0) {
                                    g_entityAsyncPathfinder->cancelPathfindingRequest(entity.instanceName);
                                }
                                
                                // Request new pathfinding from safe position to original target
                                int newRequestId = g_entityAsyncPathfinder->requestPathfinding(
                                    entity.instanceName, safeX, safeY, 
                                    entity.targetX, entity.targetY, config, entity.walkType
                                );
                                
                                if (newRequestId > 0) {
                                    entity.pathfindingRequestId = newRequestId;
                                    entity.isWaitingForPath = true;
                                    entity.lastPathRequest = std::chrono::steady_clock::now();
                                }
                            }
                        } else {
                            std::cout << "Safe position for entity " << entity.instanceName 
                                      << " is too far away (" << teleportDistance << " units) - stopping entity instead of teleporting" << std::endl;
                            
                            // Stop the entity instead of teleporting too far
                            entity.isWalking = false;
                            entity.path.clear();
                            entity.currentPathIndex = 0;
                            
                            // Disable animation and reset to standing state
                            elementsManager.changeElementAnimationStatus(elementName, false);
                            elementsManager.changeElementSpriteFrame(elementName, 0);
                            elementsManager.changeElementSpritePhase(elementName, config.defaultSpriteSheetPhase);
                            
                            // Reset stuck detection
                            entity.lastPositionX = currentActualX;
                            entity.lastPositionY = currentActualY;
                            entity.lastPositionChangeTime = entity.stuckCheckTime;
                            entity.stuckCount = 0;
                        }
                    } else {
                        std::cout << "Failed to resolve stuck condition for entity " << entity.instanceName 
                                  << " - no safe position found. Stopping entity." << std::endl;
                        
                        // Stop the entity if no safe position can be found
                        entity.isWalking = false;
                        entity.path.clear();
                        entity.currentPathIndex = 0;
                        
                        // Disable animation and reset to standing state
                        elementsManager.changeElementAnimationStatus(elementName, false);
                        elementsManager.changeElementSpriteFrame(elementName, 0);
                        elementsManager.changeElementSpritePhase(elementName, config.defaultSpriteSheetPhase);
                        
                        // Reset stuck detection to avoid immediate re-triggering
                        entity.lastPositionChangeTime = entity.stuckCheckTime;
                        entity.stuckCount = 0;
                    }
                }
                
                // Reset stuck check time to avoid immediate re-triggering
                entity.lastPositionChangeTime = entity.stuckCheckTime;
            }
        }
    }
    
    // Apply the actual movement
    float newX = currentActualX + moveDx;
    float newY = currentActualY + moveDy;
    
    // Update the entity position on the map
    elementsManager.changeElementCoordinates(elementName, newX, newY);
}

// Function to check entity collision with elements (uses collision shape points if available, otherwise fallback to radius)
bool wouldEntityCollideWithElement(const EntityConfiguration& config, float x, float y) {
    if (!config.collisionShapePoints.empty()) {
        // Use polygon-based collision detection
        return wouldEntityCollideWithElement(x, y, config.collisionShapePoints, 1.0f, 0.0f);
    }
    return false; // No collision if no shape points
}

// Enhanced collision function that respects granular collision settings
bool wouldEntityCollideWithElementsGranular(const EntityConfiguration& config, float x, float y, bool useAvoidanceList) {
    if (!config.canCollide) {
        return false; // Entity doesn't have collision enabled
    }
      // Check map boundary collision if offMapCollision is enabled
    if (config.offMapCollision) {
        if (wouldEntityCollideWithMapBounds(config, x, y)) {
            return true; // Entity collision shape would collide with map boundary
        }
    }
      // Determine which element list to check based on collision type
    const std::vector<ElementName>& elementsToCheck = useAvoidanceList ? config.avoidanceElements : config.collisionElements;
    
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
    float searchRadius = 0.0f;
    if (!config.collisionShapePoints.empty()) {
        for (const auto& point : config.collisionShapePoints) {
            float dist = std::sqrt(point.first * point.first + point.second * point.second);
            searchRadius = std::max(searchRadius, dist);
        }
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
        }
    }
    
    return false; // No collision detected with specified element types
}

// Enhanced block collision function that respects granular block collision settings
bool wouldEntityCollideWithBlocksGranular(const EntityConfiguration& config, float x, float y, bool useAvoidanceList) {
    if (!config.canCollide) {
        return false; // Entity doesn't have collision enabled
    }
    
    // Check map boundary collision if offMapCollision is enabled
    if (config.offMapCollision) {
        if (wouldEntityCollideWithMapBounds(config, x, y)) {
            return true; // Entity collision shape would collide with map boundary
        }
    }
    
    // Determine which block list to check based on collision type
    const std::vector<TextureName>& blocksToCheck = useAvoidanceList ? config.avoidanceBlocks : config.collisionBlocks;
    
    // If the list is empty, don't check any blocks (granular collision control)
    // This allows entities to have fine-grained control over what they collide with
    if (blocksToCheck.empty()) {
        return false; // No blocks to check = no collision
    }
    
    // Convert to std::set for faster lookup
    std::set<TextureName> blockSet(blocksToCheck.begin(), blocksToCheck.end());
    
    // If entity has no collision shape, fall back to point-based collision
    if (config.collisionShapePoints.empty()) {
        return wouldCollideWithMapBlock(x, y, gameMap, blockSet);
    }
    
    // Use collision shape for precise block collision detection
    // Transform entity collision shape points to world coordinates
    std::vector<std::pair<float, float>> entityWorldShapePoints;
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
    
    // Find the bounding box of the entity's collision shape
    float minX = entityWorldShapePoints[0].first;
    float maxX = entityWorldShapePoints[0].first;
    float minY = entityWorldShapePoints[0].second;
    float maxY = entityWorldShapePoints[0].second;
    
    for (const auto& point : entityWorldShapePoints) {
        minX = std::min(minX, point.first);
        maxX = std::max(maxX, point.first);
        minY = std::min(minY, point.second);
        maxY = std::max(maxY, point.second);
    }
    
    // Check all grid cells that the entity's bounding box overlaps
    int startGridX = static_cast<int>(std::floor(minX));
    int endGridX = static_cast<int>(std::ceil(maxX));
    int startGridY = static_cast<int>(std::floor(minY));
    int endGridY = static_cast<int>(std::ceil(maxY));
    
    // Clamp to valid grid coordinates
    startGridX = std::max(0, startGridX);
    endGridX = std::min(GRID_SIZE - 1, endGridX);
    startGridY = std::max(0, startGridY);
    endGridY = std::min(GRID_SIZE - 1, endGridY);
    
    // Check each grid cell that might overlap with the entity
    for (int gridY = startGridY; gridY <= endGridY; ++gridY) {
        for (int gridX = startGridX; gridX <= endGridX; ++gridX) {
            // Get the block type at this grid position
            TextureName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
            
            // Check if this block type is in our collision list
            if (blockSet.find(blockType) != blockSet.end()) {
                // Create a polygon for the block (full grid cell)
                std::vector<std::pair<float, float>> blockShapePoints = {
                    {static_cast<float>(gridX), static_cast<float>(gridY)},         // Bottom-left
                    {static_cast<float>(gridX + 1), static_cast<float>(gridY)},     // Bottom-right  
                    {static_cast<float>(gridX + 1), static_cast<float>(gridY + 1)}, // Top-right
                    {static_cast<float>(gridX), static_cast<float>(gridY + 1)}      // Top-left
                };
                
                // Check if entity collision shape overlaps with this block
                if (polygonPolygonCollision(entityWorldShapePoints, blockShapePoints)) {
                    return true; // Collision detected with block
                }
            }
        }
    }
    
    return false; // No collision detected with specified block types
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
    const EntityConfiguration* config = getConfiguration(entity->type);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entityNameToString(entity->type) << std::endl;
        return false;
    }
    
    // Get the element name
    std::string elementName = getElementName(instanceName);    // COLLISION RESOLUTION INTEGRATION
    // Check if entity would be teleported into a collision area and resolve it
    float safeX = x;
    float safeY = y;
    bool needsSafePosition = false;
    
    // Check if the entity would be stuck at the teleport destination
    if (config->canCollide && 
        (wouldEntityCollideWithElementsGranular(*config, x, y, false) || 
         wouldEntityCollideWithBlocksGranular(*config, x, y, false))) {
        
        std::cout << "Entity " << instanceName << " would be teleported into collision area at (" 
                  << x << ", " << y << ") - attempting collision resolution..." << std::endl;
        
        // Try to find a safe position nearby
        if (resolveEntityCollisionStuck(instanceName, safeX, safeY, *config, gameMap)) {
            needsSafePosition = true;
            std::cout << "Found safe teleport position for " << instanceName 
                      << " at (" << safeX << ", " << safeY << ")" << std::endl;
        } else {
            std::cout << "Warning: Could not find safe teleport position for " << instanceName 
                      << " - teleporting to requested coordinates (" << x << ", " << y << ")" << std::endl;
            safeX = x;
            safeY = y;
        }
    }
    
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
    
    std::cout << "Teleported entity " << instanceName << " to (" << safeX << ", " << safeY << ")";
    if (needsSafePosition && (safeX != x || safeY != y)) {
        std::cout << " (collision resolved from requested " << x << ", " << y << ")";
    }
    std::cout << std::endl;
    return true;
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
    
    // DEBUG: Log direction calculation for antagonist entities    // DEBUG: Log direction calculations for antagonist entities
    if (entity.type == EntityName::ANTAGONIST) {
        std::cout << "DEBUG: Antagonist " << entity.instanceName 
                  << " direction calc: dx=" << dx << ", dy=" << dy 
                  << " (abs: " << std::abs(dx) << ", " << std::abs(dy) << ")" << std::endl;
    }
      // Determine the primary direction for sprite updates
    int direction;
    if (std::abs(dx) >= std::abs(dy)) {
        // Horizontal movement dominates or is equal (prefer horizontal for diagonal movement)
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
      // DEBUG: Log sprite phase changes for antagonist entities
    if (entity.type == EntityName::ANTAGONIST) {
        std::cout << "DEBUG: Antagonist " << entity.instanceName 
                  << " direction=" << direction << " -> phase=" << phase 
                  << " (right should be dir=3->phase=" << config.spritePhaseWalkRight << ")" << std::endl;
    }
    
    // Change the sprite phase
    elementsManager.changeElementSpritePhase(elementName, phase);
    
    return true;
}

// Async pathfinding management methods
void EntitiesManager::initializeAsyncPathfinding() {
    if (!g_entityAsyncPathfinder) {
        g_entityAsyncPathfinder = new AsyncEntityPathfinder();
        
        // CRITICAL FIX: Initialize with game map reference before starting
        g_entityAsyncPathfinder->initialize(&gameMap);
        g_entityAsyncPathfinder->start();
        
        std::cout << "Async pathfinding system initialized with game map reference" << std::endl;
    }
}

void EntitiesManager::shutdownAsyncPathfinding() {
    if (g_entityAsyncPathfinder) {
        g_entityAsyncPathfinder->stop();
        delete g_entityAsyncPathfinder;
        g_entityAsyncPathfinder = nullptr;
        std::cout << "Async pathfinding system shutdown" << std::endl;
    }
}

void EntitiesManager::processAsyncPathfindingResults() {
    if (!g_entityAsyncPathfinder) {
        return;
    }
    
    // CRASH FIX: Add comprehensive try-catch around async result processing
    try {
        // Process all completed pathfinding requests
        std::vector<AsyncPathfindingResult> results = g_entityAsyncPathfinder->getCompletedResults();
        
        for (const auto& result : results) {
            // CRASH FIX: Enhanced validation of result data before processing
            if (result.instanceName.empty()) {
                std::cerr << "Warning: Received pathfinding result with empty instance name" << std::endl;
                continue;
            }
            
            // CRASH FIX: Additional safety - check if entity still exists before processing result
            if (!entityExists(result.instanceName)) {
                std::cerr << "Warning: Received pathfinding result for non-existent entity: " << result.instanceName << std::endl;
                continue;
            }
            
            // Find the entity that requested this pathfinding
            Entity* entity = getEntity(result.instanceName);
            if (!entity) {
                std::cerr << "Warning: Received pathfinding result for unknown entity: " << result.instanceName << std::endl;
                continue;
            }
            
            // CRASH FIX: Double-check entity still exists in elements manager
            std::string elementName = getElementName(result.instanceName);
            if (!elementsManager.elementExists(elementName)) {
                std::cerr << "Warning: Entity " << result.instanceName << " element no longer exists during pathfinding result processing" << std::endl;
                // Clean up entity from our map as it's inconsistent
                entities.erase(result.instanceName);
                continue;
            }
            
            // Get the configuration
            const EntityConfiguration* config = getConfiguration(entity->type);
            if (!config) {
                std::cerr << "Warning: Entity configuration not found for: " << entityNameToString(entity->type) << std::endl;
                continue;
            }
            
            if (result.success && result.path.size() >= 2) {
                // Pathfinding succeeded, set up the entity for walking
                
                // If entity is already walking, find the best transition point to avoid stopping
                if (entity->isWalking && entity->usePathfinding && !entity->path.empty()) {
                    // Get current position
                    float currentX, currentY;
                    if (elementsManager.getElementPosition(elementName, currentX, currentY)) {
                        // Find the closest point in the new path to the current position
                        size_t bestTransitionIndex = 0;
                        float minDistance = std::numeric_limits<float>::max();
                        
                        for (size_t i = 0; i < result.path.size(); ++i) {
                            float dx = result.path[i].first - currentX;
                            float dy = result.path[i].second - currentY;
                            float distance = std::sqrt(dx * dx + dy * dy);
                            
                            if (distance < minDistance) {
                                minDistance = distance;
                                bestTransitionIndex = i;
                            }
                        }
                        
                        // Smooth transition: start from the best transition point in the new path
                        entity->path = result.path;
                        entity->currentPathIndex = std::max(1u, static_cast<unsigned int>(bestTransitionIndex));
                        
                        // If we're starting from a point other than the beginning, ensure current position alignment
                        if (bestTransitionIndex > 0 && minDistance > 0.5f) {
                            // Insert current position as a waypoint to ensure smooth transition
                            entity->path.insert(entity->path.begin() + bestTransitionIndex, {currentX, currentY});
                            entity->currentPathIndex = bestTransitionIndex + 1;
                        }
                        
                        if (DEBUG_LOGS) {
                            std::cout << "Entity " << result.instanceName << " smoothly transitioned to new path at index " 
                                      << entity->currentPathIndex << " (distance: " << minDistance << ")" << std::endl;
                        }
                        
                        // SPRITE DIRECTION FIX: Update sprite direction to match new path direction
                        // Calculate direction from current position to the target waypoint
                        if (entity->currentPathIndex < entity->path.size()) {
                            float currentX, currentY;
                            if (elementsManager.getElementPosition(elementName, currentX, currentY)) {
                                const auto& targetWaypoint = entity->path[entity->currentPathIndex];
                                float dx = targetWaypoint.first - currentX;
                                float dy = targetWaypoint.second - currentY;
                                
                                // Update sprite direction for the new path direction
                                handleWaypointArrival(*entity, elementName, *config, currentX, currentY);
                            }
                        }
                    } else {
                        // Fallback: use the new path normally
                        entity->path = result.path;
                        entity->currentPathIndex = 1;
                        
                        // SPRITE DIRECTION FIX: Update sprite direction for fallback case
                        if (entity->path.size() >= 2) {
                            handleWaypointArrival(*entity, elementName, *config, entity->path[0].first, entity->path[0].second);
                        }
                    }
                } else {
                    // Entity is not walking, start normally from the beginning
                    entity->path = result.path;
                    entity->currentPathIndex = 1;
                    entity->isWalking = true;
                    
                    // Enable animation for entities that weren't walking
                    elementsManager.changeElementAnimationStatus(elementName, true);
                    
                    // Set initial sprite for first path segment
                    handleWaypointArrival(*entity, elementName, *config, entity->path[0].first, entity->path[0].second);
                }
                
                // Update entity state
                entity->walkType = result.walkType;
                entity->usePathfinding = true;
                entity->lastSegmentDirection = {0.0f, 0.0f};
                entity->targetX = result.targetX;
                entity->targetY = result.targetY;
                entity->pathfindingRequestId = 0; // Clear the request ID
                entity->isWaitingForPath = false; // No longer waiting
                
                // Ensure animation is enabled and set speed
                elementsManager.changeElementAnimationStatus(elementName, true);
                float animationSpeed = (result.walkType == WalkType::NORMAL) ?
                                      config->normalWalkingAnimationSpeed :
                                      config->sprintWalkingAnimationSpeed;
                elementsManager.changeElementAnimationSpeed(elementName, animationSpeed);
                
                std::cout << "Entity " << result.instanceName << " received async pathfinding result: success, path size: " 
                          << result.path.size() << std::endl;
            } else {
                // Pathfinding failed or path too short
                // If entity was walking before, let it continue its current movement
                // Otherwise, stop the entity
                if (!entity->isWalking) {
                    entity->isWalking = false;
                    elementsManager.changeElementAnimationStatus(elementName, false);
                    elementsManager.changeElementSpriteFrame(elementName, config->defaultSpriteSheetFrame);
                    elementsManager.changeElementSpritePhase(elementName, config->defaultSpriteSheetPhase);
                }
                
                entity->pathfindingRequestId = 0; // Clear the request ID
                entity->isWaitingForPath = false; // No longer waiting
                
                if (!result.success) {
                    std::cout << "Entity " << result.instanceName << " pathfinding failed: " << result.errorMessage << std::endl;
                } else {
                    std::cout << "Entity " << result.instanceName << " pathfinding completed - already at destination" << std::endl;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception in processAsyncPathfindingResults: " << e.what() << std::endl;
        // Continue execution to prevent complete crash
    } catch (...) {
        std::cerr << "CRITICAL: Unknown exception in processAsyncPathfindingResults!" << std::endl;
        // Continue execution to prevent complete crash
    }
}