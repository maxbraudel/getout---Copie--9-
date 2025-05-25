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
    // Reserve some space for blocks to reduce reallocations
    blocks.reserve(5000); // Assuming a map size of approximately 70x70
}

Map::~Map() {
    // Clean up all loaded textures
    for (auto const& pair : textureDetails) { // Changed from structured binding
        if (pair.second.textureID > 0) { // Access info via pair.second
            glDeleteTextures(1, &pair.second.textureID);
        }
    }
    textureDetails.clear();
    blocks.clear();
    blockPositionMap.clear();
}

bool Map::init(glbasimac::GLBI_Engine& engine) {
    enginePtr = &engine;

    // Print current working directory to debug file path issues
    char cwd[1024];
    if (GetCurrentDir(cwd, sizeof(cwd)) != NULL) {
        std::cout << "Current working directory: " << cwd << std::endl;
    }
    
    // Define texture configurations
    // std::map<TextureName, BlockInfo> textureConfigs = { // C++11 might struggle with this direct initialization
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
    std::map<TextureName, BlockInfo> textureConfigs;

    BlockInfo grass0Info;
    grass0Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\grass0.png";
    grass0Info.animType = TextureAnimationType::STATIC;
    grass0Info.randomizedRotation = true; // Enable randomized rotation for WATER_4
    textureConfigs[TextureName::GRASS_0] = grass0Info;

    BlockInfo grass1Info;
    grass1Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\grass1.png";
    grass1Info.animType = TextureAnimationType::STATIC;
    grass1Info.randomizedRotation = true; // Enable randomized rotation for WATER_4
    textureConfigs[TextureName::GRASS_1] = grass1Info;

    BlockInfo grass2Info;
    grass2Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\grass2.png";
    grass2Info.animType = TextureAnimationType::STATIC;
    grass2Info.randomizedRotation = true; // Enable randomized rotation for WATER_4
    textureConfigs[TextureName::GRASS_2] = grass2Info;

    BlockInfo sandInfo;
    sandInfo.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\sand.png";
    sandInfo.animType = TextureAnimationType::STATIC;
    textureConfigs[TextureName::SAND] = sandInfo;

    BlockInfo waterInfo; // For non-animated water, if you still have it
    waterInfo.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water.png"; // Assuming you might have a static water.png
    waterInfo.animType = TextureAnimationType::STATIC;
    // textureConfigs[TextureName::WATER] = waterInfo; // Uncomment if you have a WATER enum and texture

    BlockInfo water0Info;
    water0Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water0.png";
    water0Info.animType = TextureAnimationType::ANIMATED;
    water0Info.animationSpeed = 20.0f; // Example speed, adjust as needed
    water0Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water0Info.animationStartRandomFrame = true; // Enable random start frame
    textureConfigs[TextureName::WATER_0] = water0Info;

    BlockInfo water1Info;
    water1Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water1.png";
    water1Info.animType = TextureAnimationType::ANIMATED;
    water1Info.animationSpeed = 20.0f; // Example speed, adjust as needed
    water1Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water1Info.animationStartRandomFrame = true; // Enable random start frame
    textureConfigs[TextureName::WATER_1] = water1Info;

    BlockInfo water2Info;
    water2Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water2.png";
    water2Info.animType = TextureAnimationType::ANIMATED;
    water2Info.animationSpeed = 20.0f; // Example speed, adjust as needed
    water2Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water2Info.animationStartRandomFrame = true; // Enable random start frame
    water2Info.randomizedRotation = true; // Enable randomized rotation for WATER_4
    textureConfigs[TextureName::WATER_2] = water2Info;

    BlockInfo water3Info;
    water3Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water3.png";
    water3Info.animType = TextureAnimationType::ANIMATED;
    water3Info.animationSpeed = 20.0f; // Example speed, adjust as needed
    water3Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water3Info.animationStartRandomFrame = true; // Enable random start frame
    textureConfigs[TextureName::WATER_3] = water3Info;

    BlockInfo water4Info;
    water4Info.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\water4.png";
    water4Info.animType = TextureAnimationType::ANIMATED;
    water4Info.animationSpeed = 20.0f; // Example speed, adjust as needed
    water4Info.frameHeight = 16; // Assuming 16px frame height, adjust if different
    water4Info.animationStartRandomFrame = true; // Enable random start frame
    water4Info.randomizedRotation = true; // Enable randomized rotation for WATER_4
    textureConfigs[TextureName::WATER_4] = water4Info;




    for (auto it = textureConfigs.begin(); it != textureConfigs.end(); ++it) { // Changed to iterator loop
        TextureName name = it->first;
        BlockInfo& info = it->second; // Get a reference to modify

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
    // First check if a block already exists at these coordinates
    bool blockExists = false;
    size_t existingBlockIndex = 0;
    
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].x == x && blocks[i].y == y) {
            blockExists = true;
            existingBlockIndex = i;
            break;
        }
    }
    
    Block newBlock;
    newBlock.name = name;
    newBlock.x = x;
    newBlock.y = y;

    // Initialize currentFrame for the block
    auto it = textureDetails.find(name);
    if (it != textureDetails.end()) {
        const BlockInfo& texInfo = it->second;
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

        // Initialize rotationAngle
        if (texInfo.randomizedRotation) {
            int randomRotation = rand() % 4; // 0, 1, 2, or 3
            newBlock.rotationAngle = randomRotation * 90; // 0, 90, 180, or 270
        } else {
            newBlock.rotationAngle = 0;
        }
    } else {
        newBlock.currentFrame = 0.0f; // Default if texture info not found (should not happen)
        newBlock.rotationAngle = 0;
    }    // If a block already exists at these coordinates, replace it instead of adding a new one
    if (blockExists) {
        // Only replace if the texture is different (reduce unnecessary operations)
        if (blocks[existingBlockIndex].name != name) {
            blocks[existingBlockIndex] = newBlock;
        }
    } else {
        // Add the new block
        blocks.push_back(newBlock);
        // Update the position map
        blockPositionMap[{x, y}] = blocks.size() - 1;
    }
}

