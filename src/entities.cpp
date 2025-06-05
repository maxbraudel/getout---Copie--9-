#include "entities.h"
#include "entityBehaviors.h"
#include "collision.h"
#include "collisionCache.h"
#include "entitiesStatus.h"
#include "map.h" // Adding for gameMap access
#include "pathfinding.h" // Include for pathfinding cooldown functions
#include "globals.h" // For GRID_SIZE
#include "debug.h" // For isShowingCollisionBoxes function
#include "asyncPathfinding.h"
#include "performanceProfiler.h"
#include "camera.h" // For camera culling optimization
#include <iostream>
#include <cmath>
#include <limits>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <GLFW/glfw3.h>
#include "enumDefinitions.h"

// Forward declaration for global hierarchical grid mutex from collision.cpp
extern std::mutex g_hierarchicalGridMutex;

// Forward declaration for global hierarchical entity grid from collision.cpp
extern HierarchicalEntityGrid g_hierarchicalEntityGrid;
extern std::mutex g_hierarchicalEntityGridMutex;


// Global async pathfinder instance (separate from pathfinding.h's AsyncPathfinder)
static AsyncEntityPathfinder* g_entityAsyncPathfinder = nullptr;

// Static vector of predefined entity types
static std::vector<EntityInfo> entityTypes;
static std::mutex entityTypesInitMutex;

// Spatial grid optimization for entity collision detection (thread-safe with thread_local)
thread_local static std::unordered_map<int, std::vector<std::string>> entitySpatialGrid;
thread_local static bool entitySpatialGridInitialized = false;
thread_local static float lastEntitySpatialGridUpdateTime = 0.0f;
static const int ENTITY_SPATIAL_GRID_SIZE = 20; // Size of each spatial grid cell for entities
static const float ENTITY_SPATIAL_GRID_UPDATE_INTERVAL = 0.1f; // Update every 0.1 seconds

// Function to initialize entity types
static void initializeEntityTypes() {
    std::lock_guard<std::mutex> lock(entityTypesInitMutex);
    if (!entityTypes.empty()) {
        return; // Already initialized
    }
    
    // CRASH FIX: Reserve memory to prevent reallocations during initialization
    entityTypes.reserve(10); // Reasonable initial capacity
    
    try {        // Antagonist entity configuration
        EntityInfo antagonist;
        antagonist.type = EntityName::ANTAGONIST;
        antagonist.elementName = ElementName::ANTAGONIST1;
        antagonist.scale = 1.5f;
        
        // Default sprite configuration
        antagonist.defaultSpriteSheetPhase = 2;
        antagonist.defaultSpriteSheetFrame = 0;
        antagonist.defaultAnimationSpeed = 11.0f;
        
        // Walking animation phases
        antagonist.spritePhaseWalkUp = 3;
        antagonist.spritePhaseWalkDown = 0;
        antagonist.spritePhaseWalkLeft = 2;
        antagonist.spritePhaseWalkRight = 1;
        
        // Movement speeds
        antagonist.normalWalkingSpeed = 1.5f;
        antagonist.normalWalkingAnimationSpeed = 4.0f;
        antagonist.sprintWalkingSpeed = 4.0f;
        antagonist.sprintWalkingAnimationSpeed = 8.0f;    // Collision settings
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
            BlockName::WATER_0,
            BlockName::WATER_1,
            BlockName::WATER_2,
            BlockName::WATER_3,
            BlockName::WATER_4
        };
          antagonist.collisionBlocks = {
            BlockName::WATER_0,
            BlockName::WATER_1,
            BlockName::WATER_2,
            BlockName::WATER_3,
            BlockName::WATER_4        
        };
        
        // Damage blocks configuration - Antagonist takes damage when stepping on these blocks
        antagonist.damageBlocks = {
            BlockName::WATER_0, // Water blocks deal 1000 damage to antagonist
            BlockName::WATER_1,
            BlockName::WATER_2,
            BlockName::WATER_3,
            BlockName::WATER_4
        };
        
        // Entity collision configuration - Antagonist avoids player for pathfinding but can collide during movement
        /* antagonist.avoidanceEntities = {
            EntityName::ANTAGONIST
            // EntityName::ANTAGONIST,
        }; */
        
        /* antagonist.collisionEntities = {
            EntityName::PLAYER,
            EntityName::ANTAGONIST // Antagonist collides with itself
            // Empty - allow overlapping with player during movement/attacks
        }; */
          // Map boundary control settings
        antagonist.offMapAvoidance = true; // Antagonist pathfinding avoids map borders
        antagonist.offMapCollision = true; // Antagonist collides with map borders// Automatic behavior configuration
        antagonist.automaticBehaviors = true; // Enable automatic behaviors for antagonist
        antagonist.passiveState = true; // Enable passive state random walking
        antagonist.passiveStateWalkingRadius = 8.0f; // Walking radius for random walks
        antagonist.passiveStateRandomWalkTriggerTimeIntervalMin = 3.0f; // Min time between walks (seconds)
        antagonist.passiveStateRandomWalkTriggerTimeIntervalMax = 10.0f; // Max time between walks (seconds)
          // Alert state configuration - antagonist becomes alert when player is nearby
        antagonist.alertState = true; // Enable alert state behavior
        antagonist.alertStateStartRadius = 9.0f; // Start becoming alert when player is 3 units away
        antagonist.alertStateEndRadius = 11.0f; // Stop being alert when player is 8+ units away
        antagonist.alertStateTriggerEntitiesList = { EntityName::PLAYER }; // Player triggers alert state
          // Flee state configuration - antagonist flees when player gets too close
        antagonist.fleeState = false; // Enable flee state behavior
        antagonist.fleeStateRunning = true; // Run when fleeing
        antagonist.fleeStateStartRadius = 0.0f; // Start fleeing when player is 2 units away
        antagonist.fleeStateEndRadius = 8.0f; // Stop fleeing when player is 4+ units away
        antagonist.fleeStateMinDistance = 6.0f; // Maintain at least 6 units distance from player
        antagonist.fleeStateMaxDistance = 10.0f; // Flee up to 10 units away
        antagonist.fleeStateTriggerEntitiesList = { EntityName::PLAYER }; // Player triggers flee state
        
        // Attack state configuration - antagonist attacks when player is in range
        antagonist.attackState = true; // Enable attack state behavior
        antagonist.attackStateRunning = true; // Run when attacking
        antagonist.attackStateStartRadius = 0.0f; // Start attacking when player is 5 units away
        antagonist.attackStateEndRadius = 8.0f; // Stop attacking when player is 10+ units away        antagonist.attackStateWaitBeforeChargeMin = 0.5f; // Wait 1-3 seconds before charging again
        antagonist.attackStateWaitBeforeChargeMax = 1.0f;
        antagonist.attackStateTriggerEntitiesList = { EntityName::PLAYER }; // Player triggers attack state
          // Health settings
        antagonist.lifePoints = 20; // Antagonist has 20 life points
        antagonist.damagePoints = 5; // Antagonist deals 5 damage points
        
        // Add to the list
        entityTypes.push_back(antagonist);


        
        
        EntityInfo player;
        player.type = EntityName::PLAYER;
        player.elementName = ElementName::CHARACTER1;
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
        player.normalWalkingSpeed = 2.5f;
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
        };
        player.collisionElements = {
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
            BlockName::WATER_0, // Player avoids water for pathfinding
            BlockName::WATER_1,
            BlockName::WATER_2,
            BlockName::WATER_3,
            BlockName::WATER_4 // Player avoids deep water blocks for pathfinding

        };
          // Player physically collides with deep water but can walk through shallow water
        player.collisionBlocks = {
            BlockName::WATER_0,
            BlockName::WATER_1,
            BlockName::WATER_2,
            BlockName::WATER_3,
            BlockName::WATER_4 // Only deep water blocks movement        
        };
        
        // Damage blocks configuration - Player takes damage when stepping on these blocks
        player.damageBlocks = {
            BlockName::WATER_0, // Water blocks deal 1000 damage to player
            BlockName::WATER_1,
            BlockName::WATER_2,
            BlockName::WATER_3,
            BlockName::WATER_4
        };        
        // Entity collision configuration - Player avoids antagonist for pathfinding and can collide during movement
        /* player.avoidanceEntities = {
            EntityName::ANTAGONIST // Avoid antagonist during pathfinding
        }; */
        
        /* player.collisionEntities = {
            EntityName::ANTAGONIST // Collide with antagonist during movement (prevents overlap)
        }; */
          // Map boundary control settings
        player.offMapAvoidance = true; // Player pathfinding avoids map borders
        player.offMapCollision = true; // Player collides with map borders
        
        // Health settings
        player.lifePoints = 100; // Player has 100 life points

        // Add to the list
        entityTypes.push_back(player);


        // shark entity config
        EntityInfo shark;
        shark.type = EntityName::SHARK;
        shark.elementName = ElementName::SHARK; // Use a test element for now
        shark.scale = 5.5f;
        // Default sprite configuration
        shark.defaultSpriteSheetPhase = 2;
        shark.defaultSpriteSheetFrame = 0;
        shark.defaultAnimationSpeed = 11.0f;
        // Walking animation phases
        shark.spritePhaseWalkUp = 3;
        shark.spritePhaseWalkDown = 0;
        shark.spritePhaseWalkLeft = 2;
        shark.spritePhaseWalkRight = 1;
        // Movement speeds
        shark.normalWalkingSpeed = 1.5f;
        shark.normalWalkingAnimationSpeed = 4.0f;
        shark.sprintWalkingSpeed = 4.0f;
        shark.sprintWalkingAnimationSpeed = 8.0f;    // Collision settings
        shark.canCollide = true;
        shark.collisionShapePoints = {
            {-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f}
        };    // Granular collision control - shark avoids trees but can move through player
        // For testing: Leave lists empty to allow movement through all elements
        shark.collisionBlocks = {
            BlockName::SAND,
            BlockName::GRASS_0,
            BlockName::GRASS_1,
            BlockName::GRASS_2,
            BlockName::GRASS_3,
            BlockName::WATER_0,
            BlockName::WATER_1,
            BlockName::WATER_2,
        };        shark.avoidanceBlocks = {
            BlockName::SAND,
            BlockName::GRASS_0,
            BlockName::GRASS_1,
            BlockName::GRASS_2,
            BlockName::GRASS_3,
            BlockName::WATER_0,
            BlockName::WATER_1,
            BlockName::WATER_2,
        };

        // Damage blocks configuration - Shark doesn't take damage from any blocks (lives in water)
        shark.damageBlocks = {
            BlockName::GRASS_0,
            BlockName::GRASS_1,
            BlockName::GRASS_2,
            BlockName::GRASS_3,
            BlockName::SAND,
            BlockName::ICE_1,
            BlockName::ICE_2,
            BlockName::ICE_3
        };



        shark.offMapAvoidance = true; // Antagonist pathfinding avoids map borders
        shark.offMapCollision = true; // Antagonist collides with map borders// Automatic behavior configuration
        shark.automaticBehaviors = true; // Enable automatic behaviors for antagonist2
        shark.passiveState = true; // Enable passive state random walking
        shark.passiveStateWalkingRadius = 8.0f; // Walking radius for random walks
        shark.passiveStateRandomWalkTriggerTimeIntervalMin = 3.0f; // Min time between walks (seconds)
        shark.passiveStateRandomWalkTriggerTimeIntervalMax = 10.0f; 

        entityTypes.push_back(shark); // Add shark to the list

            // Empty - no blocks to avoid for pathfinding


          // Block collision configuration - Shark avoids water during pathfinding and movement


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
    // Clear all pathfinding cooldowns for entities
    for (const auto& entityPair : entities) {
        clearEntityPathfindingCooldown(entityPair.first);
    }
    
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

