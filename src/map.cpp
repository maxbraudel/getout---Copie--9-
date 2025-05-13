#include "map.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>   // For std::replace
#include <string>
#include <map>

// For cross-platform directory checking
#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

// Define STB_IMAGE_IMPLEMENTATION before including stb_image.h
#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/glbasimac/tools/stb_image.h"

// Global instance of the Map class
Map gameMap;

Map::Map() : enginePtr(nullptr) { // sandTexture removed from initializer list
}

Map::~Map() {
    // Clean up all loaded textures
    for (auto const& [key, val] : textureIDs) {
        if (val > 0) {
            glDeleteTextures(1, &val);
        }
    }
    textureIDs.clear();
}

bool Map::init(glbasimac::GLBI_Engine& engine) {
    enginePtr = &engine;

    // Print current working directory to debug file path issues
    char cwd[1024];
    if (GetCurrentDir(cwd, sizeof(cwd)) != NULL) {
        std::cout << "Current working directory: " << cwd << std::endl;
    }
    
    // Define texture paths
    // To add a new texture: Add its TextureType to the enum in map.h
    // and then add its file path mapping here.
    std::map<TextureType, std::string> texturePaths = {
        {TextureType::SAND, "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\sand.png"},
        {TextureType::WATER, "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water.png"},

    };

    for (const auto& pair : texturePaths) {
        TextureType type = pair.first;
        const std::string& path = pair.second;
        GLuint textureID = 0;

        std::cout << "Attempting to load texture for type " << static_cast<int>(type) << " from: " << path << std::endl;
        std::ifstream testFile(path.c_str());
        if (!testFile.good()) {
            std::cerr << "✗ Texture file NOT found at: " << path << std::endl;
            // Decide if one missing texture should halt all initialization
            // For now, we'll print an error and continue, but the textureID will be 0.
            // return false; // Uncomment to make it a fatal error
        } else {
            testFile.close();
            std::cout << "✓ Texture file found at: " << path << std::endl;
        }
        
        if (!loadTexture(path, textureID)) {
            std::cerr << "Failed to load texture: " << path << std::endl;
            // return false; // Uncomment to make it a fatal error
        }
        textureIDs[type] = textureID; // Store the loaded (or 0 if failed) texture ID
    }
    
    std::cout << "Map initialized. Loaded " << textureIDs.size() << " texture configurations." << std::endl;
    return true;
}

bool Map::loadTexture(const std::string& path, GLuint& textureID) {
    // Print attempting to load
    std::cout << "Attempting to load texture from: " << path << std::endl;
    
    // Generate texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
      // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Changed from GL_LINEAR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      // Make sure stb_image uses the correct orientation
    stbi_set_flip_vertically_on_load(true); // Flip images vertically to match OpenGL's coordinate system
    
    // Load image using stb_image (included in the engine)
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    
    if (data) {
        // Determine format based on number of channels
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else {
            std::cerr << "Unsupported number of channels: " << nrChannels << std::endl;
            stbi_image_free(data);
            return false;
        }
        
    // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        // glGenerateMipmap(GL_TEXTURE_2D); // Removed mipmap generation
        
        // Print debug info
        std::cout << "Texture loaded successfully: " << path << std::endl;
        std::cout << "Dimensions: " << width << "x" << height << ", Channels: " << nrChannels << std::endl;
        std::cout << "TextureID: " << textureID << std::endl;
        
        // Free image data
        stbi_image_free(data);
        return true;    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }
}

GLuint Map::getTexture(TextureType type) const {
    auto it = textureIDs.find(type);
    if (it != textureIDs.end()) {
        return it->second;
    }
    // Return a default/error value or throw an exception if texture not found
    std::cerr << "Texture type " << static_cast<int>(type) << " not found!" << std::endl;
    // You might want to throw an exception here to make it a more critical error
    // throw std::runtime_error("Texture type not found: " + std::to_string(static_cast<int>(type)));
    return 0; // Or some default texture ID if you have one
}

void Map::placeBlock(GLuint textureID, int x, int y) {
    // Create a new block and add it to our blocks vector
    Block newBlock;
    newBlock.textureID = textureID;
    newBlock.x = x;
    newBlock.y = y;
    blocks.push_back(newBlock);
}

void Map::placeBlocks(const std::map<std::pair<int, int>, GLuint>& blocksToPlace) {
    for (const auto& pair : blocksToPlace) {
        const std::pair<int, int>& coords = pair.first;
        GLuint textureID = pair.second;
        placeBlock(textureID, coords.first, coords.second);
    }
}

void Map::placeBlockArea(GLuint textureID, int x1, int y1, int x2, int y2) {
    int startX = std::min(x1, x2);
    int endX = std::max(x1, x2);
    int startY = std::min(y1, y2);
    int endY = std::max(y1, y2);

    for (int iy = startY; iy <= endY; ++iy) {
        for (int ix = startX; ix <= endX; ++ix) {
            placeBlock(textureID, ix, iy);
        }
    }
}

void Map::drawBlocks(float startX, float endX, float startY, float endY, int gridSize) {
    // Calculate cell size
    float cellWidth = (endX - startX) / gridSize;
    float cellHeight = (endY - startY) / gridSize;
    
    // Print debug info for the first run
    static bool firstRun = true;
    if (firstRun) {
        std::cout << "DrawBlocks params: " << std::endl;
        std::cout << "startX: " << startX << ", endX: " << endX << std::endl;
        std::cout << "startY: " << startY << ", endY: " << endY << std::endl;
        std::cout << "gridSize: " << gridSize << std::endl;
        std::cout << "cellWidth: " << cellWidth << ", cellHeight: " << cellHeight << std::endl;
        std::cout << "Number of blocks: " << blocks.size() << std::endl;
        firstRun = false;
    }

    glUseProgram(0); // Ensure fixed-function pipeline
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Set color to white

      // Enable texturing
    glEnable(GL_TEXTURE_2D);
    
    // Set texture environment
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    // Draw each block in the blocks vector
    for (const auto& block : blocks) {// Calculate position of this block in normalized coordinates
        // Convert from grid coordinates to screen coordinates
        // Remember that (0,0) is the top-left corner of the grid
        float x = startX + block.x * cellWidth;
        float y = startY + (gridSize - block.y - 1) * cellHeight; // Flip y-coordinate for OpenGL
        
        // std::cout << "Drawing block at grid (" << block.x << "," << block.y << ")" << std::endl;
        // std::cout << "Translated to coords (" << x << "," << y << ")" << std::endl;
        
        // Bind the texture
        glBindTexture(GL_TEXTURE_2D, block.textureID);
        
        // Draw textured quad
        glBegin(GL_QUADS);
        
        // Corrected texture coordinates
        // Vertex order: bottom-left, bottom-right, top-right, top-left
        // TexCoord (0,0) is bottom-left of texture after stbi_flip
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);                            // Bottom-left vertex, Bottom-left texel
        glTexCoord2f(1.0f, 0.0f); glVertex2f(x + cellWidth, y);                // Bottom-right vertex, Bottom-right texel
        glTexCoord2f(1.0f, 1.0f); glVertex2f(x + cellWidth, y + cellHeight);    // Top-right vertex, Top-right texel
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + cellHeight);                // Top-left vertex, Top-left texel
        
        glEnd();
    }
    
    // Disable texturing
    glDisable(GL_TEXTURE_2D);
}
