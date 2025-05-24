#include "entityBehaviors.h"
#include "entities.h"
#include "globals.h" // For GRID_SIZE
#include "map.h" // For accessing block types
#include "collision.h" // For wouldCollideWithElement and findSafePosition
#include <iostream>
#include <cmath>
#include <algorithm>

// Global instance
EntityBehaviorManager entityBehaviorManager;

// Forward declaration for entitiesManager access (defined in entities.cpp)
extern EntitiesManager entitiesManager;
// Forward declaration for accessing the global map instance
extern Map gameMap;

EntityBehaviorManager::EntityBehaviorManager() 
    // Initialize random number generator with a time-based seed
    : rng(std::chrono::system_clock::now().time_since_epoch().count())
{
    // Empty constructor
}

EntityBehaviorManager::~EntityBehaviorManager() {
    // Empty destructor
}

void EntityBehaviorManager::initialize() {
    // Register built-in behaviors for entity types
    registerBuiltInBehaviors();
    std::cout << "Entity behavior system initialized" << std::endl;
}

void EntityBehaviorManager::registerBuiltInBehaviors() {
    // Register the antagonist behavior
    behaviorFunctions["antagonist"] = antagonistBehavior;
    
    // Add more behaviors for different entity types here
    
    std::cout << "Registered " << behaviorFunctions.size() << " built-in behaviors" << std::endl;
}

void EntityBehaviorManager::registerEntity(const std::string& instanceName, const std::string& typeName) {
    // Check if we have a behavior for this entity type
    if (behaviorFunctions.find(typeName) == behaviorFunctions.end()) {
        std::cout << "No behavior defined for entity type: " << typeName << std::endl;
        return;
    }
    
    // Create a new behavior state for this entity
    EntityBehaviorState state(instanceName, typeName);
    
    // Initialize with a random delay before first action
    state.nextActionDelay = getRandomDelay();
    state.lastActionTime = static_cast<float>(glfwGetTime());
    
    // Store the entity state
    entityStates[instanceName] = state;
    
    std::cout << "Registered entity " << instanceName << " (type: " << typeName << ") for behavior management" << std::endl;
}

void EntityBehaviorManager::unregisterEntity(const std::string& instanceName) {
    // Remove the entity from our managed states
    auto it = entityStates.find(instanceName);
    if (it != entityStates.end()) {
        entityStates.erase(it);
        std::cout << "Unregistered entity " << instanceName << " from behavior management" << std::endl;
    }
}

void EntityBehaviorManager::update(float deltaTime) {
    // Current time
    float currentTime = static_cast<float>(glfwGetTime());
    
    // For each entity state
    for (auto& pair : entityStates) {
        EntityBehaviorState& state = pair.second;
        
        // Skip inactive entities
        if (!state.isActive) {
            continue;
        }
        
        // Get the behavior function for this entity type
        auto behaviorIt = behaviorFunctions.find(state.typeName);
        if (behaviorIt == behaviorFunctions.end()) {
            std::cerr << "Error: No behavior function found for entity type: " << state.typeName << std::endl;
            continue;
        }
        
        // Execute the behavior
        behaviorIt->second(state, deltaTime, *this);
    }
}

bool EntityBehaviorManager::getEntityPosition(const std::string& instanceName, float& x, float& y) {
    // Generate element name for this entity
    std::string elementName = EntitiesManager::getElementName(instanceName);
    
    // Get the position from the elements manager
    return elementsManager.getElementPosition(elementName, x, y);
}

bool EntityBehaviorManager::moveEntityTo(const std::string& instanceName, float x, float y) {
    // Use the entities manager to move the entity
    return entitiesManager.moveEntity(instanceName, x, y);
}