const EntityConfiguration* EntitiesManager::getConfiguration(EntityName entityType) const {
    auto it = configurations.find(entityType);
    if (it != configurations.end()) {
        return &(it->second);
    }
    return nullptr;
}

// String-based getConfiguration method for backwards compatibility
const EntityConfiguration* EntitiesManager::getConfiguration(const std::string& typeName) const {
    // Convert string to enum and call the enum-based method
    if (typeName == "player") {
        return getConfiguration(EntityName::PLAYER);
    } else if (typeName == "antagonist") {
        return getConfiguration(EntityName::ANTAGONIST);
    }
    
    std::cerr << "Unknown entity type string: " << typeName << std::endl;
    return nullptr;
}

void EntitiesManager::addConfiguration(const EntityConfiguration& config) {
    // Add or replace the configuration
    configurations[config.type] = config;
    std::cout << "Added entity configuration: " << entityNameToString(config.type) << std::endl;
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

// String-based placeEntityByType method for backwards compatibility
bool EntitiesManager::placeEntityByType(const std::string& instanceName, const std::string& typeName, float x, float y) {
    // Convert string to enum and call the enum-based method
    if (typeName == "player") {
        return placeEntityByType(instanceName, EntityName::PLAYER, x, y);
    } else if (typeName == "antagonist") {
        return placeEntityByType(instanceName, EntityName::ANTAGONIST, x, y);
    }
    
    std::cerr << "Unknown entity type string: " << typeName << std::endl;
    return false;
}

// Overloaded placeEntityByType method with sprite phase override (enum-based)
bool EntitiesManager::placeEntityByType(const std::string& instanceName, EntityName entityType, float x, float y, int overrideSpritePhase) {
    // Find the entity type in our predefined list
    for (const auto& entityInfo : entityTypes) {
        if (entityInfo.type == entityType) {
            // Create configuration from the entity info
            EntityConfiguration config(entityInfo);
            // Add the configuration if it doesn't exist yet
            if (!getConfiguration(entityType)) {
                addConfiguration(config);
            }
            // Place the entity with sprite phase override
            return placeEntity(instanceName, entityType, x, y, overrideSpritePhase);
        }
    }
    
    std::cerr << "Entity type not found: " << entityNameToString(entityType) << std::endl;
    return false;
}

// String-based placeEntityByType method with sprite phase override for backwards compatibility
bool EntitiesManager::placeEntityByType(const std::string& instanceName, const std::string& typeName, float x, float y, int overrideSpritePhase) {
    // Convert string to enum and call the enum-based method
    if (typeName == "player") {
        return placeEntityByType(instanceName, EntityName::PLAYER, x, y, overrideSpritePhase);
    } else if (typeName == "antagonist") {
        return placeEntityByType(instanceName, EntityName::ANTAGONIST, x, y, overrideSpritePhase);
    }
    
    std::cerr << "Unknown entity type string: " << typeName << std::endl;
    return false;
}

// Place entity by type safely - finds safe position first using findNearestSafePlaceFromCoordinatesForEntity
bool EntitiesManager::placeEntityByTypeSafely(const std::string& instanceName, EntityName entityType, float x, float y) {
    // First, create a temporary entity instance to get its configuration for safe position finding
    // We need the configuration to check collision properties
    const EntityConfiguration* tempConfig = nullptr;
    
    // Find the entity type in our predefined list to get its configuration
    for (const auto& entityInfo : entityTypes) {
        if (entityInfo.type == entityType) {
            // Create configuration from the entity info
            EntityConfiguration config(entityInfo);
            // Add the configuration if it doesn't exist yet
            if (!getConfiguration(entityType)) {
                addConfiguration(config);
            }
            tempConfig = getConfiguration(entityType);
            break;
        }
    }
    
    if (!tempConfig) {
        std::cerr << "Entity type not found for safe placement: " << entityNameToString(entityType) << std::endl;
        return false;
    }
    
    // Create a temporary entity to use findNearestSafePlaceFromCoordinatesForEntity
    // We need an existing entity instance for the function to work
    std::string tempElementName = getElementName(instanceName);
    
    // Place a temporary element at the requested coordinates to create the entity context
    elementsManager.placeElement(
        tempElementName,
        tempConfig->elementName,
        tempConfig->scale,
        x, y,  // Place at requested position temporarily
        0.0f,  // rotation
        tempConfig->defaultSpriteSheetPhase,
        tempConfig->defaultSpriteSheetFrame,
        false,  // not animated initially
        tempConfig->defaultAnimationSpeed,
        AnchorPoint::USE_TEXTURE_DEFAULT
    );
    
    // Create temporary entity entry
    Entity tempEntity;
    tempEntity.instanceName = instanceName;
    tempEntity.type = entityType;
    entities[instanceName] = tempEntity;
    
    // Now find the safe position using the existing function
    float safeX, safeY;
    bool foundSafePosition = findNearestSafePlaceFromCoordinatesForEntity(instanceName, x, y, safeX, safeY);
    
    // Clean up the temporary entity and element
    entities.erase(instanceName);
    elementsManager.removeElement(tempElementName);
    
    if (!foundSafePosition) {
        std::cout << "Warning: Could not find safe position for entity " << instanceName 
                  << " - placing at requested coordinates (" << x << ", " << y << ")" << std::endl;
        safeX = x;
        safeY = y;
    } else if (safeX != x || safeY != y) {
        std::cout << "Found safe position for entity " << instanceName 
                  << " - adjusted from (" << x << ", " << y << ") to (" << safeX << ", " << safeY << ")" << std::endl;
    }
    
    // Now place the entity at the safe coordinates using the normal method
    return placeEntityByType(instanceName, entityType, safeX, safeY);
}

// String-based placeEntityByTypeSafely method for backwards compatibility
bool EntitiesManager::placeEntityByTypeSafely(const std::string& instanceName, const std::string& typeName, float x, float y) {
    // Convert string to enum and call the enum-based method
    if (typeName == "player") {
        return placeEntityByTypeSafely(instanceName, EntityName::PLAYER, x, y);
    } else if (typeName == "antagonist") {
        return placeEntityByTypeSafely(instanceName, EntityName::ANTAGONIST, x, y);
    }
    
    std::cerr << "Unknown entity type string for safe placement: " << typeName << std::endl;
    return false;
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
    }    // Generate the element name
    std::string elementName = getElementName(instanceName);
    
    // Create an Entity object
    Entity entity;
    entity.instanceName = instanceName;
    entity.type = entityType;
    entity.lifePoints = config->lifePoints;  // Initialize entity's life points from config
    entity.damagePoints = config->damagePoints;  // Initialize entity's damage points from config
    
    // Place the element on the map using ElementsOnMap
    elementsManager.placeElement(
        elementName,
        config->elementName,
        config->scale,
        safeX, safeY,  // Use the requested position (no automatic adjustment)
        0.0f,  // rotation
        config->defaultSpriteSheetPhase,
        config->defaultSpriteSheetFrame,
        false,  // not animated initially
        config->defaultAnimationSpeed,
        AnchorPoint::USE_TEXTURE_DEFAULT
    );    // Add the entity to our entities map
    entities[instanceName] = entity;
    
    // Reset spatial grid since we added a new entity
    resetEntitySpatialGrid();
      // Initialize entity behaviors
    Entity& createdEntity = entities[instanceName];
    entityBehaviorManager.initializeEntityBehavior(createdEntity, *config);
    
    // Register entity with hierarchical entity grid for optimized collision detection
    {
        std::lock_guard<std::mutex> lock(g_hierarchicalEntityGridMutex);
        // Initialize hierarchical entity grid if needed
        if (!g_hierarchicalEntityGrid.isInitializedState()) {
            g_hierarchicalEntityGrid.initialize();
        }
        // Register the newly placed entity
        g_hierarchicalEntityGrid.addEntity(instanceName, safeX, safeY);
    }
    
      if (needsSafePosition && (safeX != x || safeY != y)) {
        std::cout << "Entity " << instanceName << " placed with collision resolution - moved from (" 
                  << x << ", " << y << ") to (" << safeX << ", " << safeY << ")" << std::endl;    } else {
        std::cout << "Placed entity: " << instanceName << " (type: " << entityNameToString(entityType) << ") at (" 
                << safeX << ", " << safeY << ")" << std::endl;
    }
    return true;
}

