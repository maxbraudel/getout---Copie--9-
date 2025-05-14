#ifndef TERRAIN_GENERATION_H
#define TERRAIN_GENERATION_H

#include <map>
#include <vector>
#include "map.h" // For TextureName and std::pair<int, int>

// Generates a terrain map using a placeholder Perlin noise function.
// A real Perlin noise implementation/library should be used for better results.
std::map<std::pair<int, int>, TextureName> generateTerrain(
    int gridWidth, 
    int gridHeight, 
    float islandFeatureSize, // Renamed from scale for clarity
    float seaFeatureSize,    // Added: Controls the size of sea areas
    float waterThreshold,
    float grassThreshold // Added: Values above this (and waterThreshold) become grass
);

#endif // TERRAIN_GENERATION_H
