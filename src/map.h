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