void Map::placeBlocks(const std::map<std::pair<int, int>, TextureName>& blocksToPlace) {
    // Since we already maintain blockPositionMap, we don't need to create it here anymore
      // Now process the blocks we want to place
    for (const auto& pair : blocksToPlace) {
        const std::pair<int, int>& coords = pair.first;
        TextureName name = pair.second;
        
        auto existingBlockIt = blockPositionMap.find(coords);
        if (existingBlockIt != blockPositionMap.end()) {
            // Block exists - check if we need to replace it
            size_t existingBlockIndex = existingBlockIt->second;
            if (blocks[existingBlockIndex].name != name) {                // Replace with new texture
                Block& block = blocks[existingBlockIndex];
                block.name = name;
                
                // Reset animation parameters
                auto texIt = textureDetails.find(name);
                if (texIt != textureDetails.end()) {
                    const BlockInfo& texInfo = texIt->second;
                    if (texInfo.animType == TextureAnimationType::ANIMATED && texInfo.animationStartRandomFrame && texInfo.frameCount > 0) {
                        block.currentFrame = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / texInfo.frameCount));
                        if (block.currentFrame >= texInfo.frameCount) {
                            block.currentFrame = static_cast<float>(texInfo.frameCount - 1);
                        }
                    } else {
                        block.currentFrame = 0.0f;
                    }
                    
                    if (texInfo.randomizedRotation) {
                        int randomRotation = rand() % 4;
                        block.rotationAngle = randomRotation * 90;
                    } else {
                        block.rotationAngle = 0;
                    }
                }
            }
        } else {
            // Block doesn't exist - add a new one
            Block newBlock;
            newBlock.name = name;
            newBlock.x = coords.first;
            newBlock.y = coords.second;
            
            // Initialize animation parameters
            auto texIt = textureDetails.find(name);
            if (texIt != textureDetails.end()) {
                const BlockInfo& texInfo = texIt->second;
                if (texInfo.animType == TextureAnimationType::ANIMATED && texInfo.animationStartRandomFrame && texInfo.frameCount > 0) {
                    newBlock.currentFrame = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / texInfo.frameCount));
                    if (newBlock.currentFrame >= texInfo.frameCount) {
                        newBlock.currentFrame = static_cast<float>(texInfo.frameCount - 1);
                    }
                } else {
                    newBlock.currentFrame = 0.0f;
                }
                
                if (texInfo.randomizedRotation) {
                    int randomRotation = rand() % 4;
                    newBlock.rotationAngle = randomRotation * 90;
                } else {
                    newBlock.rotationAngle = 0;
                }
            } else {
                newBlock.currentFrame = 0.0f;
                newBlock.rotationAngle = 0;
            }
            
            // Add the block and update the position map
            blocks.push_back(newBlock);
            blockPositionMap[coords] = blocks.size() - 1;
        }
    }
}

void Map::placeBlockArea(TextureName name, int x1, int y1, int x2, int y2) {
    int startX = std::min(x1, x2);
    int endX = std::max(x1, x2);
    int startY = std::min(y1, y2);
    int endY = std::max(y1, y2);

    // Create a map of coordinates to texture names for batch processing
    std::map<std::pair<int, int>, TextureName> blocksToPlace;
    for (int iy = startY; iy <= endY; ++iy) {
        for (int ix = startX; ix <= endX; ++ix) {
            blocksToPlace[{ix, iy}] = name;
        }
    }
    
    // Use the optimized placeBlocks function to place all blocks at once
    placeBlocks(blocksToPlace);
}

