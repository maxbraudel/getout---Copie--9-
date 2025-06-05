#ifndef TERRAIN_GENERATION_CONFIG_H
#define TERRAIN_GENERATION_CONFIG_H

#include <vector>
#include <string>
#include "enumDefinitions.h"
#include "elementsOnMap.h"

// Enum for different spawn types
enum class SpawnType {
    ELEMENT,  // Spawn decorative elements
    ENTITY,   // Spawn game entities (future use)
    BLOCK     // Replace/modify blocks (future use)
};

// Structure defining rules for spawning elements during terrain generation
struct GenerationRuleInfo {    // Basic spawn configuration
    SpawnType spawnType = SpawnType::ELEMENT;
    std::vector<ElementName> spawnElements;  // Elements to spawn with this rule (equiprobable if multiple)
    std::vector<EntityName> spawnEntities;   // Entities to spawn with this rule (equiprobable if multiple)
    std::vector<BlockName> spawnBlocks;      // Block types on which elements can spawn
    
    // Spawn probability and constraints
    int spawnChance = 50;                    // 1/spawnChance probability (e.g., 50 = 1/50 chance)
    int maxSpawns = 1000;                    // Maximum total spawns for this rule
    
    // Distance constraints
    float minDistanceFromSameRule = 4.0f;    // Minimum distance between spawns of this rule
    float maxDistanceFromBlocks = 3.0f;      // Maximum distance from specified blocks (0 = no constraint)
    std::vector<BlockName> proximityBlocks;  // Block types to check proximity to (empty = no constraint)
    
    // Group spawning
    bool spawnInGroup = false;               // Whether to spawn elements in groups
    float groupRadius = 2.0f;                // Radius for group spawning
    int groupNumberMin = 1;                  // Minimum elements in a group
    int groupNumberMax = 3;                  // Maximum elements in a group
    
    // Element properties
    float scaleMin = 0.7f;                   // Minimum scale multiplier
    float scaleMax = 1.0f;                   // Maximum scale multiplier
    float baseScale = 7.0f;                  // Base scale before random variation
    float rotation = 0.0f;                   // Rotation in degrees (0 = no rotation, -1 = random)
      // Sprite sheet properties
    int defaultSpriteSheetPhase = 0;         // Default sprite sheet phase
    int defaultSpriteSheetFrame = 0;         // Default sprite sheet frame
    bool randomDefaultSpriteSheetPhase = false;  // Whether to randomize sprite sheet phase for entities
    bool isAnimated = false;                 // Whether the element is animated
    float animationSpeed = 10.0f;            // Animation speed
    
    // Anchor and positioning
    AnchorPoint anchorPoint = AnchorPoint::USE_TEXTURE_DEFAULT;  // Anchor point
    float additionalXAnchorOffset = 0.0f;    // Additional X anchor offset
    float additionalYAnchorOffset = 0.0f;    // Additional Y anchor offset
      // Placement strategy
    bool randomPlacement = false;            // Whether to use randomized grid block selection instead of sequential
    
    // Rule identification
    std::string ruleName = "";               // Optional name for debugging
};

// Configuration class for terrain generation rules
class TerrainGenerationConfig {
public:
    TerrainGenerationConfig();
    ~TerrainGenerationConfig() = default;
    
    // Add a generation rule
    void addGenerationRule(const GenerationRuleInfo& rule);
    
    // Get all generation rules
    const std::vector<GenerationRuleInfo>& getGenerationRules() const;
    
    // Initialize default rules (coconut trees, etc.)
    void initializeDefaultRules();
    
    // Clear all rules
    void clearRules();
    
private:
    std::vector<GenerationRuleInfo> generationRules;
};

// Global configuration instance
extern TerrainGenerationConfig g_terrainConfig;

#endif // TERRAIN_GENERATION_CONFIG_H
