#include "entities.h"
#include "elementsOnMap.h"
#include "collision.h"
#include "map.h"
#include "player.h"
#include <iostream>
#include <vector>
#include <string>
#include <mutex>

// Function to apply damage from attacker to target entity
bool applyDamage(const std::string& attackerInstanceName, const std::string& targetInstanceName, 
                 EntitiesManager& entitiesManager) {
    // Get attacker entity
    Entity* attacker = entitiesManager.getEntity(attackerInstanceName);
    if (!attacker) {
        std::cout << "Warning: Attacker entity " << attackerInstanceName << " not found" << std::endl;
        return false;
    }
    
    // Get target entity
    Entity* target = entitiesManager.getEntity(targetInstanceName);
    if (!target) {
        std::cout << "Warning: Target entity " << targetInstanceName << " not found" << std::endl;
        return false;
    }
    
    // Apply damage
    int damageDealt = attacker->damagePoints;
    target->lifePoints -= damageDealt;
    
    std::cout << "Entity " << attackerInstanceName << " deals " << damageDealt 
              << " damage to " << targetInstanceName 
              << " (remaining life: " << target->lifePoints << ")" << std::endl;
    
    // Check if target should be destroyed
    if (target->lifePoints <= 0) {
        std::cout << "Entity " << targetInstanceName << " destroyed!" << std::endl;
        return true; // Indicates target should be destroyed
    }
    
    return false; // Target survives
}

// Function to destroy an entity completely
void destroyEntity(const std::string& instanceName, EntitiesManager& entitiesManager) {
    // Get the entity to get its element name
    Entity* entity = entitiesManager.getEntity(instanceName);
    if (!entity) {
        std::cout << "Warning: Cannot destroy entity " << instanceName << " - not found" << std::endl;
        return;
    }
    
    // Get the element name for this entity
    std::string elementName = EntitiesManager::getElementName(instanceName);
    
    std::cout << "Destroying entity " << instanceName << " and its element " << elementName << std::endl;
    
    // 1. Stop entity movement first to clean up pathfinding
    entitiesManager.stopEntityMovement(instanceName);
    
    // 2. Remove from HierarchicalEntityGrid (collision/spatial systems)
    extern HierarchicalEntityGrid g_hierarchicalEntityGrid;
    extern std::mutex g_hierarchicalEntityGridMutex;
    {
        std::lock_guard<std::mutex> lock(g_hierarchicalEntityGridMutex);
        g_hierarchicalEntityGrid.removeEntity(instanceName);
    }
    
    // 3. Reset entity spatial grid to remove references
    extern void resetEntitySpatialGrid();
    resetEntitySpatialGrid();
    
    // 4. Remove the element from the map
    extern ElementsOnMap elementsManager;
    elementsManager.removeElement(elementName);
    
    // 5. Finally, remove the entity from the entities manager
    entitiesManager.getEntities().erase(instanceName);
}

// Function to check and handle entities that should be destroyed
void processEntityDestructions(EntitiesManager& entitiesManager) {
    std::vector<std::string> entitiesToDestroy;
      // Get all entities and check their life points
    auto& entities = entitiesManager.getEntities();
    for (const auto& pair : entities) {
        const Entity& entity = pair.second;
        if (entity.lifePoints <= 0) {
            entitiesToDestroy.push_back(entity.instanceName);
        }
    }
    
    // Destroy entities with 0 or negative life points
    for (const std::string& instanceName : entitiesToDestroy) {
        destroyEntity(instanceName, entitiesManager);
    }
}

// Function to handle damage application during attack behavior
// This should be called when an entity reaches its target during attack state
void handleAttackDamage(const std::string& attackerInstanceName, const std::string& targetInstanceName,
                       EntitiesManager& entitiesManager) {
    // Apply damage to target
    bool targetDestroyed = applyDamage(attackerInstanceName, targetInstanceName, entitiesManager);
    
    // If target was destroyed, handle the destruction immediately
    if (targetDestroyed) {
        destroyEntity(targetInstanceName, entitiesManager);
    }
}