TextureName Map::getBlockNameByCoordinates(int x, int y) const {
    // Use the blockPositionMap for direct lookups
    auto it = blockPositionMap.find({x, y});
    if (it != blockPositionMap.end()) {
        size_t blockIndex = it->second;
        if (blockIndex < blocks.size()) {
            return blocks[blockIndex].name;
        } else {
            std::cerr << "Warning: Block index " << blockIndex << " out of bounds (blocks.size()=" << blocks.size() << ")" << std::endl;
        }
    }
    
    // If no block is found at these coordinates, return GRASS_0 as default
    // Check if the coordinates are within our grid bounds first
    if (x < 0 || y < 0 || x >= 70 || y >= 70) {
        std::cerr << "Warning: Coordinates (" << x << ", " << y << ") are outside the grid bounds" << std::endl;
    }
    return TextureName::GRASS_0;
}

void Map::drawBlocks(float startX, float endX, float startY, float endY, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop, double deltaTime) {
    // Calculate cell dimensions in screen coordinates
    float viewWidth = cameraRight - cameraLeft;
    float viewHeight = cameraTop - cameraBottom;
    float cellWidth = (endX - startX) / viewWidth;
    float cellHeight = (endY - startY) / viewHeight;

    glUseProgram(0); 
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // Only process blocks that are within or partially within the camera view
    for (auto& block : blocks) { // Iterate by reference to potentially modify block state if needed in future
        // Skip blocks that are outside the camera view
        if (block.x < cameraLeft - 1 || block.x > cameraRight + 1 || 
            block.y < cameraBottom - 1 || block.y > cameraTop + 1) {
            continue;
        }
        auto it = textureDetails.find(block.name);
        if (it == textureDetails.end()) {
            std::cerr << "Texture details not found for block type " << static_cast<int>(block.name) << std::endl;
            continue;
        }        const BlockInfo& texInfo = it->second; // New: Read-only for shared info
        Block& currentBlock = block; // Get reference to the current block instance
        
        // Convert block world coordinates to screen coordinates based on the camera view
        float worldX = currentBlock.x;
        float worldY = currentBlock.y;
        
        // Calculate position in screen space (normalized to [0,1] within the camera view)
        float normalizedX = (worldX - cameraLeft) / viewWidth;
        float normalizedY = (worldY - cameraBottom) / viewHeight;
        
        // Map from normalized [0,1] to screen coordinates
        float x = startX + normalizedX * (endX - startX);
        float y = startY + normalizedY * (endY - startY);
        
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

        // Define texture coordinates based on rotation
        float tc[8]; // Array to hold 8 texture coordinates (x1,y1, x2,y2, x3,y3, x4,y4)

        // Default: 0 degrees rotation (Bottom-left, Bottom-right, Top-right, Top-left)
        tc[0] = 0.0f; tc[1] = texCoordYStart; 
        tc[2] = 1.0f; tc[3] = texCoordYStart; 
        tc[4] = 1.0f; tc[5] = texCoordYEnd;   
        tc[6] = 0.0f; tc[7] = texCoordYEnd;   

        if (currentBlock.rotationAngle == 90) {
            // Rotated 90 deg: (Top-left, Bottom-left, Bottom-right, Top-right)
            tc[0] = 0.0f; tc[1] = texCoordYEnd;   
            tc[2] = 0.0f; tc[3] = texCoordYStart; 
            tc[4] = 1.0f; tc[5] = texCoordYStart; 
            tc[6] = 1.0f; tc[7] = texCoordYEnd;   
        } else if (currentBlock.rotationAngle == 180) {
            // Rotated 180 deg: (Top-right, Top-left, Bottom-left, Bottom-right)
            tc[0] = 1.0f; tc[1] = texCoordYEnd;   
            tc[2] = 0.0f; tc[3] = texCoordYEnd;   
            tc[4] = 0.0f; tc[5] = texCoordYStart; 
            tc[6] = 1.0f; tc[7] = texCoordYStart; 
        } else if (currentBlock.rotationAngle == 270) {
            // Rotated 270 deg: (Bottom-right, Top-right, Top-left, Bottom-left)
            tc[0] = 1.0f; tc[1] = texCoordYStart; 
            tc[2] = 1.0f; tc[3] = texCoordYEnd;   
            tc[4] = 0.0f; tc[5] = texCoordYEnd;   
            tc[6] = 0.0f; tc[7] = texCoordYStart; 
        }

        glBegin(GL_QUADS);
        glTexCoord2f(tc[0], tc[1]); glVertex2f(x, y);                            
        glTexCoord2f(tc[2], tc[3]); glVertex2f(x + cellWidth, y);                
        glTexCoord2f(tc[4], tc[5]); glVertex2f(x + cellWidth, y + cellHeight);    
        glTexCoord2f(tc[6], tc[7]); glVertex2f(x, y + cellHeight);                
        glEnd();
    }
    
    glDisable(GL_TEXTURE_2D);
}
