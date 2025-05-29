#pragma once

#include "elementsOnMap.h"
#include "map.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath> // For distance calculations
#include <utility> // For std::pair
#include <chrono> // For async pathfinding timing
#include "enumDefinitions.h"


// Enum for entity directions (corresponds to sprite sheet rows/phases)
enum EntityDirection {
    DIRECTION_UP = 0,    // Or 3, depending on sprite sheet convention
    DIRECTION_DOWN = 1,  // Or 0
    DIRECTION_LEFT = 2,
    DIRECTION_RIGHT = 3,
    DIRECTION_NONE = -1 // For when no specific direction is set
};

// Enum for walk types
enum class WalkType {
    NORMAL,
    SPRINT
};

// Struct to hold predefined entity types information
struct EntityInfo {
    EntityName type;
    ElementName textureName;
    float scale;
    
    // Sprite configuration
    int defaultSpriteSheetPhase;
    int defaultSpriteSheetFrame;
    float defaultAnimationSpeed;
    
    // Walking animation phases
    int spritePhaseWalkUp;
    int spritePhaseWalkDown;
    int spritePhaseWalkLeft;
    int spritePhaseWalkRight;
    
    // Movement speeds
    float normalWalkingSpeed;
    float normalWalkingAnimationSpeed;
    float sprintWalkingSpeed;
    float sprintWalkingAnimationSpeed;      // Collision settings
    bool canCollide;
    std::vector<std::pair<float, float>> collisionShapePoints;      // Granular collision control - specify which element types to avoid or collide with
    std::vector<ElementName> avoidanceElements; // Elements this entity will pathfind around (but can overlap if forced)
    std::vector<ElementName> collisionElements; // Elements this entity cannot overlap with at all
    
    // Granular block collision control - specify which block types to avoid or collide with
    std::vector<BlockName> avoidanceBlocks; // Blocks this entity will pathfind around (but can overlap if forced)
    std::vector<BlockName> collisionBlocks; // Blocks this entity cannot overlap with at all
      // Map boundary control
    bool offMapAvoidance = true; // Entity pathfinding will avoid going outside the map grid
    bool offMapCollision = true; // Entity will collide with map borders during movement
    
    // Automatic behavior system
    bool automaticBehaviors = false; // Enable/disable automatic behaviors for this entity    // Passive state behavior parameters
    bool passiveState = false; // Enable passive state random walking behavior
    float passiveStateWalkingRadius = 10.0f; // Radius around entity for random walks
    float passiveStateRandomWalkTriggerTimeIntervalMin = 2.0f; // Minimum time between random walks (seconds)
    float passiveStateRandomWalkTriggerTimeIntervalMax = 8.0f; // Maximum time between random walks (seconds)
};

// Struct to hold entity configuration
struct EntityConfiguration {
    EntityName type;
    ElementName textureName;
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
    float sprintWalkingAnimationSpeed = 12.0f;    // Collision settings
    bool canCollide = true;
    std::vector<std::pair<float, float>> collisionShapePoints;
      // Granular collision control - specify which element types to avoid or collide with
    std::vector<ElementName> avoidanceElements; // Elements this entity will pathfind around (but can overlap if forced)
    std::vector<ElementName> collisionElements; // Elements this entity cannot overlap with at all
    
    // Granular block collision control - specify which block types to avoid or collide with
    std::vector<BlockName> avoidanceBlocks; // Blocks this entity will pathfind around (but can overlap if forced)
    std::vector<BlockName> collisionBlocks; // Blocks this entity cannot overlap with at all
      // Map boundary control
    bool offMapAvoidance = true; // Entity pathfinding will avoid going outside the map grid
    bool offMapCollision = true; // Entity will collide with map borders during movement
      // Automatic behavior system
    bool automaticBehaviors = false; // Enable/disable automatic behaviors for this entity
      // Passive state behavior parameters
    bool passiveState = false; // Enable passive state random walking behavior
    float passiveStateWalkingRadius = 10.0f; // Radius around entity for random walks
    float passiveStateRandomWalkTriggerTimeIntervalMin = 2.0f; // Minimum time between random walks (seconds)
    float passiveStateRandomWalkTriggerTimeIntervalMax = 8.0f; // Maximum time between random walks (seconds)
    
