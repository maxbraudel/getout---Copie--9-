#include "terrainGeneration.h"
#include "terrainGenerationConfig.h" // For configuration system
#include "map.h" // For BlockName enum and gameMap
#include "collision.h" // For collision detection
#include "entities.h" // For global entitiesManager
#include "globals.h" // For DEBUG_MAP flag
#include <vector>
#include <queue>
#include <map>
#include <utility> // For std::pair
#include <limits>  // For std::numeric_limits
#include <cmath>   // For fmod, sqrt, etc.
#include <cstdlib> // For rand, srand
#include <ctime>   // For time
#include <algorithm> // For std::shuffle
#include <random>  // For std::random_device, std::mt19937
#include <iostream> // For debugging output
#include "enumDefinitions.h"


// Static variables for noise generation (assuming these are part of your existing setup)
thread_local static std::vector<std::vector<float>> baseNoiseGrid;
thread_local static int baseNoiseWidth_static = 0; // Renamed to avoid conflict if baseNoiseWidth is a param
thread_local static int baseNoiseHeight_static = 0; // Renamed
thread_local static bool baseNoiseInitialized = false;

// Function to initialize the base noise grid (assuming this exists)
void initializeBaseNoiseIfNeeded(int gridWidth, int gridHeight, float featureSizeFactor) {
    if (baseNoiseInitialized && baseNoiseWidth_static == static_cast<int>(gridWidth / featureSizeFactor) && baseNoiseHeight_static == static_cast<int>(gridHeight / featureSizeFactor)) {
        return;
    }

    baseNoiseWidth_static = static_cast<int>(gridWidth / featureSizeFactor);
    if (baseNoiseWidth_static == 0) baseNoiseWidth_static = 1; // Ensure at least 1
    baseNoiseHeight_static = static_cast<int>(gridHeight / featureSizeFactor);
    if (baseNoiseHeight_static == 0) baseNoiseHeight_static = 1; // Ensure at least 1
    
    baseNoiseGrid.assign(baseNoiseHeight_static, std::vector<float>(baseNoiseWidth_static));
    // srand(static_cast<unsigned int>(time(0))); // Seed should be controlled from main
    for (int y = 0; y < baseNoiseHeight_static; ++y) {
        for (int x = 0; x < baseNoiseWidth_static; ++x) {
            baseNoiseGrid[y][x] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        }
    }
    baseNoiseInitialized = true;
}

// Function for bilinear interpolation (assuming this exists)
float bilinearInterpolate(float x00, float x10, float x01, float x11, float tx, float ty) {
    float u = 1.0f - tx;
    float v = 1.0f - ty;
    return u * v * x00 + tx * v * x10 + u * ty * x01 + tx * ty * x11;
}

// Function to get interpolated noise (assuming this exists)
float getInterpolatedNoise(float normX, float normY) {
    if (!baseNoiseInitialized || baseNoiseWidth_static == 0 || baseNoiseHeight_static == 0) {
        return 0.5f; // Default if not initialized
    }

    float x = normX * (baseNoiseWidth_static -1); // map to base grid coords
    float y = normY * (baseNoiseHeight_static -1);

    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // Clamp to grid boundaries
    x0 = std::max(0, std::min(x0, baseNoiseWidth_static - 1));
    y0 = std::max(0, std::min(y0, baseNoiseHeight_static - 1));
    x1 = std::max(0, std::min(x1, baseNoiseWidth_static - 1));
    y1 = std::max(0, std::min(y1, baseNoiseHeight_static - 1));

    float tx = x - static_cast<float>(x0);
    float ty = y - static_cast<float>(y0);

    return bilinearInterpolate(
        baseNoiseGrid[y0][x0], baseNoiseGrid[y0][x1],
        baseNoiseGrid[y1][x0], baseNoiseGrid[y1][x1],
        tx, ty
    );
}


