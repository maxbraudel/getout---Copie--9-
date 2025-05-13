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
enum class TextureType {
    SAND,
    WATER
};

class Map {
public:
    Map();
    ~Map();

    // Initialize the map and load all configured textures
    bool init(glbasimac::GLBI_Engine& engine);

    // Place a block at given grid coordinates using a pre-loaded texture ID
    void placeBlock(GLuint textureID, int x, int y);

    // Place multiple blocks based on a map of coordinates to texture IDs
    void placeBlocks(const std::map<std::pair<int, int>, GLuint>& blocksToPlace);

    // Place a texture on all blocks in a rectangular area
    void placeBlockArea(GLuint textureID, int x1, int y1, int x2, int y2);

    // Draw all blocks
    void drawBlocks(float startX, float endX, float startY, float endY, int gridSize);

    // Public method to get a loaded texture ID by its type
    GLuint getTexture(TextureType type) const;
    
private:
    // Internal helper to load a single texture from file
    bool loadTexture(const std::string& path, GLuint& textureID);

    struct Block {
        GLuint textureID;
        int x;
        int y;
    };

    std::vector<Block> blocks;
    std::map<TextureType, GLuint> textureIDs; // Stores loaded texture IDs mapped by TextureType
    glbasimac::GLBI_Engine* enginePtr;
};

// Global instance of the map to be used in main.cpp
extern Map gameMap;
