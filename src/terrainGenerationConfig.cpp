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
}

// Clear all rules
void TerrainGenerationConfig::clearRules() {
    generationRules.clear();
}