std::map<std::pair<int, int>, BlockName> generateTerrain(
    int gridWidth, int gridHeight, 
    float islandFeatureSize, // Renamed from scale
    float seaFeatureSize,    // Added this line
    float waterThreshold, float grassThreshold) {

    // Check if we should generate a debug map instead of the regular terrain
    if (DEBUG_MAP) {
        std::cout << "Generating DEBUG MAP - Top half: GRASS_2, Bottom half: WATER_4" << std::endl;
        
        std::map<std::pair<int, int>, BlockName> debugMap;
        int midPoint = gridHeight / 2;
        
        // Fill the grid with the debug pattern
        for (int y_coord = 0; y_coord < gridHeight; ++y_coord) {
            for (int x_coord = 0; x_coord < gridWidth; ++x_coord) {
                if (y_coord >= midPoint) {
                    // Top half - GRASS_2
                    debugMap[{x_coord, y_coord}] = BlockName::WATER_4;
                } else {
                    // Bottom half - WATER_4
                    debugMap[{x_coord, y_coord}] = BlockName::GRASS_2;
                }
            }
        }
        
        return debugMap;
    }

    // Regular terrain generation (only executed if DEBUG_MAP is false)
    // Adjust feature size for noise generation based on seaFeatureSize
    // A larger seaFeatureSize means smaller features in the noise map, leading to larger continuous areas (sea)
    float noiseFeatureSize = islandFeatureSize / seaFeatureSize; 
    initializeBaseNoiseIfNeeded(gridWidth, gridHeight, noiseFeatureSize);

    std::vector<std::vector<BlockName>> grid(gridHeight, std::vector<BlockName>(gridWidth));

    // 1. Initial terrain generation based on noise
    for (int y_coord = 0; y_coord < gridHeight; ++y_coord) {
        for (int x_coord = 0; x_coord < gridWidth; ++x_coord) {
            float noiseValue = getInterpolatedNoise(
                static_cast<float>(x_coord) / gridWidth, 
                static_cast<float>(y_coord) / gridHeight
            );

            if (noiseValue < waterThreshold) {
                grid[y_coord][x_coord] = BlockName::WATER_0; // Initial water type (will be refined)
            } else if (noiseValue < grassThreshold) {
                grid[y_coord][x_coord] = BlockName::SAND;
            } else {
                grid[y_coord][x_coord] = BlockName::GRASS_0;
            }
        }
    }

    // 2. BFS to calculate distances to sand
    std::vector<std::vector<int>> distanceToSand(gridHeight, std::vector<int>(gridWidth, std::numeric_limits<int>::max()));
    std::queue<std::pair<int, int>> bfsQueue;

    for (int y_coord = 0; y_coord < gridHeight; ++y_coord) {
        for (int x_coord = 0; x_coord < gridWidth; ++x_coord) {
            if (grid[y_coord][x_coord] == BlockName::SAND) {
                distanceToSand[y_coord][x_coord] = 0;
                bfsQueue.push({x_coord, y_coord});
            }
        }
    }

    int dx[] = {0, 0, 1, -1}; // For 4-directional neighbors
    int dy[] = {1, -1, 0, 0};

    while (!bfsQueue.empty()) {
        std::pair<int, int> curr = bfsQueue.front();
        bfsQueue.pop();
        int cx = curr.first;
        int cy = curr.second;

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (nx >= 0 && nx < gridWidth && ny >= 0 && ny < gridHeight) {
                if (distanceToSand[ny][nx] == std::numeric_limits<int>::max()) { // If not sand and not visited
                    distanceToSand[ny][nx] = distanceToSand[cy][cx] + 1;
                    bfsQueue.push({nx, ny});
                }
            }
        }
    }
    
    // 3. Assign final water textures based on distance
    for (int y_coord = 0; y_coord < gridHeight; ++y_coord) {
        for (int x_coord = 0; x_coord < gridWidth; ++x_coord) {
            // Only modify if it was initially classified as a water block
            if (grid[y_coord][x_coord] != BlockName::SAND && grid[y_coord][x_coord] != BlockName::GRASS_0) {
                int dist = distanceToSand[y_coord][x_coord];
                
                if (dist == 1) {
                    grid[y_coord][x_coord] = BlockName::WATER_0;
                } else if (dist == 2) {
                    grid[y_coord][x_coord] = BlockName::WATER_1;
                } else if (dist == 3) {
                    grid[y_coord][x_coord] = BlockName::WATER_2;
                } else if (dist == 4) {
                    grid[y_coord][x_coord] = BlockName::WATER_3;
                } else if (dist >= 5) {
                    grid[y_coord][x_coord] = BlockName::WATER_4;
                }
                // If dist is 0, it's sand, so this block is skipped by the outer if.
                // If it was WATER_0 and dist is still max_int (e.g. isolated pond), it becomes WATER_5.
            }
        }
    }

    // 3b. Assign final grass textures based on distance to sand
    for (int y_coord = 0; y_coord < gridHeight; ++y_coord) {
        for (int x_coord = 0; x_coord < gridWidth; ++x_coord) {
            // Only modify if it was initially classified as GRASS_0 
            // (and not changed by water texturing, which it wouldn't be)
            if (grid[y_coord][x_coord] == BlockName::GRASS_0) {
                int dist = distanceToSand[y_coord][x_coord];

                if (dist == 1) { // Adjacent to sand
                    grid[y_coord][x_coord] = BlockName::GRASS_0;
                } else if (dist == 2) { // One grass block away from sand
                    grid[y_coord][x_coord] = BlockName::GRASS_1;
                } else if (dist >= 3) { // Two or more grass blocks away from sand (or isolated)
                    grid[y_coord][x_coord] = BlockName::GRASS_2;
                }
            }
        }
    }

    // 4. Convert grid to map for return
    std::map<std::pair<int, int>, BlockName> generatedBlocks;
    for (int y_coord = 0; y_coord < gridHeight; ++y_coord) {
        for (int x_coord = 0; x_coord < gridWidth; ++x_coord) {
            generatedBlocks[{x_coord, y_coord}] = grid[y_coord][x_coord];
        }
    }

    return generatedBlocks;
}