// Overloaded placeEntity method with sprite phase override
bool EntitiesManager::placeEntity(const std::string& instanceName, EntityName entityType, float x, float y, int overrideSpritePhase) {
    // Check if the configuration exists
    const EntityConfiguration* config = getConfiguration(entityType);
    if (!config) {
        std::cerr << "Entity configuration not found: " << entityNameToString(entityType) << std::endl;
        return false;
    }

    // COLLISION RESOLUTION INTEGRATION
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
    entity.lifePoints = config->lifePoints;  // Initialize entity's life points from config
    entity.damagePoints = config->damagePoints;  // Initialize entity's damage points from config
    
    // Place the element on the map using ElementsOnMap with sprite phase override
    elementsManager.placeElement(
        elementName,
        config->elementName,
        config->scale,
        safeX, safeY,  // Use the requested position (no automatic adjustment)
        0.0f,  // rotation
        overrideSpritePhase,  // Use the provided override instead of config->defaultSpriteSheetPhase
        config->defaultSpriteSheetFrame,
        false,  // not animated initially
        config->defaultAnimationSpeed,
        AnchorPoint::USE_TEXTURE_DEFAULT
    );

    // Add the entity to our entities map
    entities[instanceName] = entity;
    
    // Reset spatial grid since we added a new entity
    resetEntitySpatialGrid();
    
    // Initialize entity behaviors
    Entity& createdEntity = entities[instanceName];
    entityBehaviorManager.initializeEntityBehavior(createdEntity, *config);
    
    // Register entity with hierarchical entity grid for optimized collision detection
    {
        std::lock_guard<std::mutex> lock(g_hierarchicalEntityGridMutex);
        // Initialize hierarchical entity grid if needed
        if (!g_hierarchicalEntityGrid.isInitializedState()) {
            g_hierarchicalEntityGrid.initialize();
        }
        // Register the newly placed entity
        g_hierarchicalEntityGrid.addEntity(instanceName, safeX, safeY);
    }
    
    if (needsSafePosition && (safeX != x || safeY != y)) {
        std::cout << "Entity " << instanceName << " placed with collision resolution - moved from (" 
                  << x << ", " << y << ") to (" << safeX << ", " << safeY << ") with sprite phase override: " << overrideSpritePhase << std::endl;
    } else {
        std::cout << "Placed entity: " << instanceName << " (type: " << entityNameToString(entityType) << ") at (" 
                << safeX << ", " << safeY << ") with sprite phase override: " << overrideSpritePhase << std::endl;
    }
    return true;
}

// String-based placeEntity method for backwards compatibility
bool EntitiesManager::placeEntity(const std::string& instanceName, const std::string& typeName, float x, float y) {
    // Convert string to enum and call the enum-based method
    if (typeName == "player") {
        return placeEntity(instanceName, EntityName::PLAYER, x, y);
    } else if (typeName == "antagonist") {
        return placeEntity(instanceName, EntityName::ANTAGONIST, x, y);
    }
    
    std::cerr << "Unknown entity type string: " << typeName << std::endl;
    return false;
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
    }    // Check async pathfinding availability
    if (!g_entityAsyncPathfinder) {
        std::cerr << "Async pathfinder not initialized" << std::endl;
        return false;
    }
    
    // Check pathfinding cooldown to prevent too frequent requests
    if (!canEntityRequestPathfinding(instanceName)) {
        if (DEBUG_LOGS) {
            std::cout << "Pathfinding request for entity " << instanceName << " denied due to cooldown" << std::endl;
        }
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
        
        // Update pathfinding cooldown time for this entity
        updateEntityPathfindingTime(instanceName);
        
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
    const float searchStep = 1.0f;
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
    }    // Set up random number generation (thread-safe)
    thread_local static std::random_device rd;
    thread_local static std::mt19937 gen(rd());
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
                stopEntityMovement(entity.instanceName); // Stop walking due to error
                continue;
            }
            
            // Update the entity's walking animation with additional safety
            try {
                updateEntityWalking(entity, *config, deltaTime);            } catch (const std::exception& e) {
                std::cerr << "CRITICAL: Exception updating entity " << instanceName << ": " << e.what() << std::endl;
                // Stop entity to prevent further crashes
                stopEntityMovement(instanceName);
            } catch (...) {
                std::cerr << "CRITICAL: Unknown exception updating entity " << instanceName << std::endl;
                // Stop entity to prevent further crashes
                stopEntityMovement(instanceName);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception in EntitiesManager::update: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "CRITICAL: Unknown exception in EntitiesManager::update!" << std::endl;
    }
}