bool EntityBehaviorManager::hasEntityReachedTarget(const std::string& instanceName) {
    // Find the entity state
    auto it = entityStates.find(instanceName);
    if (it == entityStates.end()) {
        return false;
    }
    
    // Get current position
    float currentX, currentY;
    if (!getEntityPosition(instanceName, currentX, currentY)) {
        return false;
    }
    
    // Get the target from the state
    float targetX = it->second.targetX;
    float targetY = it->second.targetY;
    
    // Calculate distance to target
    float dx = targetX - currentX;
    float dy = targetY - currentY;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Check if entity is close enough to target (using a small threshold)
    const float reachThreshold = 0.2f;
    return distance <= reachThreshold;
}

bool EntityBehaviorManager::getRandomPositionNear(const std::string& instanceName, float& x, float& y, float radius) {
    // Get current position
    float currentX, currentY;
    if (!getEntityPosition(instanceName, currentX, currentY)) {
        return false;
    }
    
    // Create distributions for random offsets in X and Y
    std::uniform_real_distribution<float> distribution(-radius, radius);
    
    // Generate random offsets
    float offsetX = distribution(rng);
    float offsetY = distribution(rng);
    
    // Calculate new position
    x = currentX + offsetX;
    y = currentY + offsetY;
    
    // Clamp to grid boundaries
    x = std::max(0.0f, std::min(static_cast<float>(GRID_SIZE - 1), x));
    y = std::max(0.0f, std::min(static_cast<float>(GRID_SIZE - 1), y));
    
    return true;
}

bool EntityBehaviorManager::getRandomPositionNearOnBlockTypes(const std::string& instanceName, float& x, float& y, float radius) {
    // Get current position
    float currentX, currentY;
    if (!getEntityPosition(instanceName, currentX, currentY)) {
        return false;
    }
    
    // Collect valid positions (on whitelisted block types)
    std::vector<std::pair<float, float>> validPositions;
    
    // Define the radius of blocks to check around the entity
    int blockRadius = static_cast<int>(std::ceil(radius));
    
    // Define the block types we want to target
    const std::vector<TextureName> targetBlockTypes = {
        TextureName::SAND,
        TextureName::GRASS_0,
        TextureName::GRASS_1,
        TextureName::GRASS_2
    };
    
    // Check blocks in a square area around the entity
    int startX = std::max(0, static_cast<int>(currentX) - blockRadius);
    int endX = std::min(GRID_SIZE - 1, static_cast<int>(currentX) + blockRadius);
    int startY = std::max(0, static_cast<int>(currentY) - blockRadius);
    int endY = std::min(GRID_SIZE - 1, static_cast<int>(currentY) + blockRadius);
    
    // Get the entity configuration to find its collision radius
    float collisionRadius = 0.4f; // Default
    auto* config = entitiesManager.getConfiguration("antagonist");
    if (config) {
        collisionRadius = config->collisionRadius;
    }
      // Increase collision check radius significantly to stay further from obstacles
    float safetyBuffer = 0.4f; // Increased from 0.2f for better clearance
    float safeCollisionRadius = collisionRadius + safetyBuffer;
    
    for (int gridY = startY; gridY <= endY; gridY++) {
        for (int gridX = startX; gridX <= endX; gridX++) {
            // Calculate distance from current position to this grid cell
            float dx = gridX - currentX;
            float dy = gridY - currentY;
            float distanceSquared = dx * dx + dy * dy;
            
            // If the cell is within the radius
            if (distanceSquared <= radius * radius) {
                // Get the block type at this position
                TextureName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
                
                // Check if this block type is in our whitelist
                bool isTargetType = std::find(targetBlockTypes.begin(), targetBlockTypes.end(), blockType) != targetBlockTypes.end();
                
                if (isTargetType) {                    // Calculate position with a small random offset to avoid grid alignment
                    // Reduced offset range to keep positions more centered in safe cells
                    std::uniform_real_distribution<float> smallOffset(-0.2f, 0.2f);
                    float offsetX = smallOffset(rng);
                    float offsetY = smallOffset(rng);
                    
                    float posX = static_cast<float>(gridX) + offsetX;
                    float posY = static_cast<float>(gridY) + offsetY;
                    
                    // Add increased safety buffer to stay further from obstacles
                    float safetyBuffer = 0.4f; // Increased from 0.2f
                    float safeCollisionRadius = collisionRadius + safetyBuffer;                      // Check element collisions with increased safety radius using entity configuration
                    const EntityConfiguration* entityConfig = entitiesManager.getConfiguration(entitiesManager.getEntityType(instanceName));
                    
                    if (entityConfig) {
                        // Use polygon-based collision detection for entities
                        bool elementCollision = wouldCollideWithElement(posX, posY, safeCollisionRadius);
                        if (!elementCollision) {
                            // Check nearby elements to ensure we're not too close to obstacles
                            std::vector<std::string> nearbyElements = getNearbyElements(posX, posY, safeCollisionRadius * 1.5f);
                            bool tooCloseToObstacle = false;
                            
                            // Additional check for nearby elements
                            for (const auto& elementName : nearbyElements) {
                                float elementX, elementY;
                                if (elementsManager.getElementPosition(elementName, elementX, elementY)) {
                                    float dx = posX - elementX;
                                    float dy = posY - elementY;
                                    float distanceSquared = dx*dx + dy*dy;
                                    
                                    // Get the element's collision radius
                                    float elementRadius = 0.4f; // Default
                                    const auto& elements = elementsManager.getElements();
                                    for (const auto& element : elements) {
                                        if (element.instanceName == elementName) {
                                            elementRadius = element.collisionRadius;
                                            break;
                                        }
                                    }
                                    
                                    // Check if we're too close to this element
                                    float minDistanceSquared = (safeCollisionRadius + elementRadius) * 
                                                              (safeCollisionRadius + elementRadius);
                                    if (distanceSquared < minDistanceSquared) {
                                        tooCloseToObstacle = true;
                                        break;
                                    }
                                }
                            }
                            
                            if (!tooCloseToObstacle) {
                                validPositions.push_back(std::make_pair(posX, posY));
                            }
                        }
                    }
                }
            }
        }
    }
    
    // If we found valid positions, pick one randomly
    if (!validPositions.empty()) {
        std::uniform_int_distribution<size_t> posIndexDist(0, validPositions.size() - 1);
        size_t randomIndex = posIndexDist(rng);
        
        x = validPositions[randomIndex].first;
        y = validPositions[randomIndex].second;
        
        return true;
    }
    
    // Fallback to regular random position if no valid positions found
    // This helps ensure the entity can always move somewhere
    return getRandomPositionNear(instanceName, x, y, radius);
}

