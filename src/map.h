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
#include "enumDefinitions.h"


// Forward declaration of the GLBI_Engine class
namespace glbasimac {
    class GLBI_Engine;
}

// Define an enum for texture types for easy management
// To add a new texture:
// 1. Add its type here (e.g., DIRT, STONE)
// 2. Add its file path in Map::init() in map.cpp

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
      // Block transformation parameters
    BlockName transformBlockTo = BlockName::GRASS_0; // Default to GRASS_0 (no transformation)
    float transformBlockTimeIntervalStart = 0.0f; // Start of transformation time interval in seconds
    float transformBlockTimeIntervalEnd = 0.0f;   // End of transformation time interval in seconds
    bool hasTransformation = false; // Flag to indicate if this block type has transformation enabled
    bool savePreviousExistingBlock = false; // Flag to save the previous block at this location
    bool transformBlockToPreviousExistingBlock = false; // Flag to transform to the saved previous block
};

class Map {
public:
    Map();
    ~Map();

    // Initialize the map and load all configured textures
    bool init(glbasimac::GLBI_Engine& engine);

    // Place a block at given grid coordinates using its BlockName
    void placeBlock(BlockName name, int x, int y);

    // Place multiple blocks based on a map of coordinates to BlockNames
    void placeBlocks(const std::map<std::pair<int, int>, BlockName>& blocksToPlace);    // Place a texture on all blocks in a rectangular area using its BlockName
    void placeBlockArea(BlockName name, int x1, int y1, int x2, int y2);    // Draw all blocks, deltaTime for animations
    void drawBlocks(float startX, float endX, float startY, float endY, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop, double deltaTime);

    // Update block transformations
    void updateBlockTransformations(double deltaTime);

    // Public method to get a loaded texture ID by its type
    GLuint getTexture(BlockName type) const;
    
    // Get the texture name at the specified grid coordinates
    BlockName getBlockNameByCoordinates(int x, int y) const;
    
private:
    // Internal helper to load a single texture from file
    bool loadTexture(const std::string& path, GLuint& textureID, int& width, int& height);    struct Block {
        BlockName name; // Store BlockName instead of GLuint
        int x;
        int y;
        float currentFrame = 0.0f; // Added for individual animation state
        int rotationAngle = 0; // Added for block-specific rotation (0, 90, 180, 270)
        
        // Block transformation timing
        float transformationTimer = 0.0f; // Timer tracking how long this block has existed
        float transformationTarget = 0.0f; // The randomly chosen time when this block should transform
        bool hasBeenInitializedForTransformation = false; // Flag to ensure transformation timer is only set once
    };    std::vector<Block> blocks;
    std::map<BlockName, BlockInfo> textureDetails; // Stores detailed info for each texture
    std::map<std::pair<int, int>, size_t> blockPositionMap; // Maps coordinates to block index for faster lookups
    std::map<std::pair<int, int>, BlockName> savedExistingBlocks; // Maps coordinates to previously existing block types
    glbasimac::GLBI_Engine* enginePtr;
};

// Global instance of the map to be used in main.cpp
extern Map gameMap;