// Places decorative elements based on configuration rules
void placeTerrainElements(
    ElementsOnMap& elementsManager,
    const Map& map,
    int gridWidth,
    int gridHeight
) {
    // Get generation rules from the global configuration
    const auto& rules = g_terrainConfig.getGenerationRules();
    
    // Count of different block types for debugging
    int sandCount = 0;
    int grassCount = 0;
    int waterCount = 0;
    int otherCount = 0;
    
    // Count blocks for debugging
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            BlockName blockType = map.getBlockNameByCoordinates(x, y);
            if (blockType == BlockName::SAND) {
                sandCount++;
            } else if (blockType >= BlockName::GRASS_0 && blockType <= BlockName::GRASS_5) {
                grassCount++;
            } else if (blockType >= BlockName::WATER_0 && blockType <= BlockName::WATER_4) {
                waterCount++;
            } else {
                otherCount++;
            }
        }
    }
      // Process each generation rule
    for (const auto& rule : rules) {
        if (rule.spawnType == SpawnType::ELEMENT) {
            placeElementsFromRule(elementsManager, map, gridWidth, gridHeight, rule);
        } else if (rule.spawnType == SpawnType::ENTITY) {
            placeEntitiesFromRule(map, gridWidth, gridHeight, rule);
        }
        // Future: handle BLOCK spawn types
    }
    
    std::cout << "Terrain blocks: " << sandCount << " sand, " << grassCount << " grass, " 
              << waterCount << " water blocks" << std::endl;
    
    // Reset the collision cache to ensure collision detection uses up-to-date information
    resetCollisionCache();
}