float EntityBehaviorManager::getRandomDelay(float minSeconds, float maxSeconds) {
    std::uniform_real_distribution<float> distribution(minSeconds, maxSeconds);
    return distribution(rng);
}

// Define the antagonist behavior
void EntityBehaviorManager::antagonistBehavior(EntityBehaviorState& state, float deltaTime, EntityBehaviorManager& manager) {
    // Current time
    float currentTime = static_cast<float>(glfwGetTime());
    
    // Periodically select a new target, regardless of whether the previous target was reached
    if (currentTime - state.lastActionTime >= state.nextActionDelay) {
        // Get the entity configuration to find its collision radius
        float collisionRadius = 0.4f; // Default collision radius for antagonist
        auto* config = entitiesManager.getConfiguration("antagonist");
        if (config) {
            collisionRadius = config->collisionRadius;
        }
        
        // Add extra safety margin to collision radius for path planning
        float safetyMargin = 0.4f;
        float pathPlanningRadius = collisionRadius + safetyMargin;
        
        // Get a random position near the entity on a whitelisted block type
        float newTargetX, newTargetY;
        if (manager.getRandomPositionNearOnBlockTypes(state.instanceName, newTargetX, newTargetY, 10.0f)) {
            // Set the new target
            state.targetX = newTargetX;
            state.targetY = newTargetY;              // Check if target position has element collisions using the expanded radius
            if (wouldCollideWithElement(state.targetX, state.targetY, pathPlanningRadius)) {
                
                // If collision detected, try to find a position nearby that doesn't have collisions
                float safeX = state.targetX;
                float safeY = state.targetY;
                  // Try to find a safe position using our enhanced collision detection radius
                if (!findSafePosition(safeX, safeY, pathPlanningRadius, gameMap)) {
                    // If we couldn't find a safe position, try again with a different random position
                    // and a smaller radius to search closer to the entity
                    bool foundSafePos = false;
                    
                    // Try three different random positions with different radii
                    for (float tryRadius = 8.0f; tryRadius >= 4.0f && !foundSafePos; tryRadius -= 2.0f) {
                        if (manager.getRandomPositionNearOnBlockTypes(state.instanceName, safeX, safeY, tryRadius)) {                            // Verify this new position is actually safe
                            if (!wouldCollideWithElement(safeX, safeY, pathPlanningRadius)) {
                                state.targetX = safeX;
                                state.targetY = safeY;
                                foundSafePos = true;
                                
                                std::cout << "Entity " << state.instanceName << " found safe position at reduced radius " 
                                          << tryRadius << std::endl;
                            }
                        }
                    }
                    
                    // If all attempts failed, don't move the entity this time
                    if (!foundSafePos) {
                        // Reset the action timer but with a shorter delay before next attempt
                        state.lastActionTime = currentTime;
                        state.nextActionDelay = manager.getRandomDelay(1.0f, 3.0f); // Shorter delay before trying again
                        
                        std::cout << "Entity " << state.instanceName << " couldn't find safe position, will try again in " 
                                  << state.nextActionDelay << " seconds" << std::endl;
                        return;
                    }
                } else {
                    // Use the adjusted safe position
                    state.targetX = safeX;
                    state.targetY = safeY;
                }
            }
              // Move the entity to the new target, interrupting any current movement
            manager.moveEntityTo(state.instanceName, state.targetX, state.targetY);
            
            // Set time for next action
            state.lastActionTime = currentTime;
            state.nextActionDelay = manager.getRandomDelay(2.0f, 7.0f);
            
            // Debug output
            std::cout << "Entity " << state.instanceName << " moving to new target: (" 
                      << state.targetX << ", " << state.targetY << ") - Next change in "
                      << state.nextActionDelay << " seconds" << std::endl;
        } else {
            // If we couldn't get a random position at all, try again with a shorter delay
            state.lastActionTime = currentTime;
            state.nextActionDelay = manager.getRandomDelay(1.0f, 3.0f);
            
            std::cout << "Entity " << state.instanceName << " couldn't find any valid position, will try again in " 
                      << state.nextActionDelay << " seconds" << std::endl;
        }
    }
    
    // Check if entity is stuck and hasn't moved for a while
    float currentX, currentY;
    if (manager.getEntityPosition(state.instanceName, currentX, currentY)) {
        // If entity has been in same position for too long and should be moving
        if (currentTime - state.lastPositionChangeTime > 3.0f && 
            !manager.hasEntityReachedTarget(state.instanceName)) {
            
            std::cout << "Entity " << state.instanceName << " appears to be stuck, attempting to find new path..." << std::endl;
            
            // Force an immediate action to pick a new path
            state.lastActionTime = 0.0f;
            state.nextActionDelay = 0.0f;
        }
        
        // Update the last position change time if the entity has moved
        if (std::abs(currentX - state.lastX) > 0.01f || std::abs(currentY - state.lastY) > 0.01f) {
            state.lastPositionChangeTime = currentTime;
            state.lastX = currentX;
            state.lastY = currentY;
        }
    }
    
    // We still check if entity has reached target for debug purposes only
    if (manager.hasEntityReachedTarget(state.instanceName)) {
        std::cout << "Entity " << state.instanceName << " reached target position." << std::endl;
    }
}
