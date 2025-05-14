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
    //     {TextureName::WATER_0, {
    //         "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\waterAnimated.png", 
    //         TextureAnimationType::ANIMATED, 
    //         10.0f // animationSpeed FPS
    //     }}
    // };

    // C++11 compatible way to initialize the map
    std::map<TextureName, TextureInfo> textureConfigs;

    TextureInfo grassInfo;
    grassInfo.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\grass.png";
    grassInfo.animType = TextureAnimationType::STATIC;
    textureConfigs[TextureName::GRASS] = grassInfo;

    TextureInfo sandInfo;
    sandInfo.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\sand.png";
    sandInfo.animType = TextureAnimationType::STATIC;
    textureConfigs[TextureName::SAND] = sandInfo;

    TextureInfo waterInfo; // For non-animated water, if you still have it
    waterInfo.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water.png"; // Assuming you might have a static water.png
    waterInfo.animType = TextureAnimationType::STATIC;
    // textureConfigs[TextureName::WATER] = waterInfo; // Uncomment if you have a WATER enum and texture

    TextureInfo water0Info;
    water0Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water0.png";
    water0Info.animType = TextureAnimationType::ANIMATED;
    water0Info.animationSpeed = 5.0f; // Example speed, adjust as needed
    water0Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water0Info.animationStartRandomFrame = true; // Enable random start frame
    textureConfigs[TextureName::WATER_0] = water0Info;

    TextureInfo water1Info;
    water1Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water1.png";
    water1Info.animType = TextureAnimationType::ANIMATED;
    water1Info.animationSpeed = 5.0f; // Example speed, adjust as needed
    water1Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water1Info.animationStartRandomFrame = true; // Enable random start frame
    textureConfigs[TextureName::WATER_1] = water1Info;

    TextureInfo water2Info;
    water2Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water2.png";
    water2Info.animType = TextureAnimationType::ANIMATED;
    water2Info.animationSpeed = 5.0f; // Example speed, adjust as needed
    water2Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water2Info.animationStartRandomFrame = true; // Enable random start frame
    textureConfigs[TextureName::WATER_2] = water2Info;

    TextureInfo water3Info;
    water3Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water3.png";
    water3Info.animType = TextureAnimationType::ANIMATED;
    water3Info.animationSpeed = 5.0f; // Example speed, adjust as needed
    water3Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water3Info.animationStartRandomFrame = true; // Enable random start frame
    textureConfigs[TextureName::WATER_3] = water3Info;

    TextureInfo water4Info;
    water4Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water4.png";
    water4Info.animType = TextureAnimationType::ANIMATED;
    water4Info.animationSpeed = 5.0f; // Example speed, adjust as needed
    water4Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water4Info.animationStartRandomFrame = true; // Enable random start frame
    textureConfigs[TextureName::WATER_4] = water4Info;

    TextureInfo water5Info;
    water5Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water5.png";
    water5Info.animType = TextureAnimationType::ANIMATED;
    water5Info.animationSpeed = 5.0f; // Example speed, adjust as needed
    water5Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water5Info.animationStartRandomFrame = true; // Enable random start frame
    textureConfigs[TextureName::WATER_5] = water5Info;

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

    // Initialize currentFrame for the block
    auto it = textureDetails.find(name);
    if (it != textureDetails.end()) {
        const TextureInfo& texInfo = it->second;
        if (texInfo.animType == TextureAnimationType::ANIMATED && texInfo.animationStartRandomFrame && texInfo.frameCount > 0) {
            // Generate a random starting frame for this block instance
            newBlock.currentFrame = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / texInfo.frameCount));
            // Ensure the random frame is within [0, frameCount)
            if (newBlock.currentFrame >= texInfo.frameCount) {
                newBlock.currentFrame = static_cast<float>(texInfo.frameCount - 1);
            }
        } else {
            newBlock.currentFrame = 0.0f; // Default start frame if not random or not animated
        }
    } else {
        newBlock.currentFrame = 0.0f; // Default if texture info not found (should not happen)
    }

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
        
        const TextureInfo& texInfo = it->second; // New: Read-only for shared info
        Block& currentBlock = block; // Get reference to the current block instance

        float x = startX + currentBlock.x * cellWidth;
        float y = startY + (gridSize - currentBlock.y - 1) * cellHeight; // Adjusted for Y-down grid to Y-up OpenGL
        
        glBindTexture(GL_TEXTURE_2D, texInfo.textureID);
        
        float texCoordYStart = 0.0f;
        float texCoordYEnd = 1.0f;

        if (texInfo.animType == TextureAnimationType::ANIMATED && texInfo.frameCount > 0) {
            // Use and update the block's individual currentFrame
            currentBlock.currentFrame += static_cast<float>(deltaTime) * texInfo.animationSpeed;
            if (currentBlock.currentFrame >= texInfo.frameCount) {
                currentBlock.currentFrame = fmod(currentBlock.currentFrame, static_cast<float>(texInfo.frameCount));
            }
            
            float frameTexHeight = 1.0f / texInfo.frameCount;
            // Use the block's currentFrame for texture coordinates
            texCoordYStart = (static_cast<int>(currentBlock.currentFrame)) * frameTexHeight;
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
