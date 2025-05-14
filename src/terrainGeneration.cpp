#include "terrainGeneration.h"
#include "../src/map.h" // For TextureName enum
#include <vector>
#include <queue>
#include <map>
#include <utility> // For std::pair
#include <limits>  // For std::numeric_limits
#include <cmath>   // For fmod, sqrt, etc.
#include <cstdlib> // For rand, srand
#include <ctime>   // For time

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


std::map<std::pair<int, int>, TextureName> generateTerrain(
    int gridWidth, int gridHeight, 
    float featureSizeFactor, 
    float waterThreshold, float grassThreshold) {

    initializeBaseNoiseIfNeeded(gridWidth, gridHeight, featureSizeFactor);

    std::vector<std::vector<TextureName>> grid(gridHeight, std::vector<TextureName>(gridWidth));

    // 1. Initial terrain generation based on noise
    for (int y_coord = 0; y_coord < gridHeight; ++y_coord) {
        for (int x_coord = 0; x_coord < gridWidth; ++x_coord) {
            float noiseValue = getInterpolatedNoise(
                static_cast<float>(x_coord) / gridWidth, 
                static_cast<float>(y_coord) / gridHeight
            );

            if (noiseValue < waterThreshold) {
                grid[y_coord][x_coord] = TextureName::WATER_0; // Initial water type (will be refined)
            } else if (noiseValue < grassThreshold) {
                grid[y_coord][x_coord] = TextureName::SAND;
            } else {
                grid[y_coord][x_coord] = TextureName::GRASS;
            }
        }
    }

    // 2. BFS to calculate distances to sand
    std::vector<std::vector<int>> distanceToSand(gridHeight, std::vector<int>(gridWidth, std::numeric_limits<int>::max()));
    std::queue<std::pair<int, int>> bfsQueue;

    for (int y_coord = 0; y_coord < gridHeight; ++y_coord) {
        for (int x_coord = 0; x_coord < gridWidth; ++x_coord) {
            if (grid[y_coord][x_coord] == TextureName::SAND) {
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
            if (grid[y_coord][x_coord] != TextureName::SAND && grid[y_coord][x_coord] != TextureName::GRASS) {
                int dist = distanceToSand[y_coord][x_coord];
                
                if (dist == 1) {
                    grid[y_coord][x_coord] = TextureName::WATER_0;
                } else if (dist == 2) {
                    grid[y_coord][x_coord] = TextureName::WATER_1;
                } else if (dist == 3) {
                    grid[y_coord][x_coord] = TextureName::WATER_2;
                } else if (dist == 4) {
                    grid[y_coord][x_coord] = TextureName::WATER_3;
                } else if (dist >= 5) {
                    grid[y_coord][x_coord] = TextureName::WATER_4;
                }
                // If dist is 0, it's sand, so this block is skipped by the outer if.
                // If it was WATER_0 and dist is still max_int (e.g. isolated pond), it becomes WATER_5.
            }
        }
    }

    // 4. Convert grid to map for return
    std::map<std::pair<int, int>, TextureName> generatedBlocks;
    for (int y_coord = 0; y_coord < gridHeight; ++y_coord) {
        for (int x_coord = 0; x_coord < gridWidth; ++x_coord) {
            generatedBlocks[{x_coord, y_coord}] = grid[y_coord][x_coord];
        }
    }

    return generatedBlocks;
}
