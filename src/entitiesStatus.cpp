#include "entities.h"
#include "elementsOnMap.h"
#include <iostream>
#include <vector>
#include <string>

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
    
    // Remove the element from the map
    extern ElementsOnMap elementsManager;
    elementsManager.removeElement(elementName);
      // Remove the entity from the entities manager
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