    // Constructor to create from EntityInfo
    EntityConfiguration() = default;
      EntityConfiguration(const EntityInfo& info) {
        type = info.type;
        textureName = info.textureName;
        scale = info.scale;
        
        defaultSpriteSheetPhase = info.defaultSpriteSheetPhase;
        defaultSpriteSheetFrame = info.defaultSpriteSheetFrame;
        defaultAnimationSpeed = info.defaultAnimationSpeed;
        
        spritePhaseWalkUp = info.spritePhaseWalkUp;
        spritePhaseWalkDown = info.spritePhaseWalkDown;
        spritePhaseWalkLeft = info.spritePhaseWalkLeft;
        spritePhaseWalkRight = info.spritePhaseWalkRight;
          normalWalkingSpeed = info.normalWalkingSpeed;
        normalWalkingAnimationSpeed = info.normalWalkingAnimationSpeed;
        sprintWalkingSpeed = info.sprintWalkingSpeed;
        sprintWalkingAnimationSpeed = info.sprintWalkingAnimationSpeed;
        
        canCollide = info.canCollide;
        collisionShapePoints = info.collisionShapePoints;        // Copy granular collision settings
        avoidanceElements = info.avoidanceElements;
        collisionElements = info.collisionElements;
        
        // Copy granular block collision settings
        avoidanceBlocks = info.avoidanceBlocks;
        collisionBlocks = info.collisionBlocks;
          // Copy map boundary control settings
        offMapAvoidance = info.offMapAvoidance;
        offMapCollision = info.offMapCollision;        // Copy automatic behavior settings
        automaticBehaviors = info.automaticBehaviors;
        passiveState = info.passiveState;
        passiveStateWalkingRadius = info.passiveStateWalkingRadius;
        passiveStateRandomWalkTriggerTimeIntervalMin = info.passiveStateRandomWalkTriggerTimeIntervalMin;
        passiveStateRandomWalkTriggerTimeIntervalMax = info.passiveStateRandomWalkTriggerTimeIntervalMax;
    }
};

// Struct to hold entity instance data
struct Entity {
    std::string instanceName;
    EntityName type;
    
    // Movement target - only used when walking to a location
    bool isWalking = false;
    float targetX = 0.0f;
    float targetY = 0.0f;
    WalkType walkType = WalkType::NORMAL;
      // Direction tracking
    int lastDirection = 1; // 0 = Up, 1 = Down, 2 = Left, 3 = Right
    float directionChangeTime = 0.0f; // Time accumulator for direction change smoothing
      // Pathfinding data
    bool usePathfinding = true; // Whether to use pathfinding for movement
    std::vector<std::pair<float, float>> path; // Current path from pathfinding
    size_t currentPathIndex = 0; // Current position in the path
    std::pair<float, float> lastSegmentDirection = {0.0f, 0.0f}; // Last major direction for path smoothing
    
    // Async pathfinding state
    bool isPathfindingRequested = false;
    bool hasValidPath = false;
    int pathfindingRequestId = -1;
    std::chrono::steady_clock::time_point lastPathRequest;
    
    // Path following state
    float pathfindingCooldown = 0.5f; // Minimum time between requests
    bool isWaitingForPath = false;
    
    // Movement tracking for stuck detection
    float lastPositionX = 0.0f;
    float lastPositionY = 0.0f;
    float lastPositionChangeTime = 0.0f;
    float stuckCheckTime = 0.0f;
    int stuckCount = 0;    float interactionRadius;
    EntityDirection currentSegmentSpriteDirection = DIRECTION_DOWN; // Initialize to a default
    std::string currentBehavior;
      // Automatic behavior state variables
    double behaviorTimer = 0.0; // Time accumulator for behavior timing
    double nextBehaviorTriggerTime = 0.0; // When the next behavior should trigger
};

// EntitiesManager - Manages all entities in the game
class EntitiesManager {
public:
    EntitiesManager();
    ~EntitiesManager();
    
    // Initialize entities from predefined configurations
    void initializeEntityConfigurations();    // Get a configuration by type name
    const EntityConfiguration* getConfiguration(const std::string& typeName) const;
    const EntityConfiguration* getConfiguration(EntityName entityType) const;
    
