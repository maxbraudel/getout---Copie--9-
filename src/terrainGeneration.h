#ifndef TERRAIN_GENERATION_H
#define TERRAIN_GENERATION_H

#include <map>
#include <vector>
#include "map.h" // For BlockName and std::pair<int, int>
#include "elementsOnMap.h" // For ElementsOnMap class
#include "enumDefinitions.h"

// Forward declaration for configuration structure
struct GenerationRuleInfo;


// Generates a terrain map using a placeholder Perlin noise function.
// A real Perlin noise implementation/library should be used for better results.
std::map<std::pair<int, int>, BlockName> generateTerrain(
    int gridWidth, 
    int gridHeight, 
    float islandFeatureSize, // Renamed from scale for clarity
    float seaFeatureSize,    // Added: Controls the size of sea areas
    float waterThreshold,
    float grassThreshold // Added: Values above this (and waterThreshold) become grass
);

// Places decorative elements (bushes, etc.) on appropriate terrain blocks
// (bushes on sand, etc.) with some probability
// COORDINATE SYSTEM: Uses the game's standard coordinate system where (0,0) is at bottom-left
// and Y increases upward. Elements are placed at the center of blocks at coordinates (x+0.5, y+0.5)
void placeTerrainElements(
    ElementsOnMap& elementsManager,
    const Map& map,
    int gridWidth,
    int gridHeight
);

// Helper function to place elements based on a single generation rule
void placeElementsFromRule(
    ElementsOnMap& elementsManager,
    const Map& map,
    int gridWidth,
    int gridHeight,
    const GenerationRuleInfo& rule
);

// Helper function to place entities based on a single generation rule
void placeEntitiesFromRule(
    const Map& map,
    int gridWidth,
    int gridHeight,
    const GenerationRuleInfo& rule
);

#endif // TERRAIN_GENERATION_H
