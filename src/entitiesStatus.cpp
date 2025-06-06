#include "entities.h"
#include "elementsOnMap.h"
#include "collision.h"
#include "map.h"
#include "player.h"
#include "gameMenus.h"
#include "globals.h"
#include "enumDefinitions.h"
#include "PlayerMovementManager.h"
#include "threading.h"
#include <iostream>
#include <vector>
#include <string>
#include <mutex>

// Function to update health bar when player health changes
void updatePlayerHealthBar(EntitiesManager& entitiesManager) {
    const std::string playerInstanceName = "player1";
    
    // Get player entity
    Entity* player = entitiesManager.getEntity(playerInstanceName);
    if (player) {
        // Update health bar with current health
        extern GameMenus gameMenus;
        gameMenus.updateHealthBar(player->lifePoints);
    }
}

// Function to remove life points from an entity and handle health bar updates
bool removeLifePointsFromEntity(const std::string& instanceName, int lifePointsToRemove, EntitiesManager& entitiesManager) {
    // Get the entity
    Entity* entity = entitiesManager.getEntity(instanceName);
    if (!entity) {
        std::cout << "Warning: Entity " << instanceName << " not found for life point removal" << std::endl;
        return false;
    }
    
    // Remove life points
    entity->lifePoints -= lifePointsToRemove;
    
    std::cout << "Entity " << instanceName << " lost " << lifePointsToRemove 
              << " life points! Remaining life: " << entity->lifePoints << std::endl;
    
    // Update health bar if this is the player
    if (instanceName == "player1") {
        updatePlayerHealthBar(entitiesManager);
    }
    
    // Return true if entity should be destroyed (0 or negative life points)
    return entity->lifePoints <= 0;
}

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
    bool shouldDestroy = removeLifePointsFromEntity(targetInstanceName, damageDealt, entitiesManager);
    
    std::cout << "Entity " << attackerInstanceName << " deals " << damageDealt 
              << " damage to " << targetInstanceName << std::endl;
    
    // Check if target should be destroyed
    if (shouldDestroy) {
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
      // 6. Check if this was the player - if so, trigger defeat condition
    if (instanceName == "player1") {
        std::cout << "PLAYER DESTROYED! Triggering defeat condition..." << std::endl;
        
        // Access the player movement manager to trigger defeat condition
        extern PlayerMovementManager* g_playerMovementManager;
        if (g_playerMovementManager != nullptr) {
            // Use the proper method to trigger defeat condition
            g_playerMovementManager->triggerDefeatCondition();
        } else {
            // Fallback: set flags directly if player movement manager is not available
            extern bool SHOULD_SHOW_GAME_OVER;
            extern GameState GAME_STATE;
            
            GAME_STATE = GameState::DEFEAT;
            SHOULD_SHOW_GAME_OVER = true;
            std::cout << "Player movement manager not available - using fallback defeat trigger" << std::endl;
            
            // Force pause the game immediately
            extern GameThreadManager* g_threadManager;
            if (g_threadManager) {
                g_threadManager->pauseGame();
                std::cout << "Game forcibly paused for defeat condition (fallback)" << std::endl;
            }
        }
    }
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

// Function to check and apply damage blocks to any entity based on its configuration
void checkAndApplyDamageBlocksToEntity(const std::string& instanceName, EntitiesManager& entitiesManager) {
    // Get the entity
    Entity* entity = entitiesManager.getEntity(instanceName);
    if (!entity) {
        return; // Entity doesn't exist
    }
    
    // Get the entity configuration to access damage blocks
    const EntityConfiguration* config = entitiesManager.getConfiguration(entity->type);
    if (!config || config->damageBlocks.empty()) {
        return; // No configuration or no damage blocks defined
    }
    
    // Get the block underneath the entity
    BlockName blockUnderEntity = giveBlockNameUnderneathEntity(instanceName, entitiesManager);
      // Check if the block under the entity is in the damage blocks list
    for (const BlockName& damageBlock : config->damageBlocks) {
        if (blockUnderEntity == damageBlock) {
            // Apply 1000 damage (enough to kill most entities)
            bool shouldDestroy = removeLifePointsFromEntity(instanceName, 1000, entitiesManager);
            
            std::cout << "Entity " << instanceName << " stepped on damage block (" 
                      << static_cast<int>(blockUnderEntity) << ") and took 1000 damage!" << std::endl;
            
            // Check if entity should be destroyed
            if (shouldDestroy) {
                std::cout << "Entity " << instanceName << " destroyed by damage block!" << std::endl;
                destroyEntity(instanceName, entitiesManager);
            }
            
            // Only apply damage once per check, even if multiple damage blocks match
            break;
        }
    }
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
        
        // Apply 1000 damage (enough to kill the player)
        bool shouldDestroy = removeLifePointsFromEntity(playerInstanceName, 1000, entitiesManager);
        
        std::cout << "Player stepped on water block (" << static_cast<int>(blockUnderPlayer)
                  << ") and took 1000 damage!" << std::endl;
        
        // Check if player should be destroyed
        if (shouldDestroy) {
            std::cout << "Player destroyed by water damage!" << std::endl;
            destroyEntity(playerInstanceName, entitiesManager);
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
        bool shouldDestroy = removeLifePointsFromEntity(playerInstanceName, 1000, entitiesManager);
        
        std::cout << "Water block (" << static_cast<int>(blockType) 
                  << ") placed under player! Player took 1000 damage!" << std::endl;
        
        // Check if player should be destroyed
        if (shouldDestroy) {
            std::cout << "Player destroyed by water damage from placed block!" << std::endl;
            destroyEntity(playerInstanceName, entitiesManager);
        }
    }
}

// Function to check all entities at a specific position and apply damage if damage blocks are placed
void checkAllEntitiesDamageAtPosition(int blockX, int blockY, BlockName blockType, EntitiesManager& entitiesManager) {
    // Get all entities and check if any are at the same position as the placed block
    auto& entities = entitiesManager.getEntities();
    
    for (const auto& pair : entities) {
        const std::string& instanceName = pair.first;
        const Entity& entity = pair.second;
        
        // Get entity configuration to check if this block type is a damage block for this entity
        const EntityConfiguration* config = entitiesManager.getConfiguration(entity.type);
        if (!config || config->damageBlocks.empty()) {
            continue; // No damage blocks configured for this entity
        }
        
        // Check if the placed block is in this entity's damage blocks list
        bool isDamageBlock = false;
        for (const BlockName& damageBlock : config->damageBlocks) {
            if (blockType == damageBlock) {
                isDamageBlock = true;
                break;
            }
        }
        
        if (!isDamageBlock) {
            continue; // This block type doesn't damage this entity
        }
        
        // Get entity position
        float entityX, entityY;
        std::string elementName = EntitiesManager::getElementName(instanceName);
        extern ElementsOnMap elementsManager;
        if (!elementsManager.getElementPosition(elementName, entityX, entityY)) {
            continue; // Could not get entity position
        }
        
        // Convert entity position to grid coordinates
        int entityGridX = static_cast<int>(std::floor(entityX));
        int entityGridY = static_cast<int>(std::floor(entityY));
          // Check if entity is on the same grid position as the placed block
        if (entityGridX == blockX && entityGridY == blockY) {
            // Entity is on the damage block, apply damage
            bool shouldDestroy = removeLifePointsFromEntity(instanceName, 1000, entitiesManager);
            
            std::cout << "Entity " << instanceName << " was standing on position where damage block (" 
                      << static_cast<int>(blockType) << ") was placed! Entity took 1000 damage!" << std::endl;
            
            // Check if entity should be destroyed
            if (shouldDestroy) {
                std::cout << "Entity " << instanceName << " destroyed by damage block placement!" << std::endl;
                destroyEntity(instanceName, entitiesManager);
            }        }
    }
}