void EntitiesManager::update(double deltaTime, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop) {
    PROFILE_SCOPE("EntitiesManager_Update_Total");
    
    // CRASH FIX: Add comprehensive protection for entity updates with view frustum culling
    try {
        // Process async pathfinding results first
        {
            PROFILE_SCOPE("Entities_AsyncPathfinding");
            processAsyncPathfindingResults();
        }
        
        // CRASH FIX: Collect entity names first to avoid iterator invalidation
        std::vector<std::string> walkingEntityNames;
        {
            PROFILE_SCOPE("Entities_CollectWalkingNames");
            walkingEntityNames.reserve(entities.size());
            
            for (const auto& pair : entities) {
                if (pair.second.isWalking) {
                    walkingEntityNames.push_back(pair.first);
                }
            }
        }
        
        // Update all walking entities using safe name-based iteration with view frustum culling
        {
            PROFILE_SCOPE("Entities_UpdateWalking");
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
                
                // View frustum culling: Check if entity is within camera bounds
                std::string elementName = getElementName(entity.instanceName);
                float entityX, entityY;
                if (elementsManager.getElementPosition(elementName, entityX, entityY)) {
                    // Get entity configuration to access scale for culling bounds
                    const EntityConfiguration* config = getConfiguration(entity.type);
                    if (config) {
                        float entityScale = config->scale;
                        
                        // Check if entity is outside camera view (with buffer for scale)
                        if (entityX < cameraLeft - entityScale || entityX > cameraRight + entityScale || 
                            entityY < cameraBottom - entityScale || entityY > cameraTop + entityScale) {
                            // Entity is outside camera view, skip update to improve performance
                            continue;
                        }
                    }
                }
                
                // Get the configuration for this entity
                const EntityConfiguration* config = getConfiguration(entity.type);
                if (!config) {
                    std::cerr << "Error: Cannot find configuration for entity: " << entity.instanceName << std::endl;
                    stopEntityMovement(entity.instanceName); // Stop walking due to error
                    continue;
                }
                
                // Update the entity's walking animation with additional safety
                try {
                    updateEntityWalking(entity, *config, deltaTime);
                } catch (const std::exception& e) {
                    std::cerr << "CRITICAL: Exception updating entity " << instanceName << ": " << e.what() << std::endl;
                    // Stop entity to prevent further crashes
                    stopEntityMovement(instanceName);
                } catch (...) {
                    std::cerr << "CRITICAL: Unknown exception updating entity " << instanceName << std::endl;
                    // Stop entity to prevent further crashes
                    stopEntityMovement(instanceName);
                }
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

void EntitiesManager::updateEntityWalking(Entity& entity, const EntityConfiguration& config, double deltaTime) {    // CRASH FIX: Validate entity state before processing
    if (entity.instanceName.empty()) {
        std::cerr << "ERROR: Entity has empty instance name in updateEntityWalking" << std::endl;
        stopEntityMovement(entity.instanceName);
        return;
    }
    
    // Get the element name
    std::string elementName = getElementName(entity.instanceName);
      // CRASH FIX: Verify element exists before processing
    if (!elementsManager.elementExists(elementName)) {
        std::cerr << "ERROR: Element " << elementName << " for entity " << entity.instanceName << " no longer exists" << std::endl;
        stopEntityMovement(entity.instanceName);
        return;
    }
      // Get current position
    float currentActualX, currentActualY;    if (!elementsManager.getElementPosition(elementName, currentActualX, currentActualY)) {
        std::cerr << "Error getting position for entity: " << entity.instanceName << std::endl;
        stopEntityMovement(entity.instanceName); // Stop walking due to error
        return;
    }

    // Stuck detection logic - check if entity has been in same position for too long
    const float MOVEMENT_THRESHOLD = 0.1f; // Minimum movement to consider "not stuck"
    
    // Calculate distance moved since last position check
    float distanceMoved = std::sqrt(std::pow(currentActualX - entity.lastPositionX, 2) + 
                                   std::pow(currentActualY - entity.lastPositionY, 2));
    
    // If entity hasn't moved significantly, accumulate stuck time
    if (distanceMoved < MOVEMENT_THRESHOLD) {
        entity.stuckCheckTime += static_cast<float>(deltaTime);
        
        // If stuck for too long, stop movement
        if (entity.stuckCheckTime >= ENTITY_STUCK_TIMEOUT_FOR_STOPPING_MOVEMENT) {
            std::cout << "Entity " << entity.instanceName << " appears to be stuck at (" 
                      << currentActualX << ", " << currentActualY << ") for " 
                      << entity.stuckCheckTime << " seconds. Stopping movement." << std::endl;
            stopEntityMovement(entity.instanceName);
            return;
        }
    } else {
        // Entity moved, reset stuck timer and update last position
        entity.stuckCheckTime = 0.0f;
        entity.lastPositionX = currentActualX;
        entity.lastPositionY = currentActualY;
        entity.lastPositionChangeTime = static_cast<float>(deltaTime);
    }

    auto updateDirectionAndSprite = [&](Entity& ent, const std::string& elName, const EntityConfiguration& cfg, float dirX, float dirY) {
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
        (entity.usePathfinding && entity.currentPathIndex >= entity.path.size()))) {        // We've reached the final target
        // Always use current position as final position to prevent teleportation
        float finalX = currentActualX;
        float finalY = currentActualY;
        
        std::cout << "Entity " << entity.instanceName << " stopping naturally at (" 
                  << finalX << ", " << finalY << ") - target was (" 
                  << entity.targetX << ", " << entity.targetY << "), distance: " << distance << std::endl;
        
        // Move to the final position (usually current position, preventing teleportation)
        elementsManager.changeElementCoordinates(elementName, finalX, finalY);
        
        // Stop entity movement using centralized function
        stopEntityMovement(entity.instanceName);
        
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
        float newY = currentActualY + moveDy;        // Check if the combined movement would collide with collision elements (hard collision only)
        // Note: We only check collision elements here, not avoidance elements
        // This allows entities to move through avoidance elements during direct movement
        bool collisionWithElement = wouldEntityCollideWithElementsGranular(config, newX, newY, false);
        
        // Also check for collision with blocks using granular block collision control
        bool collisionWithBlock = wouldEntityCollideWithBlocksGranular(config, newX, newY, false);
        
        // Check for collision with other entities using granular entity collision control
        // For movement, we check collision entities (not avoidance entities)
        // This prevents entities from overlapping during movement
        bool collisionWithEntity = wouldEntityCollideWithEntitiesGranular(config, newX, newY, false, entity.instanceName);
        
        if ((collisionWithElement || collisionWithBlock || collisionWithEntity) && (moveDx != 0 || moveDy != 0)) {
            // If diagonal movement fails, try axis-separated movement (like player system)
            actualMoveDx = 0;
            actualMoveDy = 0;
            canMove = false;
            
            // If we're trying to move diagonally, test each axis separately
            if (moveDx != 0 && moveDy != 0) {                // Try moving only horizontally
                float testX = currentActualX + moveDx;
                float testY = currentActualY; // Keep Y the same
                bool horizontalCollision = wouldEntityCollideWithElementsGranular(config, testX, testY, false) || 
                                           wouldEntityCollideWithBlocksGranular(config, testX, testY, false) ||
                                           wouldEntityCollideWithEntitiesGranular(config, testX, testY, false, entity.instanceName);
                
                // Try moving only vertically
                float testX2 = currentActualX; // Keep X the same
                float testY2 = currentActualY + moveDy;
                bool verticalCollision = wouldEntityCollideWithElementsGranular(config, testX2, testY2, false) || 
                                        wouldEntityCollideWithBlocksGranular(config, testX2, testY2, false) ||
                                        wouldEntityCollideWithEntitiesGranular(config, testX2, testY2, false, entity.instanceName);
                
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
        } else if (!collisionWithElement && !collisionWithBlock && !collisionWithEntity) {
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
            
            // Update pathfinding timeout timer when entity uses pathfinding
            if (entity.usePathfinding) {
                entity.pathfindingTimeoutTimer += static_cast<float>(deltaTime);
            }
            
            if (positionChangeDistance > positionChangeThreshold) {
                // Entity has moved significantly, reset stuck detection
                entity.lastPositionX = currentActualX;
                entity.lastPositionY = currentActualY;
                entity.lastPositionChangeTime = entity.stuckCheckTime;
                entity.stuckCount = 0;
                  // Also reset pathfinding timeout
                entity.pathfindingTimeoutTimer = 0.0f;
                entity.pathfindingTimeoutActive = false;
            } 
            // Check for pathfinding timeout condition first (shorter threshold)
            else if (entity.usePathfinding && entity.pathfindingTimeoutTimer >= ENTITY_STUCK_TIMEOUT_FOR_STOPPING_MOVEMENT) {
                // Entity has been stuck for the pathfinding timeout duration, stop it
                std::cout << "Entity " << entity.instanceName << " has not moved for " 
                          << ENTITY_STUCK_TIMEOUT_FOR_STOPPING_MOVEMENT << " seconds during pathfinding - stopping movement" << std::endl;
                
                // Stop the entity movement
                stopEntityMovement(entity.instanceName);
                
                // Reset both pathfinding timeout and stuck detection
                entity.pathfindingTimeoutTimer = 0.0f;
                entity.pathfindingTimeoutActive = false;
                entity.lastPositionChangeTime = entity.stuckCheckTime;
                entity.stuckCount = 0;
            } 
            else if (entity.stuckCheckTime - entity.lastPositionChangeTime >= stuckThreshold) {
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
                    
                    // Stop the entity naturally using centralized function
                    stopEntityMovement(entity.instanceName);
                    
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
                                
                                // Check pathfinding cooldown before requesting
                                if (canEntityRequestPathfinding(entity.instanceName)) {
                                    // Request new pathfinding from safe position to original target
                                    int newRequestId = g_entityAsyncPathfinder->requestPathfinding(
                                        entity.instanceName, safeX, safeY, entity.targetX, entity.targetY, config, entity.walkType
                                    );
                                      if (newRequestId > 0) {
                                        entity.pathfindingRequestId = newRequestId;
                                        entity.isWaitingForPath = true;
                                        entity.lastPathRequest = std::chrono::steady_clock::now();
                                        
                                        // Update pathfinding cooldown time for this entity
                                        updateEntityPathfindingTime(entity.instanceName);
                                    }
                                } else {
                                    std::cout << "Pathfinding request for unstuck entity " << entity.instanceName 
                                              << " denied due to cooldown" << std::endl;
                                }
                            }
                        } else {
                            std::cout << "Safe position for entity " << entity.instanceName 
                                      << " is too far away (" << teleportDistance << " units) - stopping entity instead of teleporting" << std::endl;
                            
                            // Stop the entity instead of teleporting too far using centralized function
                            stopEntityMovement(entity.instanceName);
                            
                            // Reset stuck detection
                            entity.lastPositionX = currentActualX;
                            entity.lastPositionY = currentActualY;
                            entity.lastPositionChangeTime = entity.stuckCheckTime;
                            entity.stuckCount = 0;
                        }                    } else {
                        std::cout << "Failed to resolve stuck condition for entity " << entity.instanceName 
                                  << " - no safe position found. Stopping entity." << std::endl;
                        
                        // Stop the entity if no safe position can be found using centralized function
                        stopEntityMovement(entity.instanceName);
                        
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
    
    // Check for damage blocks after entity movement
    checkAndApplyDamageBlocksToEntity(entity.instanceName, *this);
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
      // Calculate approximate radius for nearby element search
    float searchRadius = 0.0f;
    if (!config.collisionShapePoints.empty()) {
        for (const auto& point : config.collisionShapePoints) {
            float dist = std::sqrt(point.first * point.first + point.second * point.second);
            searchRadius = std::max(searchRadius, dist);
        }
    }// Use hierarchical spatial grid for better performance with large numbers of elements
    // Thread-safe access to hierarchical grid
    std::vector<std::string> nearbyElements;
    {
        std::lock_guard<std::mutex> lock(g_hierarchicalGridMutex);
        // Initialize hierarchical grid if needed
        if (!g_hierarchicalGrid.isInitializedState()) {
            g_hierarchicalGrid.initialize();
        }
        
        // Update grid before collision check
        g_hierarchicalGrid.updateGrid();
        
        // Get nearby elements using optimized hierarchical lookup
        nearbyElements = g_hierarchicalGrid.getElementsHierarchical(x, y, searchRadius + MAX_COLLISION_CHECK_RANGE);
    }
    // PERFORMANCE OPTIMIZATION: Thread-safe cache with proper synchronization
    // Use thread_local to avoid contention between threads
    thread_local static std::unordered_map<std::string, PlacedElement> elementDataCache;
    thread_local static float lastCacheUpdateTime = 0.0f;
    thread_local static std::unordered_set<ElementName> elementsToCheckSet;
    thread_local static bool elementSetCached = false;
    thread_local static bool collisionCacheInitialized = false;
    
    // Refresh cache periodically or when elements list changes
    float currentTime = static_cast<float>(glfwGetTime());
    if (!collisionCacheInitialized || currentTime - lastCacheUpdateTime > 1.0f) {
        elementDataCache.clear();
        elementsToCheckSet.clear();
        
        // Get fresh copy of elements (this is already thread-safe in ElementsManager)
        const auto& elements = elementsManager.getElements();
        elementDataCache.reserve(elements.size()); // Pre-allocate to prevent reallocations
        
        for (const auto& el : elements) {
            elementDataCache[el.instanceName] = el; // Store copy of element data
        }
        
        lastCacheUpdateTime = currentTime;
        collisionCacheInitialized = true;
        elementSetCached = false;
    }
    
    // Cache the set conversion for faster lookups (thread-local, no race condition)
    if (!elementSetCached) {
        elementsToCheckSet.clear();
        elementsToCheckSet.reserve(elementsToCheck.size()); // Pre-allocate
        elementsToCheckSet.insert(elementsToCheck.begin(), elementsToCheck.end());
        elementSetCached = true;
    }    // CAMERA CULLING OPTIMIZATION: Get current camera bounds to avoid processing elements outside view
    float cameraLeft = gameCamera.getLeft();
    float cameraRight = gameCamera.getRight();
    float cameraBottom = gameCamera.getBottom();
    float cameraTop = gameCamera.getTop();
    
    // Check collision only with elements that match the specified texture types
    for (const auto& elementName : nearbyElements) {
        // PERFORMANCE FIX: Use cached lookup instead of linear search
        auto cacheIt = elementDataCache.find(elementName);
        if (cacheIt == elementDataCache.end()) {
            continue; // Element not found in cache
        }
        
        const PlacedElement& currentElement = cacheIt->second; // Get reference to cached data
        if (!currentElement.hasCollision) {
            continue; // Skip elements without collision enabled
        }
        
        // PERFORMANCE FIX: Use set lookup instead of linear search
        if (elementsToCheckSet.find(currentElement.elementName) == elementsToCheckSet.end()) {
            continue; // Skip elements not in our collision/avoidance list
        }
          // CAMERA CULLING: Skip elements that are outside the camera view with generous buffer
        // Only apply culling when entity is also outside camera view to prevent edge cases
        const float cameraBuffer = 10.0f; // Buffer to prevent culling elements near camera edges
        if (currentElement.x < cameraLeft - currentElement.scale - cameraBuffer || 
            currentElement.x > cameraRight + currentElement.scale + cameraBuffer || 
            currentElement.y < cameraBottom - currentElement.scale - cameraBuffer || 
            currentElement.y > cameraTop + currentElement.scale + cameraBuffer) {
            
            // Also check if the entity itself is outside camera view before culling
            if (x < cameraLeft - searchRadius - cameraBuffer || 
                x > cameraRight + searchRadius + cameraBuffer || 
                y < cameraBottom - searchRadius - cameraBuffer || 
                y > cameraTop + searchRadius + cameraBuffer) {
                continue; // Skip element collision check when both element and entity are outside camera view
            }
        }

        // Perform the actual collision check for this specific element
        if (!config.collisionShapePoints.empty() && !currentElement.collisionShapePoints.empty()) {
            // MEMORY SAFETY: Validate collision shape data integrity
            if (config.collisionShapePoints.size() > 1000 || currentElement.collisionShapePoints.size() > 1000) {
                continue; // Skip elements with suspiciously large collision shapes
            }
            
            // Validate scale and rotation values to prevent numerical issues
            if (std::isnan(currentElement.scale) || std::isinf(currentElement.scale) || 
                currentElement.scale <= 0.0f || currentElement.scale > 100.0f) {
                continue; // Skip elements with invalid scale
            }
            
            if (std::isnan(currentElement.rotation) || std::isinf(currentElement.rotation)) {
                continue; // Skip elements with invalid rotation
            }
            
            // PERFORMANCE OPTIMIZATION: Use pre-calculated collision boxes for massive speedup
            // This replaces the expensive real-time transformation calculations
            
            // Get or calculate cached collision box for entity
            PreCalculatedCollisionBox entityCollisionBox;
            const auto& entityCachedBox = CollisionBoxUtils::getOrUpdateCollisionBox(
                entityCollisionBox, config.collisionShapePoints, x, y, 0.0f, 1.0f);
            
            // Get or calculate cached collision box for element 
            const auto& elementCachedBox = CollisionBoxUtils::getOrUpdateCollisionBox(
                currentElement.cachedCollisionBox, currentElement.collisionShapePoints, 
                currentElement.x, currentElement.y, currentElement.rotation, currentElement.scale);
            
            // Fast bounding box intersection test first
            if (!entityCachedBox.boundingBoxIntersects(elementCachedBox)) {
                continue; // No intersection possible, skip expensive polygon test
            }
              // Perform polygon-polygon collision detection using cached world points
            // MEMORY SAFETY: Check if both polygons have valid points before collision detection
            if (!entityCachedBox.worldPoints.empty() && !elementCachedBox.worldPoints.empty() &&
                entityCachedBox.worldPoints.size() >= 3 && elementCachedBox.worldPoints.size() >= 3) {
                
                // Additional safety check: ensure cached vectors are not corrupted
                bool entityShapeValid = true;
                bool elementShapeValid = true;
                
                for (const auto& point : entityCachedBox.worldPoints) {
                    if (std::isnan(point.first) || std::isnan(point.second) ||
                        std::isinf(point.first) || std::isinf(point.second)) {
                        entityShapeValid = false;
                        break;
                    }
                }
                
                if (entityShapeValid) {
                    for (const auto& point : elementCachedBox.worldPoints) {
                        if (std::isnan(point.first) || std::isnan(point.second) ||
                            std::isinf(point.first) || std::isinf(point.second)) {
                            elementShapeValid = false;
                            break;
                        }
                    }
                }
                
                // Only perform collision detection if both shapes are valid
                if (entityShapeValid && elementShapeValid) {
                    if (polygonPolygonCollision(entityCachedBox.worldPoints, elementCachedBox.worldPoints)) {
                        return true; // Collision detected
                    }
                }
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
    const std::vector<BlockName>& blocksToCheck = useAvoidanceList ? config.avoidanceBlocks : config.collisionBlocks;
    
    // If the list is empty, don't check any blocks (granular collision control)
    // This allows entities to have fine-grained control over what they collide with
    if (blocksToCheck.empty()) {
        return false; // No blocks to check = no collision
    }
    
    // Convert to std::set for faster lookup
    std::set<BlockName> blockSet(blocksToCheck.begin(), blocksToCheck.end());
    
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
      // CRASH FIX: Check if transformation actually produced any points
    if (entityWorldShapePoints.empty()) {
        std::cerr << "CRITICAL: entityWorldShapePoints is empty after transformation - using fallback collision detection" << std::endl;
        return wouldCollideWithMapBlock(x, y, gameMap, blockSet);
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
            BlockName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
            
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
    stopEntityMovement(instanceName);
      // Teleport the element on the map
    if (!elementsManager.changeElementCoordinates(elementName, safeX, safeY)) {
        std::cerr << "Failed to teleport entity element: " << instanceName << std::endl;
        return false;
    }
    
    // Disable animation (entity is not walking after teleport)
    elementsManager.changeElementAnimationStatus(elementName, false);
    
    // Reset to default sprite
    
    std::cout << "Teleported entity " << instanceName << " to (" << safeX << ", " << safeY << ")";
    if (needsSafePosition && (safeX != x || safeY != y)) {
        std::cout << " (collision resolved from requested " << x << ", " << y << ")";
    }
    std::cout << std::endl;
    return true;
}

// General function to stop entity movement and clear its path
void EntitiesManager::stopEntityMovement(const std::string& instanceName) {
    // Get the entity
    Entity* entity = getEntity(instanceName);
    if (!entity) {
        std::cerr << "Entity not found for stopping movement: " << instanceName << std::endl;
        return;
    }
    
    // Get the configuration for default sprite settings
    const EntityConfiguration* config = getConfiguration(entity->type);
    
    // Get the element name
    std::string elementName = getElementName(instanceName);
    
    // Stop walking and clear path
    entity->isWalking = false;
    entity->path.clear();
    entity->currentPathIndex = 0;
    
    // Disable animation
    elementsManager.changeElementAnimationStatus(elementName, false);
    
    // Reset to default sprite
    elementsManager.changeElementSpriteFrame(elementName, 0);
      // Cancel any pending pathfinding requests
    if (entity->pathfindingRequestId > 0 && g_entityAsyncPathfinder) {
        g_entityAsyncPathfinder->cancelPathfindingRequest(instanceName);
        entity->pathfindingRequestId = 0;
    }
    entity->isWaitingForPath = false;
    
    // CRITICAL FIX: Clear stuck detection fields to prevent false stuck detection on future movements
    entity->pathfindingTimeoutTimer = 0.0f;
    entity->pathfindingTimeoutActive = false;
    entity->lastPositionX = 0.0f;
    entity->lastPositionY = 0.0f;
    entity->stuckCheckTime = 0.0f;
    entity->lastPositionChangeTime = 0.0f;
    entity->stuckCount = 0;
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
    try {        // Process all completed pathfinding requests
        std::vector<AsyncPathfindingResult> results = g_entityAsyncPathfinder->getCompletedResults();
        
        // CRASH FIX: Validate results vector before processing
        if (results.empty()) {
            return; // No results to process
        }
        
        // Additional safety: Validate each result before processing
        std::vector<AsyncPathfindingResult> validResults;
        validResults.reserve(results.size());
        
        for (const auto& result : results) {
            try {
                // Test that we can safely access the result data
                if (!result.instanceName.empty() && 
                    (!result.success || (!result.path.empty() && result.path.size() > 0))) {
                    validResults.push_back(result);
                } else {
                    std::cerr << "Warning: Skipping invalid pathfinding result for entity: " << result.instanceName << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Exception validating pathfinding result: " << e.what() << std::endl;
            }
        }
        
        for (const auto& result : validResults) {
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
                    // CRASH FIX: Validate path data before any vector operations
                    if (result.path.empty()) {
                        std::cerr << "Warning: Entity " << result.instanceName << " received empty path" << std::endl;
                        continue;
                    }
                    
                    // CRASH FIX: Ensure path has minimum required size
                    if (result.path.size() < 2) {
                        std::cerr << "Warning: Entity " << result.instanceName << " received path with insufficient points: " 
                                  << result.path.size() << std::endl;
                        continue;
                    }
                    
                    // Additional memory safety check
                    try {
                        // Test vector access to ensure memory is valid
                        float testX = result.path[0].first;
                        float testY = result.path[result.path.size() - 1].second;
                        (void)testX; (void)testY; // Suppress unused variable warnings
                    } catch (const std::exception& e) {
                        std::cerr << "CRITICAL: Path data corruption detected for entity " << result.instanceName 
                                  << ": " << e.what() << std::endl;
                        continue;
                    }
                    
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
                        }                        // CRASH FIX: Safe smooth transition with comprehensive bounds checking
                        try {
                            // Create a safe copy of the result path to avoid any potential race conditions
                            std::vector<std::pair<float, float>> safePath = result.path;
                            
                            // Validate the safe path
                            if (safePath.empty()) {
                                std::cerr << "Warning: Safe path copy is empty for entity " << result.instanceName << std::endl;
                                entity->path = result.path;
                                entity->currentPathIndex = 1;
                            } else {
                                entity->path = std::move(safePath);
                                
                                // Ensure bestTransitionIndex is within valid bounds
                                if (bestTransitionIndex >= entity->path.size()) {
                                    bestTransitionIndex = entity->path.size() > 0 ? entity->path.size() - 1 : 0;
                                }
                                
                                entity->currentPathIndex = std::max(1u, static_cast<unsigned int>(bestTransitionIndex));
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "CRITICAL: Exception during safe path assignment for entity " << result.instanceName 
                                      << ": " << e.what() << std::endl;
                            // Emergency fallback
                            entity->path.clear();
                            entity->path.push_back({currentX, currentY});
                            entity->currentPathIndex = 0;
                        }
                          // CRASH FIX: Replace unsafe vector::insert with safer bounds-checked approach
                        if (bestTransitionIndex > 0 && minDistance > 0.5f && bestTransitionIndex < entity->path.size()) {
                            try {
                                // SAFER APPROACH: Create a new vector instead of using insert
                                std::vector<std::pair<float, float>> newPath;
                                newPath.reserve(entity->path.size() + 1); // Pre-allocate memory
                                
                                // Copy elements before insertion point
                                for (size_t i = 0; i < bestTransitionIndex && i < entity->path.size(); ++i) {
                                    newPath.push_back(entity->path[i]);
                                }
                                
                                // Add current position as transition waypoint
                                newPath.push_back({currentX, currentY});
                                
                                // Copy remaining elements
                                for (size_t i = bestTransitionIndex; i < entity->path.size(); ++i) {
                                    newPath.push_back(entity->path[i]);
                                }
                                
                                // Safely replace the path
                                entity->path = std::move(newPath);
                                entity->currentPathIndex = bestTransitionIndex + 1;
                                
                                if (DEBUG_LOGS) {
                                    std::cout << "Entity " << result.instanceName << " path smoothly transitioned with safe vector construction" << std::endl;
                                }
                                
                            } catch (const std::exception& e) {
                                std::cerr << "CRITICAL: Exception during safe path construction for entity " << result.instanceName 
                                          << ": " << e.what() << std::endl;
                                // Fallback: use simple path assignment
                                entity->path = result.path;
                                entity->currentPathIndex = std::min(1u, static_cast<unsigned int>(entity->path.size()));
                            } catch (...) {
                                std::cerr << "CRITICAL: Unknown exception during safe path construction for entity " << result.instanceName << std::endl;
                                // Fallback: use simple path assignment
                                entity->path = result.path;
                                entity->currentPathIndex = std::min(1u, static_cast<unsigned int>(entity->path.size()));
                            }
                        }
                        
                        if (DEBUG_LOGS) {
                            std::cout << "Entity " << result.instanceName << " smoothly transitioned to new path at index " 
                                      << entity->currentPathIndex << " (distance: " << minDistance << ")" << std::endl;
                        }
                          // SPRITE DIRECTION FIX: Update sprite direction to match new path direction
                        // Calculate direction from current position to the target waypoint
                        if (entity->currentPathIndex < entity->path.size()) {
                            try {
                                float currentX, currentY;
                                if (elementsManager.getElementPosition(elementName, currentX, currentY)) {
                                    // CRASH FIX: Validate array access before using
                                    if (entity->currentPathIndex < entity->path.size()) {
                                        const auto& targetWaypoint = entity->path[entity->currentPathIndex];
                                        float dx = targetWaypoint.first - currentX;
                                        float dy = targetWaypoint.second - currentY;
                                        
                                        // Update sprite direction for the new path direction
                                        handleWaypointArrival(*entity, elementName, *config, currentX, currentY);
                                    }
                                }
                            } catch (const std::exception& e) {
                                std::cerr << "CRITICAL: Exception during sprite direction update for entity " << result.instanceName 
                                          << ": " << e.what() << std::endl;
                            }
                        }                    } else {
                        // Fallback: use the new path normally with enhanced safety
                        try {
                            // Create a safe copy to avoid any potential memory issues
                            std::vector<std::pair<float, float>> safeFallbackPath = result.path;
                            
                            if (safeFallbackPath.size() >= 2) {
                                entity->path = std::move(safeFallbackPath);
                                entity->currentPathIndex = 1;
                            } else {
                                std::cerr << "Warning: Fallback path has insufficient points for entity " << result.instanceName 
                                          << ": " << safeFallbackPath.size() << std::endl;
                                entity->path = std::move(safeFallbackPath);
                                entity->currentPathIndex = 0;
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "CRITICAL: Exception during fallback path assignment for entity " << result.instanceName 
                                      << ": " << e.what() << std::endl;
                            // Emergency fallback - create minimal path
                            entity->path.clear();
                            entity->path.push_back({0.0f, 0.0f}); // Fallback position
                            entity->currentPathIndex = 0;
                        }
                        
                        // SPRITE DIRECTION FIX: Update sprite direction for fallback case
                        try {
                            if (entity->path.size() >= 2) {
                                handleWaypointArrival(*entity, elementName, *config, entity->path[0].first, entity->path[0].second);
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "CRITICAL: Exception during fallback sprite direction update for entity " << result.instanceName 
                                      << ": " << e.what() << std::endl;
                        }
                    }                } else {
                    // Entity is not walking, start normally from the beginning
                    try {
                        // CRASH FIX: Safe path assignment with validation
                        std::vector<std::pair<float, float>> safeNewPath = result.path;
                        
                        if (safeNewPath.size() >= 2) {
                            entity->path = std::move(safeNewPath);
                            entity->currentPathIndex = 1;
                        } else if (safeNewPath.size() == 1) {
                            entity->path = std::move(safeNewPath);
                            entity->currentPathIndex = 0;
                            std::cerr << "WARNING: Entity " << result.instanceName << " received single-point path" << std::endl;
                        } else {
                            entity->path.clear();
                            entity->currentPathIndex = 0;
                            std::cerr << "WARNING: Entity " << result.instanceName << " received empty path" << std::endl;
                        }
                        
                        entity->isWalking = true;
                        
                        // Enable animation for entities that weren't walking
                        elementsManager.changeElementAnimationStatus(elementName, true);
                          } catch (const std::exception& e) {
                        std::cerr << "CRITICAL: Exception during new entity path setup for " << result.instanceName 
                                  << ": " << e.what() << std::endl;
                        // Emergency fallback
                        stopEntityMovement(result.instanceName);
                    }
                    
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
            } else {                // Pathfinding failed or path too short
                // If entity was walking before, let it continue its current movement
                // Otherwise, stop the entity
                if (!entity->isWalking) {
                    // Stop the entity using centralized function
                    stopEntityMovement(result.instanceName);
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

// Handle waypoint arrival - update sprite direction for the next segment
bool EntitiesManager::handleWaypointArrival(Entity& entity, const std::string& elementName, const EntityConfiguration& config, float currentX, float currentY) {
    // If we don't have a next waypoint, nothing to update
    if (entity.currentPathIndex >= entity.path.size()) {
        return false;
    }
    
    // Get the next waypoint
    const auto& nextWaypoint = entity.path[entity.currentPathIndex];
    float dx = nextWaypoint.first - currentX;
    float dy = nextWaypoint.second - currentY;
    
    // Only update direction if there's actual movement
    if (dx == 0 && dy == 0) {
        return false;
    }
    
    // Determine sprite direction based on movement
    int spritePhase = config.defaultSpriteSheetPhase;
    
    // Use a threshold to determine the primary direction
    const float directionThreshold = 0.2f;
    
    if (std::abs(dx) > std::abs(dy) + directionThreshold) {
        // Primary horizontal movement
        if (dx > 0) {
            spritePhase = config.spritePhaseWalkRight;
            entity.lastDirection = 3; // Right
        } else {
            spritePhase = config.spritePhaseWalkLeft;
            entity.lastDirection = 2; // Left
        }
    } else if (std::abs(dy) > std::abs(dx) + directionThreshold) {
        // Primary vertical movement
        if (dy > 0) {
            spritePhase = config.spritePhaseWalkDown;
            entity.lastDirection = 1; // Down
        } else {
            spritePhase = config.spritePhaseWalkUp;
            entity.lastDirection = 0; // Up
        }
    } else {
        // Diagonal movement - choose based on the larger component
        if (std::abs(dx) >= std::abs(dy)) {
            if (dx > 0) {
                spritePhase = config.spritePhaseWalkRight;
                entity.lastDirection = 3; // Right
            } else {
                spritePhase = config.spritePhaseWalkLeft;
                entity.lastDirection = 2; // Left
            }
        } else {
            if (dy > 0) {
                spritePhase = config.spritePhaseWalkDown;
                entity.lastDirection = 1; // Down
            } else {
                spritePhase = config.spritePhaseWalkUp;
                entity.lastDirection = 0; // Up
            }
        }
    }
    
    // Update sprite phase
    elementsManager.changeElementSpritePhase(elementName, spritePhase);
    
    return true;
}

// Enhanced entity collision function that respects granular entity collision settings
bool wouldEntityCollideWithEntitiesGranular(const EntityConfiguration& config, float x, float y, bool useAvoidanceList, const std::string& excludeInstanceName) {
    if (!config.canCollide) {
        return false; // Entity doesn't have collision enabled
    }
    
    // Check map boundary collision if offMapCollision is enabled
    if (config.offMapCollision) {
        if (wouldEntityCollideWithMapBounds(config, x, y)) {
            return true; // Entity collision shape would collide with map boundary
        }
    }
    
    // Determine which entity list to check based on collision type
    const std::vector<EntityName>& entitiesToCheck = useAvoidanceList ? config.avoidanceEntities : config.collisionEntities;
    
    // If the list is empty, don't check any entities (granular collision control)
    // This allows entities to have fine-grained control over what they collide with
    if (entitiesToCheck.empty()) {
        return false; // No entities to check = no collision
    }
    
    // Convert to std::set for faster lookup
    std::set<EntityName> entitySet(entitiesToCheck.begin(), entitiesToCheck.end());    // If entity has no collision shape, use center point collision
    if (config.collisionShapePoints.empty()) {
        // PERFORMANCE OPTIMIZATION: Use hierarchical entity grid instead of getNearbyEntities
        const float pointCollisionRadius = 2.0f; // Search radius for nearby entities
        std::vector<std::string> nearbyEntityNames;
        
        // Thread-safe access to hierarchical entity grid
        {
            std::lock_guard<std::mutex> lock(g_hierarchicalEntityGridMutex);
            // Initialize hierarchical entity grid if needed
            if (!g_hierarchicalEntityGrid.isInitializedState()) {
                g_hierarchicalEntityGrid.initialize();
            }
            
            // Update grid before collision check
            g_hierarchicalEntityGrid.updateGrid();
            
            // Get nearby entities using hierarchical lookup (REPLACES O(N²) getNearbyEntities)
            nearbyEntityNames = g_hierarchicalEntityGrid.getEntitiesHierarchical(x, y, pointCollisionRadius);
        }
        
        for (const std::string& otherInstanceName : nearbyEntityNames) {
            // Skip self and excluded entity
            if (otherInstanceName == excludeInstanceName) {
                continue;
            }
            
            // Get entity reference
            const Entity* otherEntity = entitiesManager.getEntity(otherInstanceName);
            if (!otherEntity) {
                continue; // Entity no longer exists
            }
            
            // Check if this entity type is in our collision list
            if (entitySet.find(otherEntity->type) == entitySet.end()) {
                continue; // Skip entities not in our collision/avoidance list
            }
            
            // Get other entity position and configuration
            std::string otherElementName = EntitiesManager::getElementName(otherEntity->instanceName);
            float otherX, otherY;
            if (!elementsManager.getElementPosition(otherElementName, otherX, otherY)) {
                continue; // Can't get position, skip this entity
            }
            
            const EntityConfiguration* otherConfig = entitiesManager.getConfiguration(otherEntity->type);
            if (!otherConfig || !otherConfig->canCollide) {
                continue; // Other entity doesn't have collision enabled
            }
            
            // Simple distance check for point-based collision
            float dx = x - otherX;
            float dy = y - otherY;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            // Use a small collision radius for point-based collision
            if (distance < 0.5f) {
                return true; // Collision detected
            }
        }
        return false;
    }// PERFORMANCE OPTIMIZATION: Use pre-calculated collision boxes for massive speedup
    // This replaces expensive real-time transformation calculations
    PreCalculatedCollisionBox entityCollisionBox;
    const auto& entityCachedBox = CollisionBoxUtils::getOrUpdateCollisionBox(
        entityCollisionBox, config.collisionShapePoints, x, y, 0.0f, 1.0f);
        // CRASH FIX: Check if transformation actually produced any points
    if (entityCachedBox.worldPoints.empty()) {
        std::cerr << "CRITICAL: entityCachedBox.worldPoints is empty after transformation - using fallback collision detection" << std::endl;
        // Fallback to point-based collision with hierarchical spatial optimization
        const float fallbackRadius = 2.0f;
        std::vector<std::string> nearbyEntityNames;
        
        // Thread-safe access to hierarchical entity grid
        {
            std::lock_guard<std::mutex> lock(g_hierarchicalEntityGridMutex);
            // Initialize hierarchical entity grid if needed
            if (!g_hierarchicalEntityGrid.isInitializedState()) {
                g_hierarchicalEntityGrid.initialize();
            }
            
            // Update grid before collision check
            g_hierarchicalEntityGrid.updateGrid();
            
            // Get nearby entities using hierarchical lookup (REPLACES O(N²) getNearbyEntities)
            nearbyEntityNames = g_hierarchicalEntityGrid.getEntitiesHierarchical(x, y, fallbackRadius);
        }
        
        for (const std::string& otherInstanceName : nearbyEntityNames) {
            // Skip self and excluded entity
            if (otherInstanceName == excludeInstanceName) {
                continue;
            }
            
            // Get entity reference
            const Entity* otherEntity = entitiesManager.getEntity(otherInstanceName);
            if (!otherEntity) {
                continue; // Entity no longer exists
            }
            
            // Check if this entity type is in our collision list
            if (entitySet.find(otherEntity->type) == entitySet.end()) {
                continue;
            }
            
            // Get other entity position
            std::string otherElementName = EntitiesManager::getElementName(otherEntity->instanceName);
            float otherX, otherY;
            if (!elementsManager.getElementPosition(otherElementName, otherX, otherY)) {
                continue;
            }
            
            // Simple distance check
            float dx = x - otherX;
            float dy = y - otherY;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance < 0.5f) {
                return true;
            }
        }
        return false;
    }    // Check collision with nearby entities using hierarchical spatial optimization
    // Calculate search radius based on entity collision shape
    float searchRadius = 0.0f;
    for (const auto& point : config.collisionShapePoints) {
        float dist = std::sqrt(point.first * point.first + point.second * point.second);
        searchRadius = std::max(searchRadius, dist);
    }
    searchRadius += 5.0f; // Add buffer for other entity collision shapes
    
    std::vector<std::string> nearbyEntityNames;
    
    // PERFORMANCE OPTIMIZATION: Use hierarchical entity grid instead of O(N²) getNearbyEntities
    {
        std::lock_guard<std::mutex> lock(g_hierarchicalEntityGridMutex);
        // Initialize hierarchical entity grid if needed
        if (!g_hierarchicalEntityGrid.isInitializedState()) {
            g_hierarchicalEntityGrid.initialize();
        }
        
        // Update grid before collision check
        g_hierarchicalEntityGrid.updateGrid();
        
        // Get nearby entities using hierarchical lookup (MASSIVE PERFORMANCE IMPROVEMENT)
        nearbyEntityNames = g_hierarchicalEntityGrid.getEntitiesHierarchical(x, y, searchRadius);
    }
    
    for (const std::string& otherInstanceName : nearbyEntityNames) {
        // Skip self and excluded entity
        if (otherInstanceName == excludeInstanceName) {
            continue;
        }
        
        // Get entity reference
        const Entity* otherEntity = entitiesManager.getEntity(otherInstanceName);
        if (!otherEntity) {
            continue; // Entity no longer exists
        }
        
        // Check if this entity type is in our collision list
        if (entitySet.find(otherEntity->type) == entitySet.end()) {
            continue; // Skip entities not in our collision/avoidance list
        }
        
        // Get other entity position and configuration
        std::string otherElementName = EntitiesManager::getElementName(otherEntity->instanceName);
        float otherX, otherY;
        if (!elementsManager.getElementPosition(otherElementName, otherX, otherY)) {
            continue; // Can't get position, skip this entity
        }
        
        const EntityConfiguration* otherConfig = entitiesManager.getConfiguration(otherEntity->type);
        if (!otherConfig || !otherConfig->canCollide || otherConfig->collisionShapePoints.empty()) {
            continue; // Other entity doesn't have collision enabled or collision shape
        }// PERFORMANCE OPTIMIZATION: Use pre-calculated collision boxes for other entity
            // Get or calculate cached collision box for other entity using its cached collision box from Entity struct
            const auto& otherEntityCachedBox = CollisionBoxUtils::getOrUpdateCollisionBox(
                const_cast<Entity*>(otherEntity)->cachedCollisionBox, otherConfig->collisionShapePoints, 
                otherX, otherY, 0.0f, 1.0f);
            
            // Fast bounding box intersection test first
            if (!entityCachedBox.boundingBoxIntersects(otherEntityCachedBox)) {
                continue; // No intersection possible, skip expensive polygon test
            }
              
            // Perform polygon-polygon collision detection using cached world points
            // MEMORY SAFETY: Check if both polygons have valid points before collision detection
            if (!entityCachedBox.worldPoints.empty() && !otherEntityCachedBox.worldPoints.empty() &&
                entityCachedBox.worldPoints.size() >= 3 && otherEntityCachedBox.worldPoints.size() >= 3) {
                
                // Additional safety check: ensure cached vectors are not corrupted
                bool entityShapeValid = true;
                bool otherEntityShapeValid = true;
                
                for (const auto& point : entityCachedBox.worldPoints) {
                    if (std::isnan(point.first) || std::isnan(point.second) ||
                        std::isinf(point.first) || std::isinf(point.second)) {
                        entityShapeValid = false;
                        break;
                    }
                }
                
                if (entityShapeValid) {
                    for (const auto& point : otherEntityCachedBox.worldPoints) {
                        if (std::isnan(point.first) || std::isnan(point.second) ||
                            std::isinf(point.first) || std::isinf(point.second)) {
                            otherEntityShapeValid = false;
                            break;
                        }
                    }
                }
                
                // Only perform collision detection if both shapes are valid
                if (entityShapeValid && otherEntityShapeValid) {
                    try {
                        if (polygonPolygonCollision(entityCachedBox.worldPoints, otherEntityCachedBox.worldPoints)) {
                            return true; // Collision detected
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "WARNING: Exception in polygonPolygonCollision: " << e.what() << std::endl;
                        continue; // Skip this collision check and continue with next entity
                    } catch (...) {
                        std::cerr << "WARNING: Unknown exception in polygonPolygonCollision" << std::endl;
                        continue; // Skip this collision check and continue with next entity
                    }
                }
            }
    }
    
    return false; // No collision detected with specified entity types
}

// Spatial grid system management

// Get spatial grid cell index for entity positions
int getEntitySpatialGridIndex(float x, float y) {
    int gridX = static_cast<int>(x) / ENTITY_SPATIAL_GRID_SIZE;
    int gridY = static_cast<int>(y) / ENTITY_SPATIAL_GRID_SIZE;
    return gridX * 1000 + gridY;
}

// Update the entity spatial partitioning grid
void updateEntitySpatialGrid() {
    PROFILE_SCOPE("EntitySpatialGrid_Update");
    
    float currentTime = static_cast<float>(glfwGetTime());
    
    // Only update periodically to avoid performance impact
    if (entitySpatialGridInitialized && currentTime - lastEntitySpatialGridUpdateTime < ENTITY_SPATIAL_GRID_UPDATE_INTERVAL) {
        return;
    }
    
    lastEntitySpatialGridUpdateTime = currentTime;
    entitySpatialGrid.clear();
    
    // Place each entity in the appropriate grid cell
    const auto& allEntities = entitiesManager.getEntities();
    for (const auto& pair : allEntities) {
        const Entity& entity = pair.second;
        
        // Get entity position
        std::string elementName = EntitiesManager::getElementName(entity.instanceName);
        float entityX, entityY;
        if (elementsManager.getElementPosition(elementName, entityX, entityY)) {
            int index = getEntitySpatialGridIndex(entityX, entityY);
            entitySpatialGrid[index].push_back(entity.instanceName);
        }
    }
    
    entitySpatialGridInitialized = true;
}

// Function to reset entity spatial grid when entities are added/removed
void resetEntitySpatialGrid() {
    entitySpatialGridInitialized = false;
    entitySpatialGrid.clear();
}

// Get nearby entities within a radius using spatial grid
std::vector<std::string> getNearbyEntities(float x, float y, float radius) {
    std::vector<std::string> result;
    
    // Make sure the spatial grid is up to date
    updateEntitySpatialGrid();
    
    // Calculate the grid cell range that could contain entities within the radius
    int cellRadius = static_cast<int>(radius / ENTITY_SPATIAL_GRID_SIZE) + 1;
    int centerCellX = static_cast<int>(x) / ENTITY_SPATIAL_GRID_SIZE;
    int centerCellY = static_cast<int>(y) / ENTITY_SPATIAL_GRID_SIZE;
    
    // Check all cells in the vicinity
    for (int cellX = centerCellX - cellRadius; cellX <= centerCellX + cellRadius; cellX++) {
        for (int cellY = centerCellY - cellRadius; cellY <= centerCellY + cellRadius; cellY++) {
            int index = cellX * 1000 + cellY;
            
            // Get all entities in this grid cell
            auto it = entitySpatialGrid.find(index);
            if (it != entitySpatialGrid.end()) {
                // Add entities from this cell to the result
                result.insert(result.end(), it->second.begin(), it->second.end());
            }
        }
    }
    
    return result;
}