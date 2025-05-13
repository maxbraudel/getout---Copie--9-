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

Map::Map() : enginePtr(nullptr) {
}

Map::~Map() {
    // Clean up all loaded textures
    for (auto const& pair : textureDetails) { // Changed from structured binding
        if (pair.second.textureID > 0) { // Access info via pair.second
            glDeleteTextures(1, &pair.second.textureID);
        }
    }
    textureDetails.clear();
}

bool Map::init(glbasimac::GLBI_Engine& engine) {
    enginePtr = &engine;

    // Print current working directory to debug file path issues
    char cwd[1024];
    if (GetCurrentDir(cwd, sizeof(cwd)) != NULL) {
        std::cout << "Current working directory: " << cwd << std::endl;
    }
    
    // Define texture configurations
    // std::map<TextureName, TextureInfo> textureConfigs = { // C++11 might struggle with this direct initialization
    //     {TextureName::SAND, {
    //         "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\sand.png", 
    //         TextureAnimationType::STATIC
    //     }},
    //     {TextureName::WATER, {
    //         "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water.png", 
    //         TextureAnimationType::STATIC
    //     }},
    //     {TextureName::WATER_ANIMATED, {
    //         "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\waterAnimated.png", 
    //         TextureAnimationType::ANIMATED, 
    //         10.0f // animationSpeed FPS
    //     }}
    // };

    // C++11 compatible way to initialize the map
    std::map<TextureName, TextureInfo> textureConfigs;

    TextureInfo sandInfo;
    sandInfo.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\sand.png";
    sandInfo.animType = TextureAnimationType::STATIC;
    textureConfigs[TextureName::SAND] = sandInfo;

    TextureInfo waterInfo;
    waterInfo.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water.png";
    waterInfo.animType = TextureAnimationType::STATIC;
    textureConfigs[TextureName::WATER] = waterInfo;

    TextureInfo waterAnimatedInfo;
    waterAnimatedInfo.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\waterAnimated.png";
    waterAnimatedInfo.animType = TextureAnimationType::ANIMATED;
    waterAnimatedInfo.animationSpeed = 0.01f;
    textureConfigs[TextureName::WATER_ANIMATED] = waterAnimatedInfo;


    for (auto it = textureConfigs.begin(); it != textureConfigs.end(); ++it) { // Changed to iterator loop
        TextureName name = it->first;
        TextureInfo& info = it->second; // Get a reference to modify

        std::cout << "Attempting to load texture for type " << static_cast<int>(name) << " from: " << info.path << std::endl;
        std::ifstream testFile(info.path.c_str());
        if (!testFile.good()) {
            std::cerr << "✗ Texture file NOT found at: " << info.path << std::endl;
            // return false; // Uncomment to make it a fatal error
        } else {
            testFile.close();
            std::cout << "✓ Texture file found at: " << info.path << std::endl;
        }
        
        if (!loadTexture(info.path, info.textureID, info.textureWidth, info.textureHeight)) {
            std::cerr << "Failed to load texture: " << info.path << std::endl;
            // return false; // Uncomment to make it a fatal error
        }

        if (info.animType == TextureAnimationType::ANIMATED) {
            if (info.frameHeight > 0) {
                info.frameCount = info.textureHeight / info.frameHeight;
            } else {
                info.frameCount = 1; // Avoid division by zero
            }
            std::cout << "Animated texture loaded: " << info.path << " with " << info.frameCount << " frames." << std::endl;
        }
        textureDetails[name] = info; // Store the configured and loaded texture info
    }
    
    std::cout << "Map initialized. Loaded " << textureDetails.size() << " texture configurations." << std::endl;
    return true;
}

bool Map::loadTexture(const std::string& path, GLuint& textureID, int& width, int& height) {
    // Print attempting to load
    std::cout << "Attempting to load texture from: " << path << std::endl;
    
    // Generate texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
      // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      // Make sure stb_image uses the correct orientation
    stbi_set_flip_vertically_on_load(true);
    
    // Load image using stb_image
    int nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    
    if (data) {
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
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        
        std::cout << "Texture loaded successfully: " << path << std::endl;
        std::cout << "Dimensions: " << width << "x" << height << ", Channels: " << nrChannels << std::endl;
        std::cout << "TextureID: " << textureID << std::endl;
        
        stbi_image_free(data);
        return true;
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }
}

GLuint Map::getTexture(TextureName name) const {
    auto it = textureDetails.find(name);
    if (it != textureDetails.end()) {
        return it->second.textureID;
    }
    std::cerr << "Texture type " << static_cast<int>(name) << " not found!" << std::endl;
    return 0; 
}

void Map::placeBlock(TextureName name, int x, int y) {
    Block newBlock;
    newBlock.name = name;
    newBlock.x = x;
    newBlock.y = y;
    blocks.push_back(newBlock);
}

void Map::placeBlocks(const std::map<std::pair<int, int>, TextureName>& blocksToPlace) {
    for (const auto& pair : blocksToPlace) {
        const std::pair<int, int>& coords = pair.first;
        TextureName name = pair.second;
        placeBlock(name, coords.first, coords.second);
    }
}

void Map::placeBlockArea(TextureName name, int x1, int y1, int x2, int y2) {
    int startX = std::min(x1, x2);
    int endX = std::max(x1, x2);
    int startY = std::min(y1, y2);
    int endY = std::max(y1, y2);

    for (int iy = startY; iy <= endY; ++iy) {
        for (int ix = startX; ix <= endX; ++ix) {
            placeBlock(name, ix, iy);
        }
    }
}

void Map::drawBlocks(float startX, float endX, float startY, float endY, int gridSize, double deltaTime) {
    float cellWidth = (endX - startX) / gridSize;
    float cellHeight = (endY - startY) / gridSize;

    glUseProgram(0); 
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    for (auto& block : blocks) { // Iterate by reference to potentially modify block state if needed in future
        auto it = textureDetails.find(block.name);
        if (it == textureDetails.end()) {
            std::cerr << "Texture details not found for block type " << static_cast<int>(block.name) << std::endl;
            continue;
        }
        
        TextureInfo& texInfo = it->second; // Use reference to update currentFrame for animated textures

        float x = startX + block.x * cellWidth;
        float y = startY + (gridSize - block.y - 1) * cellHeight;
        
        glBindTexture(GL_TEXTURE_2D, texInfo.textureID);
        
        float texCoordYStart = 0.0f;
        float texCoordYEnd = 1.0f;

        if (texInfo.animType == TextureAnimationType::ANIMATED && texInfo.frameCount > 0) {
            texInfo.currentFrame += static_cast<float>(deltaTime) * texInfo.animationSpeed;
            if (texInfo.currentFrame >= texInfo.frameCount) {
                texInfo.currentFrame = fmod(texInfo.currentFrame, static_cast<float>(texInfo.frameCount));
            }
            
            float frameTexHeight = 1.0f / texInfo.frameCount;
            texCoordYStart = (static_cast<int>(texInfo.currentFrame)) * frameTexHeight;
            texCoordYEnd = texCoordYStart + frameTexHeight;
        }

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, texCoordYStart); glVertex2f(x, y);                            
        glTexCoord2f(1.0f, texCoordYStart); glVertex2f(x + cellWidth, y);                
        glTexCoord2f(1.0f, texCoordYEnd); glVertex2f(x + cellWidth, y + cellHeight);    
        glTexCoord2f(0.0f, texCoordYEnd); glVertex2f(x, y + cellHeight);                
        glEnd();
    }
    
    glDisable(GL_TEXTURE_2D);
}
