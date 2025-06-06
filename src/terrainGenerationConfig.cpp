#include "terrainGenerationConfig.h"

// Global configuration instance
TerrainGenerationConfig g_terrainConfig;

// Constructor
TerrainGenerationConfig::TerrainGenerationConfig() {
    // Initialize with default rules
    initializeDefaultRules();
}

// Add a generation rule
void TerrainGenerationConfig::addGenerationRule(const GenerationRuleInfo& rule) {
    generationRules.push_back(rule);
}

// Get all generation rules
const std::vector<GenerationRuleInfo>& TerrainGenerationConfig::getGenerationRules() const {
    return generationRules;
}

// Initialize default rules (coconut trees, etc.)
void TerrainGenerationConfig::initializeDefaultRules() {
    // Clear any existing rules
    clearRules();
    
    // Coconut tree rule - based on the existing logic in placeTerrainElements
    GenerationRuleInfo coconutTreeRule;
    coconutTreeRule.ruleName = "CoconutTrees";
    coconutTreeRule.spawnType = SpawnType::ELEMENT;
    
    // Add all three coconut tree variants with equal probability
    coconutTreeRule.spawnElements = {
        ElementName::COCONUT_TREE_1,
        ElementName::COCONUT_TREE_2,
        ElementName::COCONUT_TREE_3
    };
    
    // Spawn on sand blocks only
    coconutTreeRule.spawnBlocks = {BlockName::SAND};
    
    // Spawn probability and constraints (matching existing logic)
    coconutTreeRule.spawnChance = 50;                    // 1/50 chance
    coconutTreeRule.maxSpawns = 1000;                    // Max 1000 trees
    
    // Distance constraints (matching existing MIN_COCONUT_TREE_DISTANCE and MAX_WATER_DISTANCE)
    coconutTreeRule.minDistanceFromSameRule = 4.0f;     // MIN_COCONUT_TREE_DISTANCE
    coconutTreeRule.maxDistanceFromBlocks = 3.0f;       // MAX_WATER_DISTANCE
    coconutTreeRule.proximityBlocks = {                 // Must be near water
        BlockName::WATER_0, BlockName::WATER_1, BlockName::WATER_2, 
        BlockName::WATER_3, BlockName::WATER_4
    };
      // No group spawning for coconut trees
    coconutTreeRule.spawnInGroup = false;
    
    // Enable random placement for more organic tree distribution
    coconutTreeRule.randomPlacement = true;
    
    // Element properties (matching existing logic)
    coconutTreeRule.scaleMin = 0.7f;                     // Random scale variation
    coconutTreeRule.scaleMax = 1.0f;
    coconutTreeRule.baseScale = 7.0f;                    // Base scale
    coconutTreeRule.rotation = 0.0f;                     // No rotation
    
    // Sprite sheet properties (matching existing logic)
    coconutTreeRule.defaultSpriteSheetPhase = 0;
    coconutTreeRule.defaultSpriteSheetFrame = 0;
    coconutTreeRule.isAnimated = false;
    coconutTreeRule.animationSpeed = 10.0f;
    
    // Anchor and positioning (matching existing logic)
    coconutTreeRule.anchorPoint = AnchorPoint::USE_TEXTURE_DEFAULT;
    coconutTreeRule.additionalXAnchorOffset = 0.0f;
    coconutTreeRule.additionalYAnchorOffset = 0.0f;
      // Add the rule to the configuration
    addGenerationRule(coconutTreeRule);


    GenerationRuleInfo coconut;
    coconut.ruleName = "coconut";
    coconut.spawnType = SpawnType::ELEMENT;
    
    // Add all three coconut tree variants with equal probability
    coconut.spawnElements = {
        ElementName::COCONUT
    };
    
    // Spawn on sand blocks only
    coconut.spawnBlocks = {BlockName::SAND};
    
    // Spawn probability and constraints (matching existing logic)
    coconut.spawnChance = 1;                    // 1/50 chance
    coconut.maxSpawns = 3;                    // Max 1000 trees
    
    // Distance constraints (matching existing MIN_COCONUT_TREE_DISTANCE and MAX_WATER_DISTANCE)
    coconut.minDistanceFromSameRule = 10.0f;     // MIN_COCONUT_TREE_DISTANCE
    coconut.maxDistanceFromBlocks = 4.0f;       // MAX_WATER_DISTANCE
    coconut.proximityBlocks = {                 // Must be near water
        BlockName::WATER_0, BlockName::WATER_1, BlockName::WATER_2, 
        BlockName::WATER_3, BlockName::WATER_4
    };
      // No group spawning for coconut trees
    coconut.spawnInGroup = false;
    
    // Enable random placement for more organic tree distribution
    coconut.randomPlacement = true;
    
    // Element properties (matching existing logic)
    coconut.scaleMin = 1.0f;                     // Random scale variation
    coconut.scaleMax = 1.3f;
    coconut.baseScale = 2.0f;                    // Base scale
    coconut.rotation = 0.0f;                     // No rotation
    
    // Sprite sheet properties (matching existing logic)
    coconut.defaultSpriteSheetPhase = 0;
    coconut.defaultSpriteSheetFrame = 0;
    coconut.isAnimated = false;
    coconut.animationSpeed = 10.0f;
    
    // Anchor and positioning (matching existing logic)
    coconut.anchorPoint = AnchorPoint::USE_TEXTURE_DEFAULT;
    coconut.additionalXAnchorOffset = 0.0f;
    coconut.additionalYAnchorOffset = 0.0f;
      // Add the rule to the configuration
    addGenerationRule(coconut);


    
    // Antagonist entity rule - spawn antagonist entities in groups on GRASS_2 blocks
    GenerationRuleInfo antagonistRule;
    antagonistRule.ruleName = "AntagonistEntities";
    antagonistRule.spawnType = SpawnType::ENTITY;
    
    // Add antagonist entity types with equal probability
    antagonistRule.spawnEntities = {
        EntityName::ANTAGONIST,
    };
    
    // Spawn on GRASS_2 blocks only
    antagonistRule.spawnBlocks = {BlockName::GRASS_2};      // Spawn probability and constraints
    antagonistRule.spawnChance = 100;                   // 1/100 chance for much less frequent spawning
    antagonistRule.maxSpawns = 50;                      // Max 50 antagonist entities total
    
    // Distance constraints - maintain reasonable spacing between groups
    antagonistRule.minDistanceFromSameRule = 8.0f;      // Min distance between antagonist groups
    antagonistRule.maxDistanceFromBlocks = 0.0f;        // No proximity requirement
    antagonistRule.proximityBlocks = {};                // No specific blocks needed nearby
      // Group spawning configuration - spawn antagonists in small groups
    antagonistRule.spawnInGroup = true;
    antagonistRule.groupRadius = 3.0f;                  // Spread group members within 3 units
    antagonistRule.groupNumberMin = 2;                  // Min 2 entities per group
    antagonistRule.groupNumberMax = 4;                  // Max 4 entities per group
      // Enable random placement for more organic entity distribution
    antagonistRule.randomPlacement = true;
    
    // Sprite sheet properties for entities
    antagonistRule.randomDefaultSpriteSheetPhase = true;  // Randomize sprite phase for variety
    
    // Entity properties (not used for entities, but kept for consistency)
    antagonistRule.scaleMin = 1.0f;
    antagonistRule.scaleMax = 1.0f;
    antagonistRule.baseScale = 1.0f;
    antagonistRule.rotation = 0.0f;
    
    // Add the antagonist rule to the configuration
    addGenerationRule(antagonistRule);

    // Shark entity rule - spawn shark entities in water
    GenerationRuleInfo sharksRule;
    sharksRule.ruleName = "SharkEntities";
    sharksRule.spawnType = SpawnType::ENTITY;
    
    // Add shark entity types with equal probability
    sharksRule.spawnEntities = {
        EntityName::SHARK,
    };
    
    // Spawn on deep water blocks only
    sharksRule.spawnBlocks = {BlockName::WATER_4};
      // Spawn probability and constraints
    sharksRule.spawnChance = 1000;                   // 1/100 chance for rare spawning
    sharksRule.maxSpawns = 50;                      // Max 50 shark entities total
    
    // Distance constraints - maintain reasonable spacing between groups
    sharksRule.minDistanceFromSameRule = 8.0f;      // Min distance between shark groups
    sharksRule.maxDistanceFromBlocks = 0.0f;        // No proximity requirement
    sharksRule.proximityBlocks = {};                // No specific blocks needed nearby
    
    // Group spawning configuration - spawn sharks in small groups
    sharksRule.spawnInGroup = false;
      // Enable random placement for more organic shark distribution
    sharksRule.randomPlacement = true;
    
    // Sprite sheet properties for entities
    sharksRule.randomDefaultSpriteSheetPhase = true;  // Randomize sprite phase for variety
    
    // Entity properties (not used for entities, but kept for consistency)
    sharksRule.scaleMin = 1.0f;
    sharksRule.scaleMax = 1.0f;
    sharksRule.baseScale = 1.0f;
    sharksRule.rotation = 0.0f;
    
    // Add the shark rule to the configuration
    addGenerationRule(sharksRule);




    // Spawn on deep water blocks only
    GenerationRuleInfo giraffesRule;
    giraffesRule.ruleName = "GiraffeEntities";
    giraffesRule.spawnType = SpawnType::ENTITY;
    
    // Add shark entity types with equal probability
    giraffesRule.spawnEntities = {
        EntityName::GIRAFFE,
    };

    giraffesRule.spawnBlocks = {BlockName::GRASS_2};
      // Spawn probability and constraints
    giraffesRule.spawnChance = 1000;                   // 1/100 chance for rare spawning
    giraffesRule.maxSpawns = 1000;                      // Max 50 shark entities total
    
    // Distance constraints - maintain reasonable spacing between groups
    giraffesRule.minDistanceFromSameRule = 8.0f;      // Min distance between shark groups
    giraffesRule.maxDistanceFromBlocks = 0.0f;        // No proximity requirement
    giraffesRule.proximityBlocks = {};                // No specific blocks needed nearby
    
    // Group spawning configuration - spawn sharks in small groups
    giraffesRule.spawnInGroup = false;
      // Enable random placement for more organic shark distribution
    giraffesRule.randomPlacement = true;
    
    // Sprite sheet properties for entities
    giraffesRule.randomDefaultSpriteSheetPhase = true;  // Randomize sprite phase for variety
    
    // Entity properties (not used for entities, but kept for consistency)
    giraffesRule.scaleMin = 1.0f;
    giraffesRule.scaleMax = 1.0f;
    giraffesRule.baseScale = 1.0f;
    giraffesRule.rotation = 0.0f;
    
    // Add the shark rule to the configuration
    addGenerationRule(giraffesRule);
}

// Clear all rules
void TerrainGenerationConfig::clearRules() {
    generationRules.clear();
}
