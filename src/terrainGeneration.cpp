#include "terrainGeneration.h"
#include "map.h" // For BlockName enum and gameMap
#include "collision.h" // For collision detection
#include "globals.h" // For DEBUG_MAP flag
#include <vector>
#include <queue>
#include <map>
#include <utility> // For std::pair
#include <limits>  // For std::numeric_limits
#include <cmath>   // For fmod, sqrt, etc.
#include <cstdlib> // For rand, srand
#include <ctime>   // For time
#include <iostream> // For debugging output
#include "enumDefinitions.h"


// Static variables for noise generation (assuming these are part of your existing setup)
static std::vector<std::vector<float>> baseNoiseGrid;
static int baseNoiseWidth_static = 0; // Renamed to avoid conflict if baseNoiseWidth is a param
static int baseNoiseHeight_static = 0; // Renamed
static bool baseNoiseInitialized = false;

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

// Places decorative elements (bushes, etc.) on appropriate terrain blocks
void placeTerrainElements(
    ElementsOnMap& elementsManager,
    const Map& map,
    int gridWidth,
    int gridHeight
) {
    // Keep track of how many bushes we've placed to give unique IDs
    int bushCount = 0;
    
    // Count of different block types for debugging
    int sandCount = 0;
    int grassCount = 0;
    int waterCount = 0;
    int otherCount = 0;
    
    // Track placed bush locations for distance checking
    std::vector<std::pair<int, int>> placedBushes;
    const int MIN_COCONUT_TREE_DISTANCE = 4; // Minimum distance between trees
    const int MAX_WATER_DISTANCE = 3; // Maximum distance from water for coconut trees
    
    // First, find all water blocks and store their positions for distance checks
    std::vector<std::pair<int, int>> waterBlocks;
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            BlockName blockType = map.getBlockNameByCoordinates(x, y);
            if (blockType >= BlockName::WATER_0 && blockType <= BlockName::WATER_4) {
                waterBlocks.push_back({x, y});
            }
        }
    }
    
    // Iterate through all grid positions
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            // Get the actual block type from the map
            BlockName blockType = map.getBlockNameByCoordinates(x, y);
            
            // Count block types for debugging
            if (blockType == BlockName::SAND) {
                // Count sand blocks and edges
                sandCount++;
                // Place trees with 1/50 chance, but limit to max 50 total trees for performance
                const int MAX_COCONUT_TREES = 1000;
                const int COCONUT_TREE_CHANCE = 50; // 1/50 chance
                
                if (bushCount < MAX_COCONUT_TREES && rand() % COCONUT_TREE_CHANCE == 0) {
                    // Check distance from existing trees
                    bool tooClose = false;
                    for (const auto& bush : placedBushes) {
                        int dx = bush.first - x;
                        int dy = bush.second - y;
                        int distanceSquared = dx*dx + dy*dy;
                        
                        // Use square of distance to avoid square root calculation
                        if (distanceSquared < MIN_COCONUT_TREE_DISTANCE * MIN_COCONUT_TREE_DISTANCE) {
                            tooClose = true;
                            break;
                        }
                    }
                    
                    // Check distance from water blocks - trees must be within MAX_WATER_DISTANCE blocks from water
                    bool nearWater = false;
                    for (const auto& water : waterBlocks) {
                        int dx = water.first - x;
                        int dy = water.second - y;
                        int distanceSquared = dx*dx + dy*dy;
                        
                        // Check if within MAX_WATER_DISTANCE blocks from water (using squared distance)
                        if (distanceSquared <= MAX_WATER_DISTANCE * MAX_WATER_DISTANCE) {
                            nearWater = true;
                            break;
                        }
                    }
                    
                    // Only place tree if not too close to existing trees AND is near water
                    if (!tooClose && nearWater) {
                        // Create a unique name for this coconut tree
                        std::string bushName = "terrain_coconut_tree_" + std::to_string(bushCount++);                        // Convert grid coordinates to world coordinates (center of the block)
                        float worldX = x + 0.5f;  // Center of the block
                        
                        // Ensure Y coordinate matches the system with (0,0) at bottom-left
                        float worldY = y + 0.5f;  // Center of the block
                        
                        // Add tree location to tracking vector
                        placedBushes.push_back({x, y});
                        
                        // Randomly pick one of the three coconut tree textures with equal probability
                        ElementName treeTexture;
                        int randomTree = rand() % 3;
                        switch (randomTree) {
                            case 0:
                                treeTexture = ElementName::COCONUT_TREE_1;
                                break;
                            case 1:
                                treeTexture = ElementName::COCONUT_TREE_2;
                                break;
                            case 2:
                                treeTexture = ElementName::COCONUT_TREE_3;
                                break;
                            default: // Should never happen, but just in case
                                treeTexture = ElementName::COCONUT_TREE_1;
                        }                        // Add random scale variation to trees for visual diversity
                        float randomScale = 0.7f + static_cast<float>(rand()) / RAND_MAX * (1.0f - 0.7f);
                        
                        // Place a coconut tree at this location
                        // Using default anchor point from texture (BOTTOM_CENTER for trees)                        // Using default anchor point from texture (BOTTOM_CENTER for trees)
                        elementsManager.placeElement(
                            bushName,                    // Unique name
                            treeTexture,                 // Randomly selected coconut tree texture
                            7.0f * randomScale,          // Size (scaled by 5.0f with random variation)
                            worldX,                      // X position
                            worldY,                      // Y position
                            0.0f,                        // No rotation
                            0,                           // Default sprite sheet phase
                            0,                           // Default sprite sheet frame
                            false,                       // Not animated
                            10.0f,                       // Default animation speed
                            AnchorPoint::USE_TEXTURE_DEFAULT, // Use texture's default anchor point
                            0.0f,                        // No additional X anchor offset
                            0.0f                         // No additional Y anchor offset
                        );
                        // Note: Hitbox parameters were removed as they're not supported in the function definition
                    }
                }
            } else if (blockType >= BlockName::GRASS_0 && blockType <= BlockName::GRASS_5) {
                grassCount++;
            } else if (blockType >= BlockName::WATER_0 && blockType <= BlockName::WATER_4) {
                waterCount++;
            } else {
                otherCount++;
            }
        }
    }    // Concise summary of terrain generation results
    std::cout << "Terrain blocks: " << sandCount << " sand, " << grassCount << " grass, " 
              << waterCount << " water blocks" << std::endl;
    std::cout << "Placed " << bushCount << " coconut trees (near water only)" << std::endl;
    
    // Reset the collision cache to ensure collision detection uses up-to-date information
    resetCollisionCache();
}
