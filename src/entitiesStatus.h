#pragma once

#include "entities.h"
#include "enumDefinitions.h"
#include <string>

// Forward declaration
class EntitiesManager;

// Function to apply damage from attacker to target entity
// Returns true if target should be destroyed, false otherwise
bool applyDamage(const std::string& attackerInstanceName, const std::string& targetInstanceName, 
                 EntitiesManager& entitiesManager);

// Function to destroy an entity completely (removes from map and entities manager)
void destroyEntity(const std::string& instanceName, EntitiesManager& entitiesManager);

// Function to check and handle entities that should be destroyed
void processEntityDestructions(EntitiesManager& entitiesManager);

// Function to handle damage application during attack behavior
// This should be called when an entity reaches its target during attack state
void handleAttackDamage(const std::string& attackerInstanceName, const std::string& targetInstanceName,
                       EntitiesManager& entitiesManager);

// Function to get the block name underneath an entity's anchor point
BlockName giveBlockNameUnderneathEntity(const std::string& instanceName, EntitiesManager& entitiesManager);

// Function to check and apply water damage to the player
void checkAndApplyWaterDamageToPlayer(EntitiesManager& entitiesManager);

// Function to check and apply damage blocks to any entity based on its configuration
void checkAndApplyDamageBlocksToEntity(const std::string& instanceName, EntitiesManager& entitiesManager);

// Function to check if a player is on a specific block position and apply water damage if needed
void checkPlayerWaterDamageAtPosition(int blockX, int blockY, BlockName blockType, EntitiesManager& entitiesManager);

// Function to check all entities at a specific position and apply damage if damage blocks are placed
void checkAllEntitiesDamageAtPosition(int blockX, int blockY, BlockName blockType, EntitiesManager& entitiesManager);

// Function to update health bar when player health changes
void updatePlayerHealthBar(EntitiesManager& entitiesManager);

// Function to remove life points from an entity and handle health bar updates
bool removeLifePointsFromEntity(const std::string& instanceName, int lifePointsToRemove, EntitiesManager& entitiesManager);