// Helper function to place elements based on a single generation rule
void placeElementsFromRule(
    ElementsOnMap& elementsManager,
    const Map& map,
    int gridWidth,
    int gridHeight,
    const GenerationRuleInfo& rule
) {
    int placedCount = 0;
    std::vector<std::pair<int, int>> placedLocations;
    
    // Pre-compute proximity blocks locations if needed
    std::vector<std::pair<int, int>> proximityBlockLocations;
    if (!rule.proximityBlocks.empty() && rule.maxDistanceFromBlocks > 0) {
        for (int y = 0; y < gridHeight; y++) {
            for (int x = 0; x < gridWidth; x++) {
                BlockName blockType = map.getBlockNameByCoordinates(x, y);
                for (const auto& proximityBlock : rule.proximityBlocks) {
                    if (blockType == proximityBlock) {
                        proximityBlockLocations.push_back({x, y});
                        break;
                    }
                }
            }
        }
    }
      // Iterate through all grid positions using either sequential or random order
    if (rule.randomPlacement) {
        // Create a list of all valid grid positions
        std::vector<std::pair<int, int>> gridPositions;
        for (int y = 0; y < gridHeight; y++) {            for (int x = 0; x < gridWidth; x++) {
                gridPositions.push_back({x, y});
            }
        }
        
        // Shuffle the positions for random placement
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(gridPositions.begin(), gridPositions.end(), g);
        
        // Process positions in randomized order
        for (const auto& pos : gridPositions) {
            if (placedCount >= rule.maxSpawns) break;
            
            int x = pos.first;
            int y = pos.second;
            
            BlockName blockType = map.getBlockNameByCoordinates(x, y);
            
            // Check if this block type is valid for spawning
            bool validBlock = false;
            for (const auto& spawnBlock : rule.spawnBlocks) {
                if (blockType == spawnBlock) {
                    validBlock = true;
                    break;
                }
            }
            
            if (!validBlock) continue;
            
            // Check spawn probability
            if (rand() % rule.spawnChance != 0) continue;
            
            // Check distance from previously placed elements of this rule
            bool tooClose = false;
            if (rule.minDistanceFromSameRule > 0) {
                for (const auto& placed : placedLocations) {
                    int dx = placed.first - x;
                    int dy = placed.second - y;
                    float distanceSquared = static_cast<float>(dx*dx + dy*dy);
                    if (distanceSquared < rule.minDistanceFromSameRule * rule.minDistanceFromSameRule) {
                        tooClose = true;
                        break;
                    }
                }
            }
            
            if (tooClose) continue;
            
            // Check proximity to required blocks
            bool nearProximityBlocks = true;
            if (!rule.proximityBlocks.empty() && rule.maxDistanceFromBlocks > 0) {
                nearProximityBlocks = false;
                for (const auto& proximityLoc : proximityBlockLocations) {
                    int dx = proximityLoc.first - x;
                    int dy = proximityLoc.second - y;
                    float distanceSquared = static_cast<float>(dx*dx + dy*dy);
                    if (distanceSquared <= rule.maxDistanceFromBlocks * rule.maxDistanceFromBlocks) {
                        nearProximityBlocks = true;
                        break;
                    }
                }
            }
            
            if (!nearProximityBlocks) continue;
            
            // Determine how many elements to place (group spawning)
            int elementsToPlace = 1;
            if (rule.spawnInGroup) {
                elementsToPlace = rule.groupNumberMin + 
                    (rand() % (rule.groupNumberMax - rule.groupNumberMin + 1));
            }
            
            // Place the element(s)
            for (int groupIndex = 0; groupIndex < elementsToPlace && placedCount < rule.maxSpawns; groupIndex++) {
                // Calculate position for this element
                float elementX = x + 0.5f;  // Center of the block
                float elementY = y + 0.5f;  // Center of the block
                
                // Add group positioning offset if spawning in groups
                if (rule.spawnInGroup && groupIndex > 0) {
                    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
                    float distance = static_cast<float>(rand()) / RAND_MAX * rule.groupRadius;
                    elementX += distance * cos(angle);
                    elementY += distance * sin(angle);
                }
                
                // Select element to spawn (equiprobable if multiple)
                ElementName selectedElement = rule.spawnElements[rand() % rule.spawnElements.size()];
                
                // Calculate scale with random variation
                float randomScale = rule.scaleMin + 
                    static_cast<float>(rand()) / RAND_MAX * (rule.scaleMax - rule.scaleMin);
                float finalScale = rule.baseScale * randomScale;
                
                // Calculate rotation
                float finalRotation = rule.rotation;
                if (rule.rotation < 0) {  // -1 means random rotation
                    finalRotation = static_cast<float>(rand()) / RAND_MAX * 360.0f;
                }
                
                // Create unique name for this element
                std::string elementName = rule.ruleName + "_" + std::to_string(placedCount);
                
                // Place the element
                elementsManager.placeElement(
                    elementName,
                    selectedElement,
                    finalScale,
                    elementX,
                    elementY,
                    finalRotation,
                    rule.defaultSpriteSheetPhase,
                    rule.defaultSpriteSheetFrame,
                    rule.isAnimated,
                    rule.animationSpeed,
                    rule.anchorPoint,
                    rule.additionalXAnchorOffset,
                    rule.additionalYAnchorOffset
                );
                
                placedCount++;
                
                // Only track the first element of a group for distance calculations
                if (groupIndex == 0) {
                    placedLocations.push_back({x, y});
                }
            }
        }
    } else {
        // Sequential placement (original algorithm)
        for (int y = 0; y < gridHeight && placedCount < rule.maxSpawns; y++) {
            for (int x = 0; x < gridWidth && placedCount < rule.maxSpawns; x++) {
                BlockName blockType = map.getBlockNameByCoordinates(x, y);
                
                // Check if this block type is valid for spawning
                bool validBlock = false;
                for (const auto& spawnBlock : rule.spawnBlocks) {
                    if (blockType == spawnBlock) {
                        validBlock = true;
                        break;
                    }
                }
                
                if (!validBlock) continue;
                
                // Check spawn probability
                if (rand() % rule.spawnChance != 0) continue;
                
                // Check distance from previously placed elements of this rule
                bool tooClose = false;
                if (rule.minDistanceFromSameRule > 0) {
                    for (const auto& placed : placedLocations) {
                        int dx = placed.first - x;
                        int dy = placed.second - y;
                        float distanceSquared = static_cast<float>(dx*dx + dy*dy);
                        if (distanceSquared < rule.minDistanceFromSameRule * rule.minDistanceFromSameRule) {
                            tooClose = true;
                            break;
                        }
                    }
                }
                
                if (tooClose) continue;
                
                // Check proximity to required blocks
                bool nearProximityBlocks = true;
                if (!rule.proximityBlocks.empty() && rule.maxDistanceFromBlocks > 0) {
                    nearProximityBlocks = false;
                    for (const auto& proximityLoc : proximityBlockLocations) {
                        int dx = proximityLoc.first - x;
                        int dy = proximityLoc.second - y;
                        float distanceSquared = static_cast<float>(dx*dx + dy*dy);
                        if (distanceSquared <= rule.maxDistanceFromBlocks * rule.maxDistanceFromBlocks) {
                            nearProximityBlocks = true;
                            break;
                        }
                    }
                }
                
                if (!nearProximityBlocks) continue;
                
                // Determine how many elements to place (group spawning)
                int elementsToPlace = 1;
                if (rule.spawnInGroup) {
                    elementsToPlace = rule.groupNumberMin + 
                        (rand() % (rule.groupNumberMax - rule.groupNumberMin + 1));
                }
                
                // Place the element(s)
                for (int groupIndex = 0; groupIndex < elementsToPlace && placedCount < rule.maxSpawns; groupIndex++) {
                    // Calculate position for this element
                    float elementX = x + 0.5f;  // Center of the block
                    float elementY = y + 0.5f;  // Center of the block
                    
                    // Add group positioning offset if spawning in groups
                    if (rule.spawnInGroup && groupIndex > 0) {
                        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
                        float distance = static_cast<float>(rand()) / RAND_MAX * rule.groupRadius;
                        elementX += distance * cos(angle);
                        elementY += distance * sin(angle);
                    }
                    
                    // Select element to spawn (equiprobable if multiple)
                    ElementName selectedElement = rule.spawnElements[rand() % rule.spawnElements.size()];
                    
                    // Calculate scale with random variation
                    float randomScale = rule.scaleMin + 
                        static_cast<float>(rand()) / RAND_MAX * (rule.scaleMax - rule.scaleMin);
                    float finalScale = rule.baseScale * randomScale;
                    
                    // Calculate rotation
                    float finalRotation = rule.rotation;
                    if (rule.rotation < 0) {  // -1 means random rotation
                        finalRotation = static_cast<float>(rand()) / RAND_MAX * 360.0f;
                    }
                    
                    // Create unique name for this element
                    std::string elementName = rule.ruleName + "_" + std::to_string(placedCount);
                    
                    // Place the element
                    elementsManager.placeElement(
                        elementName,
                        selectedElement,
                        finalScale,
                        elementX,
                        elementY,
                        finalRotation,
                        rule.defaultSpriteSheetPhase,
                        rule.defaultSpriteSheetFrame,
                        rule.isAnimated,
                        rule.animationSpeed,
                        rule.anchorPoint,
                        rule.additionalXAnchorOffset,
                        rule.additionalYAnchorOffset
                    );
                    
                    placedCount++;
                    
                    // Only track the first element of a group for distance calculations
                    if (groupIndex == 0) {
                        placedLocations.push_back({x, y});
                    }
                }
            }
        }
    }
    
    std::cout << "Placed " << placedCount << " elements using rule '" << rule.ruleName << "'" << std::endl;
}