    // Add a new entity configuration
    void addConfiguration(const EntityConfiguration& config);
      // Place a new entity on the map
    bool placeEntity(const std::string& instanceName, const std::string& typeName, float x, float y);
    bool placeEntity(const std::string& instanceName, EntityName entityType, float x, float y);
      // Helper method to get element name from entity instance name
    static std::string getElementName(const std::string& instanceName);
      // Place a predefined entity by type
    bool placeEntityByType(const std::string& instanceName, const std::string& typeName, float x, float y);
    bool placeEntityByType(const std::string& instanceName, EntityName entityType, float x, float y);
    
    // Get entity type name from instance name
    std::string getEntityType(const std::string& instanceName) const;
    
    // Move an entity to specific coordinates (will walk there)
    bool moveEntity(const std::string& instanceName, float x, float y);
    
    // Teleport an entity to specific coordinates immediately (handles collisions)
    bool teleportEntity(const std::string& instanceName, float x, float y);
      // Walk an entity to specific coordinates
      // Walk an entity to specific coordinates using pathfinding
    bool walkEntityWithPathfinding(const std::string& instanceName, float x, float y, WalkType walkType = WalkType::NORMAL);
    
    // Find the nearest safe place from given coordinates for an entity
    bool findNearestSafePlaceFromCoordinatesForEntity(const std::string& instanceName, float x, float y, float& safeX, float& safeY);
    
    // Find a random safe point around an entity within a given radius
    bool findRandomSafePointAroundTheEntity(const std::string& instanceName, float radius, float& randomX, float& randomY);
    
    // Walk entity to a random accessible point within the given radius
    bool walkEntityWithPathFindingToRandomRadiusTarget(const std::string& instanceName, float radius, WalkType walkType = WalkType::NORMAL);
    
    // Initialize async pathfinding system
    void initializeAsyncPathfinding();
    
    // Shutdown async pathfinding system
    void shutdownAsyncPathfinding();
              // Update all entities (called once per frame)
    void update(double deltaTime);
      // Collision resolution functions removed - entities will no longer be automatically moved    // Public access for entity iteration (for behavior manager)
    std::map<std::string, Entity>& getEntities() { return entities; }
    const std::map<std::string, Entity>& getEntities() const { return entities; }
      // CRASH FIX: Check if entity exists safely
    bool entityExists(const std::string& instanceName) const;

    // Get entity by instance name (made public for EntityBehaviorManager access)
    Entity* getEntity(const std::string& instanceName);

    // Draw debug paths for all entities
    void drawDebugPaths(float startX, float endX, float startY, float endY, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop);    // Draw debug collision radii for all entities
    void drawDebugCollisionRadii(float startX, float endX, float startY, float endY, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop);
    
private:
    std::map<EntityName, EntityConfiguration> configurations;    std::map<std::string, Entity> entities;    // Helper methods
    
    // Get the next waypoint from the entity's path
    bool getNextPathWaypoint(Entity& entity, float& nextX, float& nextY);
      // Update entity walking (internal method)
    void updateEntityWalking(Entity& entity, const EntityConfiguration& config, double deltaTime);      // Handle waypoint arrival - added to improve movement precision
    bool handleWaypointArrival(Entity& entity, const std::string& elementName, const EntityConfiguration& config, float currentX, float currentY);
    
    // Process async pathfinding results
    void processAsyncPathfindingResults();
    
    // COLLISION RESOLUTION FUNCTIONS (DISABLED)
    // These functions are disabled but declarations kept for compatibility
    bool ensureEntityNotStuck(const std::string& instanceName);
};

// Function to check entity collision with elements (uses collision shape points if available, otherwise fallback to radius)
bool wouldEntityCollideWithElement(const EntityConfiguration& config, float x, float y);

// Enhanced collision function that respects granular collision settings
// useAvoidanceList: true = check avoidanceElements, false = check collisionElements
bool wouldEntityCollideWithElementsGranular(const EntityConfiguration& config, float x, float y, bool useAvoidanceList = false);

// Enhanced block collision function that respects granular block collision settings
// useAvoidanceList: true = check avoidanceBlocks, false = check collisionBlocks
bool wouldEntityCollideWithBlocksGranular(const EntityConfiguration& config, float x, float y, bool useAvoidanceList = false);

// Global instance
extern EntitiesManager entitiesManager;