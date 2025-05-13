#pragma once

#define GLFW_INCLUDE_NONE
// Include glad first since it provides its own GL headers
#include "glad/glad.h"
// Then include GLFW
#include "GLFW/glfw3.h"
#include "glbasimac/glbi_texture.hpp"
#include "glbasimac/glbi_engine.hpp"
#include <string>
#include <memory>
#include <map> // Added to include std::map
#include <vector> // Added to include std::vector (used for private member blocks)

// Forward declaration of the GLBI_Engine class
namespace glbasimac {
    class GLBI_Engine;
}

class Map {
public:
    Map();
    ~Map();

    // Initialize the map
    bool init(glbasimac::GLBI_Engine& engine);

    // Load a texture from file
    bool loadTexture(const std::string& path, GLuint& textureID);

    // Place a block at given grid coordinates
    void placeBlock(GLuint textureID, int x, int y);

    // Place multiple blocks based on a map of coordinates to texture IDs
    void placeBlocks(const std::map<std::pair<int, int>, GLuint>& blocksToPlace);

    // Place a texture on all blocks in a rectangular area
    void placeBlockArea(GLuint textureID, int x1, int y1, int x2, int y2);

    // Draw all blocks
    void drawBlocks(float startX, float endX, float startY, float endY, int gridSize);

public:
    // Texture IDs - publicly accessible for easy use
    GLuint sandTexture;
    
private:
    struct Block {
        GLuint textureID;
        int x;
        int y;
    };

    std::vector<Block> blocks;
    glbasimac::GLBI_Engine* enginePtr;
};

// Global instance of the map to be used in main.cpp
extern Map gameMap;