// Function to get the block name underneath an entity's anchor point
BlockName giveBlockNameUnderneathEntity(const std::string& instanceName, EntitiesManager& entitiesManager) {
    // Get the entity
    Entity* entity = entitiesManager.getEntity(instanceName);
    if (!entity) {
        std::cout << "Warning: Entity " << instanceName << " not found" << std::endl;
        return BlockName::GRASS_0; // Default return value
    }
    
    // Get entity position
    float entityX, entityY;
    std::string elementName = EntitiesManager::getElementName(instanceName);
    extern ElementsOnMap elementsManager;
    if (!elementsManager.getElementPosition(elementName, entityX, entityY)) {
        std::cout << "Warning: Could not get position for entity " << instanceName << std::endl;
        return BlockName::GRASS_0; // Default return value
    }
    
    // Convert entity position to grid coordinates (entity anchor point)
    int gridX = static_cast<int>(std::floor(entityX));
    int gridY = static_cast<int>(std::floor(entityY));
    
    // Get the block at the entity's anchor point
    extern Map gameMap;
    BlockName blockType = gameMap.getBlockNameByCoordinates(gridX, gridY);
    
    return blockType;
}

// Function to check and apply water damage to the player
void checkAndApplyWaterDamageToPlayer(EntitiesManager& entitiesManager) {
    const std::string playerInstanceName = "player1";
    
    // Get the block underneath the player
    BlockName blockUnderPlayer = giveBlockNameUnderneathEntity(playerInstanceName, entitiesManager);
    
    // Check if it's a water block
    if (blockUnderPlayer == BlockName::WATER_0 || 
        blockUnderPlayer == BlockName::WATER_1 || 
        blockUnderPlayer == BlockName::WATER_2 || 
        blockUnderPlayer == BlockName::WATER_3 || 
        blockUnderPlayer == BlockName::WATER_4) {
        
        // Get player entity
        Entity* player = entitiesManager.getEntity(playerInstanceName);
        if (player) {
            // Apply 1000 damage (enough to kill the player)
            player->lifePoints -= 1000;
            
            std::cout << "Player stepped on water block (" << static_cast<int>(blockUnderPlayer) 
                      << ") and took 1000 damage! Remaining life: " << player->lifePoints << std::endl;
            
            // Check if player should be destroyed
            if (player->lifePoints <= 0) {
                std::cout << "Player destroyed by water damage!" << std::endl;
                destroyEntity(playerInstanceName, entitiesManager);
            }
        }
    }
}

// Function to check if a player is on a specific block position and apply water damage if needed
void checkPlayerWaterDamageAtPosition(int blockX, int blockY, BlockName blockType, EntitiesManager& entitiesManager) {
    const std::string playerInstanceName = "player1";
    
    // Only check if the new block is a water block
    if (blockType != BlockName::WATER_0 && 
        blockType != BlockName::WATER_1 && 
        blockType != BlockName::WATER_2 && 
        blockType != BlockName::WATER_3 && 
        blockType != BlockName::WATER_4) {
        return; // Not a water block, no damage needed
    }
    
    // Get player position
    float playerX, playerY;
    if (!getPlayerPosition(playerX, playerY)) {
        return; // Player doesn't exist
    }
    
    // Convert player position to grid coordinates
    int playerGridX = static_cast<int>(std::floor(playerX));
    int playerGridY = static_cast<int>(std::floor(playerY));
    
    // Check if player is on the same grid position as the water block
    if (playerGridX == blockX && playerGridY == blockY) {
        // Player is on the water block, apply damage
        Entity* player = entitiesManager.getEntity(playerInstanceName);
        if (player) {
            player->lifePoints -= 1000;
            
            std::cout << "Water block (" << static_cast<int>(blockType) 
                      << ") placed under player! Player took 1000 damage! Remaining life: " << player->lifePoints << std::endl;
            
            // Check if player should be destroyed
            if (player->lifePoints <= 0) {
                std::cout << "Player destroyed by water damage from placed block!" << std::endl;
                destroyEntity(playerInstanceName, entitiesManager);
            }
        }
    }
}
