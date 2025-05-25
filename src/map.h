#pragma once

#define GLFW_INCLUDE_NONE
// Include glad first since it provides its own GL headers
#include "glad/glad.h"
// Then include GLFW
#include "GLFW/glfw3.h"
#include "glbasimac/glbi_texture.hpp" // May review if this specific include is still needed by map.h directly
#include "glbasimac/glbi_engine.hpp"
#include <string>
#include <vector>
#include <map>
#include <stdexcept> // For std::runtime_error

// Forward declaration of the GLBI_Engine class
namespace glbasimac {
    class GLBI_Engine;
}

// Define an enum for texture types for easy management
// To add a new texture:
// 1. Add its type here (e.g., DIRT, STONE)
// 2. Add its file path in Map::init() in map.cpp
enum class TextureName {
    GRASS_0,
    GRASS_1,
    GRASS_2,
    GRASS_3,
    GRASS_4,
    GRASS_5,
    SAND,
    WATER_0,
    WATER_1,
    WATER_2,
    WATER_3,
    WATER_4
};

// Enum for texture animation type
enum class TextureAnimationType {
    STATIC,
    ANIMATED
};

// Struct to hold all details for a texture
struct BlockInfo {
    std::string path;
    TextureAnimationType animType = TextureAnimationType::STATIC;
    bool animationStartRandomFrame = false; // Added for random start frame
    float animationSpeed = 0.0f; // FPS for animated textures
    GLuint textureID = 0;
    int frameCount = 1;          // Total frames in the sprite sheet
    float currentFrame = 0.0f;   // Default starting frame, will be overridden by Block.currentFrame for individual blocks
    int frameHeight = 16;        // Height of a single frame, assuming 16px
    int textureWidth = 0;
    int textureHeight = 0;
    bool randomizedRotation = false; // Added for randomized rotation
};

class Map {
public:
    Map();
    ~Map();

    // Initialize the map and load all configured textures
    bool init(glbasimac::GLBI_Engine& engine);

    // Place a block at given grid coordinates using its TextureName
    void placeBlock(TextureName name, int x, int y);

    // Place multiple blocks based on a map of coordinates to TextureNames
    void placeBlocks(const std::map<std::pair<int, int>, TextureName>& blocksToPlace);    // Place a texture on all blocks in a rectangular area using its TextureName
    void placeBlockArea(TextureName name, int x1, int y1, int x2, int y2);

    // Draw all blocks, deltaTime for animations
    void drawBlocks(float startX, float endX, float startY, float endY, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop, double deltaTime);

    // Public method to get a loaded texture ID by its type
    GLuint getTexture(TextureName type) const;
    
    // Get the texture name at the specified grid coordinates
    TextureName getBlockNameByCoordinates(int x, int y) const;
    
private:
    // Internal helper to load a single texture from file
    bool loadTexture(const std::string& path, GLuint& textureID, int& width, int& height);

    struct Block {
        TextureName name; // Store TextureName instead of GLuint
        int x;
        int y;
        float currentFrame = 0.0f; // Added for individual animation state
        int rotationAngle = 0; // Added for block-specific rotation (0, 90, 180, 270)
    };

    std::vector<Block> blocks;
    std::map<TextureName, BlockInfo> textureDetails; // Stores detailed info for each texture
    std::map<std::pair<int, int>, size_t> blockPositionMap; // Maps coordinates to block index for faster lookups
    glbasimac::GLBI_Engine* enginePtr;
};

// Global instance of the map to be used in main.cpp
extern Map gameMap;