// Helper function to place entities based on a single generation rule
void placeEntitiesFromRule(
    const Map& map,
    int gridWidth,
    int gridHeight,
    const GenerationRuleInfo& rule
) {
    // Access the global entities manager
    extern EntitiesManager entitiesManager;
    
    int placedCount = 0;
    std::vector<std::pair<int, int>> placedLocations;
    
    // Pre-compute proximity blocks locations if needed
    std::vector<std::pair<int, int>> proximityBlockLocations;
    if (!rule.proximityBlocks.empty() && rule.maxDistanceFromBlocks > 0) {
        for (int y = 0; y < gridHeight; y++) {
            for (int x = 0; x < gridWidth; x++) {
                BlockName blockType = map.getBlockNameByCoordinates(x, y);
                for (const auto& proximityBlock : rule.proximityBlocks) {
                    if (blockType == proximityBlock) {
                        proximityBlockLocations.push_back({x, y});
                        break;
                    }
                }
            }
        }
    }
      // Iterate through all grid positions using either sequential or random order
    if (rule.randomPlacement) {
        // Create a list of all valid grid positions
        std::vector<std::pair<int, int>> gridPositions;
        for (int y = 0; y < gridHeight; y++) {            for (int x = 0; x < gridWidth; x++) {
                gridPositions.push_back({x, y});
            }
        }
        
        // Shuffle the positions for random placement
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(gridPositions.begin(), gridPositions.end(), g);
        
        // Process positions in randomized order
        for (const auto& pos : gridPositions) {
            if (placedCount >= rule.maxSpawns) break;
            
            int x = pos.first;
            int y = pos.second;
            
            // Check if current block type matches spawn requirements
            BlockName currentBlockType = map.getBlockNameByCoordinates(x, y);
            bool matchesSpawnBlock = false;
            for (const auto& spawnBlock : rule.spawnBlocks) {
                if (currentBlockType == spawnBlock) {
                    matchesSpawnBlock = true;
                    break;
                }
            }
            
            if (!matchesSpawnBlock) continue;
            
            // Check spawn probability
            if (rand() % rule.spawnChance != 0) continue;
            
            // Check minimum distance from previous spawns of same rule
            bool tooClose = false;
            for (const auto& placedLoc : placedLocations) {
                int dx = placedLoc.first - x;
                int dy = placedLoc.second - y;
                float distanceSquared = static_cast<float>(dx*dx + dy*dy);
                if (distanceSquared < rule.minDistanceFromSameRule * rule.minDistanceFromSameRule) {
                    tooClose = true;
                    break;
                }
            }
            
            if (tooClose) continue;
            
            // Check proximity to required blocks if specified
            if (!rule.proximityBlocks.empty() && rule.maxDistanceFromBlocks > 0) {
                bool nearProximityBlocks = false;
                for (const auto& proximityLoc : proximityBlockLocations) {
                    int dx = proximityLoc.first - x;
                    int dy = proximityLoc.second - y;
                    float distanceSquared = static_cast<float>(dx*dx + dy*dy);
                    if (distanceSquared <= rule.maxDistanceFromBlocks * rule.maxDistanceFromBlocks) {
                        nearProximityBlocks = true;
                        break;
                    }
                }
                
                if (!nearProximityBlocks) continue;
            }
            
            // Determine how many entities to place (group spawning)
            int entitiesToPlace = 1;
            if (rule.spawnInGroup) {
                entitiesToPlace = rule.groupNumberMin + 
                    (rand() % (rule.groupNumberMax - rule.groupNumberMin + 1));
            }
            
            // Place the entity(s)
            for (int groupIndex = 0; groupIndex < entitiesToPlace && placedCount < rule.maxSpawns; groupIndex++) {
                // Calculate position for this entity
                float entityX = x + 0.5f;  // Center of the block
                float entityY = y + 0.5f;  // Center of the block
                
                // Add group positioning offset if spawning in groups
                if (rule.spawnInGroup && groupIndex > 0) {
                    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
                    float distance = static_cast<float>(rand()) / RAND_MAX * rule.groupRadius;
                    entityX += distance * cos(angle);
                    entityY += distance * sin(angle);
                }
                
                // Select entity to spawn (equiprobable if multiple)
                EntityName selectedEntity = rule.spawnEntities[rand() % rule.spawnEntities.size()];
                
                // Create unique name for this entity
                std::string entityInstanceName = rule.ruleName + "_" + std::to_string(placedCount);
                
                // Place the entity using safe placement to avoid collisions
                bool success = entitiesManager.placeEntityByTypeSafely(entityInstanceName, selectedEntity, entityX, entityY);
                
                if (success) {
                    placedCount++;
                    
                    // Only track the first entity of a group for distance calculations
                    if (groupIndex == 0) {
                        placedLocations.push_back({x, y});
                    }
                } else {
                    std::cout << "Warning: Failed to place entity " << entityInstanceName 
                              << " at position (" << entityX << "," << entityY << ")" << std::endl;
                }
            }
        }
    } else {
        // Sequential placement (original algorithm)
        for (int y = 0; y < gridHeight && placedCount < rule.maxSpawns; y++) {
            for (int x = 0; x < gridWidth && placedCount < rule.maxSpawns; x++) {
                // Check if current block type matches spawn requirements
                BlockName currentBlockType = map.getBlockNameByCoordinates(x, y);
                bool matchesSpawnBlock = false;
                for (const auto& spawnBlock : rule.spawnBlocks) {
                    if (currentBlockType == spawnBlock) {
                        matchesSpawnBlock = true;
                        break;
                    }
                }
                
                if (!matchesSpawnBlock) continue;
                
                // Check spawn probability
                if (rand() % rule.spawnChance != 0) continue;
                
                // Check minimum distance from previous spawns of same rule
                bool tooClose = false;
                for (const auto& placedLoc : placedLocations) {
                    int dx = placedLoc.first - x;
                    int dy = placedLoc.second - y;
                    float distanceSquared = static_cast<float>(dx*dx + dy*dy);
                    if (distanceSquared < rule.minDistanceFromSameRule * rule.minDistanceFromSameRule) {
                        tooClose = true;
                        break;
                    }
                }
                
                if (tooClose) continue;
                
                // Check proximity to required blocks if specified
                if (!rule.proximityBlocks.empty() && rule.maxDistanceFromBlocks > 0) {
                    bool nearProximityBlocks = false;
                    for (const auto& proximityLoc : proximityBlockLocations) {
                        int dx = proximityLoc.first - x;
                        int dy = proximityLoc.second - y;
                        float distanceSquared = static_cast<float>(dx*dx + dy*dy);
                        if (distanceSquared <= rule.maxDistanceFromBlocks * rule.maxDistanceFromBlocks) {
                            nearProximityBlocks = true;
                            break;
                        }
                    }
                    
                    if (!nearProximityBlocks) continue;
                }
                
                // Determine how many entities to place (group spawning)
                int entitiesToPlace = 1;
                if (rule.spawnInGroup) {
                    entitiesToPlace = rule.groupNumberMin + 
                        (rand() % (rule.groupNumberMax - rule.groupNumberMin + 1));
                }
                
                // Place the entity(s)
                for (int groupIndex = 0; groupIndex < entitiesToPlace && placedCount < rule.maxSpawns; groupIndex++) {
                    // Calculate position for this entity
                    float entityX = x + 0.5f;  // Center of the block
                    float entityY = y + 0.5f;  // Center of the block
                    
                    // Add group positioning offset if spawning in groups
                    if (rule.spawnInGroup && groupIndex > 0) {
                        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
                        float distance = static_cast<float>(rand()) / RAND_MAX * rule.groupRadius;
                        entityX += distance * cos(angle);
                        entityY += distance * sin(angle);
                    }
                    
                    // Select entity to spawn (equiprobable if multiple)
                    EntityName selectedEntity = rule.spawnEntities[rand() % rule.spawnEntities.size()];
                    
                    // Create unique name for this entity
                    std::string entityInstanceName = rule.ruleName + "_" + std::to_string(placedCount);
                    
                    // Place the entity using safe placement to avoid collisions
                    bool success = entitiesManager.placeEntityByTypeSafely(entityInstanceName, selectedEntity, entityX, entityY);
                    
                    if (success) {
                        placedCount++;
                        
                        // Only track the first entity of a group for distance calculations
                        if (groupIndex == 0) {
                            placedLocations.push_back({x, y});
                        }
                    } else {
                        std::cout << "Warning: Failed to place entity " << entityInstanceName 
                                  << " at position (" << entityX << "," << entityY << ")" << std::endl;
                    }
                }
            }
        }
    }
    
    std::cout << "Placed " << placedCount << " entities using rule '" << rule.ruleName << "'" << std::endl;
}
