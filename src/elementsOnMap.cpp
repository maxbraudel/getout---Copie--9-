#include "elementsOnMap.h"
#include "debug.h"
#include "globals.h" // For GRID_SIZE
#include <magic_enum.hpp>
#include <iostream>
#include <algorithm> // Added for std::find_if
#include "enumDefinitions.h"


// Include stb_image without defining STB_IMAGE_IMPLEMENTATION
// (It should already be defined in map.cpp)
#include "../third_party/glbasimac/tools/stb_image.h"

// Global instance
ElementsOnMap elementsManager;

// Define textures to load - using C++11 compatible initialization syntax
static std::vector<ElementInfo> createElementTexturesToLoad() {
    std::vector<ElementInfo> textures;
      // Static texture for test/grass
    ElementInfo testTexture;
    testTexture.name = ElementName::TEST;
    testTexture.path = "../assets/textures/blocks/grass.png";
    testTexture.type = ElementTextureType::STATIC;
    testTexture.anchorPoint = AnchorPoint::CENTER; // Default center anchor
    textures.push_back(testTexture);    // Static texture for coconut tree 1
    ElementInfo coconutTree1Texture;
    coconutTree1Texture.name = ElementName::COCONUT_TREE_1;
    coconutTree1Texture.path = "../assets/textures/decorations/coconut_tree_1.png";
    coconutTree1Texture.type = ElementTextureType::STATIC;
    coconutTree1Texture.anchorPoint = AnchorPoint::BOTTOM_CENTER; // Tree grows from ground up, so anchor at bottom
    coconutTree1Texture.anchorOffsetX = -0.3f; // X offset
    coconutTree1Texture.anchorOffsetY = 0.2f; // No Y offset
    coconutTree1Texture.hasCollision = true;  // Enable collision for this tree

    coconutTree1Texture.collisionShapePoints = {
        {-0.07f, 0.0f}, {-0.07f, 0.1}, {0.07f, 0.1f}, {0.07f, 0.0f}
    };
    textures.push_back(coconutTree1Texture);    ElementInfo coconutTree2Texture;
    coconutTree2Texture.name = ElementName::COCONUT_TREE_2;
    coconutTree2Texture.path = "../assets/textures/decorations/coconut_tree_2.png";
    coconutTree2Texture.type = ElementTextureType::STATIC;
    coconutTree2Texture.anchorPoint = AnchorPoint::BOTTOM_CENTER; // Tree grows from ground up, so anchor at bottom
    coconutTree2Texture.anchorOffsetX = 0.0f; // No offset
    coconutTree2Texture.anchorOffsetY = 0.8f; // No offset
    coconutTree2Texture.hasCollision = true;  // Enable collision for this tree
    coconutTree2Texture.collisionShapePoints = {
        {-0.07f, 0.0f}, {-0.07f, 0.1}, {0.07f, 0.1f}, {0.07f, 0.0f}
    };
    textures.push_back(coconutTree2Texture);    ElementInfo coconutTree3Texture;
    coconutTree3Texture.name = ElementName::COCONUT_TREE_3;
    coconutTree3Texture.path = "../assets/textures/decorations/coconut_tree_3.png";
    coconutTree3Texture.type = ElementTextureType::STATIC;    coconutTree3Texture.anchorPoint = AnchorPoint::BOTTOM_CENTER; // Tree grows from ground up, so anchor at bottom
    coconutTree3Texture.anchorOffsetX = 0.3f; // No offset
    coconutTree3Texture.anchorOffsetY = 0.4f; // No offset
    coconutTree3Texture.hasCollision = true;  // Enable collision for this tree
    coconutTree3Texture.collisionShapePoints = {
        {-0.07f, 0.0f}, {-0.07f, 0.1}, {0.07f, 0.1f}, {0.07f, 0.0f}
    };
    textures.push_back(coconutTree3Texture);
      // Sprite sheet texture for character
    ElementInfo characterTexture;
    characterTexture.name = ElementName::CHARACTER1;
    characterTexture.path = "../assets/textures/entities/player.png";
    characterTexture.type = ElementTextureType::SPRITESHEET;
    characterTexture.spriteWidth = 32;  // Assuming 32px width for each sprite frame
    characterTexture.spriteHeight = 48; // Assuming 48px height for each sprite frame
    characterTexture.anchorPoint = AnchorPoint::BOTTOM_CENTER; // Default anchor - can be changed in config
    characterTexture.anchorOffsetY = 0.2f; // Y offset
    characterTexture.hasCollision = false; // Enable collision for player
    textures.push_back(characterTexture);

    ElementInfo pirateMan;
    pirateMan.name = ElementName::PIRATE_MAN;
    pirateMan.path = "../assets/textures/entities/pirateMan.png";
    pirateMan.type = ElementTextureType::SPRITESHEET;
    pirateMan.spriteWidth = 32;  // Assuming 32px width for each sprite frame
    pirateMan.spriteHeight = 48; // Assuming 48px height for each sprite frame
    pirateMan.anchorPoint = AnchorPoint::BOTTOM_CENTER; // Default anchor - can be changed in config
    pirateMan.anchorOffsetY = 0.2f; // Y offset
    pirateMan.hasCollision = false; // Enable collision for player
    textures.push_back(pirateMan);

    ElementInfo pirateWoman;
    pirateWoman.name = ElementName::PIRATE_WOMAN;
    pirateWoman.path = "../assets/textures/entities/pirateWoman.png";
    pirateWoman.type = ElementTextureType::SPRITESHEET;
    pirateWoman.spriteWidth = 32;  // Assuming 32px width for each sprite frame
    pirateWoman.spriteHeight = 48; // Assuming 48px height for each sprite frame
    pirateWoman.anchorPoint = AnchorPoint::BOTTOM_CENTER; // Default anchor - can be changed in config
    pirateWoman.anchorOffsetY = 0.2f; // Y offset
    pirateWoman.hasCollision = false; // Enable collision for player
    textures.push_back(pirateWoman);

    ElementInfo shark;
    shark.name = ElementName::SHARK;
    shark.path = "../assets/textures/entities/shark.png";
    shark.type = ElementTextureType::SPRITESHEET;
    shark.spriteWidth = 148;  // Assuming 64px width for each sprite frame
    shark.spriteHeight = 141; // Assuming 64px height for each sprite frame
    shark.anchorPoint = AnchorPoint::CENTER; // Default anchor - can be changed in config
    shark.anchorOffsetY = 0.2f; // Y offset
    shark.hasCollision = false; // Enable collision for player
    textures.push_back(shark);

    ElementInfo giraffe;
    giraffe.name = ElementName::GIRAFFE;
    giraffe.path = "../assets/textures/entities/giraffe.png";
    giraffe.type = ElementTextureType::SPRITESHEET;
    giraffe.spriteWidth = 55;  // Assuming 64px width for each sprite frame
    giraffe.spriteHeight = 78; // Assuming 64px height for each sprite frame
    giraffe.anchorPoint = AnchorPoint::BOTTOM_CENTER; // Default anchor - can be changed in config
    giraffe.anchorOffsetY = 0.2f; // Y offset
    giraffe.hasCollision = false; // Enable collision for player
    textures.push_back(giraffe);

    ElementInfo armadillo;
    armadillo.name = ElementName::ARMADILLO;
    armadillo.path = "../assets/textures/entities/armadillo.png";
    armadillo.type = ElementTextureType::SPRITESHEET;
    armadillo.spriteWidth = 48;  // Assuming 64px width for each sprite frame
    armadillo.spriteHeight = 48; // Assuming 64px height for each sprite frame
    armadillo.anchorPoint = AnchorPoint::BOTTOM_CENTER; // Default anchor - can be changed in config
    armadillo.anchorOffsetY = 0.2f; // Y offset
    armadillo.hasCollision = false; // Enable collision for player
    textures.push_back(armadillo);

    ElementInfo coconut;
    coconut.name = ElementName::COCONUT;
    coconut.path = "../assets/textures/items/coconut_1.png";
    coconut.type = ElementTextureType::STATIC;
    coconut.anchorPoint = AnchorPoint::CENTER; // Default anchor - can be changed in config
    coconut.anchorOffsetX = 0.0f; // No offset
    coconut.anchorOffsetY = 0.0f; // No offset
    coconut.hasCollision = true;  // Enable collision for this item
    coconut.collisionShapePoints = {
        {-0.05f, 0.0f}, {-0.05f, 0.1f}, {0.05f, 0.1f}, {0.05f, 0.0f}
    };  
    textures.push_back(coconut);
    
    // Add more texture definitions here as needed
    
    return textures;
}

// Create textures vector using the function
static const std::vector<ElementInfo> elementTexturesToLoad = createElementTexturesToLoad();

ElementsOnMap::ElementsOnMap() {
    // Constructor - elementIndexMap initialized implicitly
}

ElementsOnMap::~ElementsOnMap() {
    // Clean up all loaded textures
    for (const auto& pair : textureIDs) {
        if (pair.second > 0) {
            glDeleteTextures(1, &pair.second);
        }
    }
    textureIDs.clear();
    textureDimensions.clear();
    
    // Elements vector will be cleared automatically
}

bool ElementsOnMap::init(glbasimac::GLBI_Engine& engine) {
    bool allLoaded = true;
    
    for (auto texInfo : elementTexturesToLoad) { // Copy to modify
        // Create a mutable copy of the texture info
        ElementInfo textureDetails = texInfo;
        
        // Load the texture and get dimensions
        int width, height;
        GLuint textureID = loadTexture(texInfo.path);
        
        if (textureID > 0) {
            // Store the texture ID and dimensions in our local copy
            textureDetails.textureID = textureID;
            
            // Get the dimensions from our texture dimensions map
            auto dimIt = textureDimensions.find(texInfo.name);
            if (dimIt != textureDimensions.end()) {
                textureDetails.totalWidth = dimIt->second.first;
                textureDetails.totalHeight = dimIt->second.second;
                  std::cout << "Loaded element texture: " << texInfo.path 
                          << " (ID: " << textureID 
                          << ", Dimensions: " << textureDetails.totalWidth 
                          << "x" << textureDetails.totalHeight << ")" << std::endl;
                
                // For sprite sheets, calculate number of frames per row
                if (textureDetails.type == ElementTextureType::SPRITESHEET && 
                    textureDetails.spriteWidth > 0 && 
                    textureDetails.spriteHeight > 0) {
                    
                    int framesPerRow = textureDetails.totalWidth / textureDetails.spriteWidth;
                    int numRows = textureDetails.totalHeight / textureDetails.spriteHeight;
                    
                    std::cout << "Sprite sheet details for " << static_cast<int>(texInfo.name) << ": " 
                              << framesPerRow << " frames per row, " 
                              << numRows << " rows, "
                              << "spriteWidth=" << textureDetails.spriteWidth << ", "
                              << "spriteHeight=" << textureDetails.spriteHeight << std::endl;
                }
            }
              // Store in the textureIDs map for backward compatibility
            textureIDs[texInfo.name] = textureID;
        } else {
if (DEBUG_LOGS) { std::cerr << "Failed to load element texture: " << texInfo.path << std::endl; }
            allLoaded = false;
        }
    }
    
    return allLoaded;
}

GLuint ElementsOnMap::loadTexture(const std::string& path) {
    // Match the same stbi_flip setting used in map.cpp to ensure consistency
    // In map.cpp, it's set to true, so we'll do the same here
    stbi_set_flip_vertically_on_load(true); // Match map.cpp setting
    
    // Load image using stb_image
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
if (DEBUG_LOGS) { std::cerr << "Failed to load texture: " << path << " (" << stbi_failure_reason() << ")" << std::endl; }
        return 0;
    }
    
    // Create and bind an OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters for pixel art (GL_NEAREST)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Upload the texture data
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    // Store dimensions for later use (aspect ratio calculation)
    ElementName currentBlockName = ElementName::COCONUT_TREE_1; // Default
    
    // Find which texture we're currently loading
    for (const auto& texInfo : elementTexturesToLoad) {
        if (texInfo.path == path) {
            currentBlockName = texInfo.name;
            break;
        }
    }
    
    textureDimensions[currentBlockName] = std::make_pair(width, height);
    
    // Free the image data
    stbi_image_free(data);
    
    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureID;
}

void ElementsOnMap::placeElement(const std::string& instanceName, ElementName elementName, 
                               float scale, float x, float y, float rotation,
                               int spriteSheetPhase, int spriteSheetFrame,
                               bool isAnimated, float animationSpeed,
                               AnchorPoint anchorPoint, float anchorOffsetX, float anchorOffsetY) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Check if an element with this name already exists by searching directly
    // This is more reliable than using elementIndexMap which becomes stale after sorting
    auto existingIt = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (existingIt != elements.end()) {
        if (DEBUG_LOGS) {
            std::cerr << "WARNING: Element with name '" << instanceName << "' already exists" << std::endl;
            std::cerr << "  Details: position=(" << existingIt->x << "," << existingIt->y 
                      << "), texture=" << static_cast<int>(existingIt->elementName) << std::endl;
            std::cerr << "To modify the existing element, use functions like changeElementCoordinates() instead." << std::endl;
        }
        return;
    }// Create a PlacedElement explicitly instead of using initializer list (C++11 compatibility)
    PlacedElement element;
    element.instanceName = instanceName;
    element.elementName = elementName;
    element.scale = scale;    element.x = x;
    element.y = y;
    element.rotation = rotation;
    
    // Handle anchor point - check if we need to use the texture's default
    if (anchorPoint == AnchorPoint::USE_TEXTURE_DEFAULT) {
        // Look up the texture\'s default anchor point
        bool found = false;        for (const auto& texInfo : elementTexturesToLoad) {
            if (texInfo.name == elementName) {
                element.anchorPoint = texInfo.anchorPoint;
                element.anchorOffsetX = texInfo.anchorOffsetX + anchorOffsetX; // Add custom offset to default
                element.anchorOffsetY = texInfo.anchorOffsetY + anchorOffsetY; // Add custom offset to default
                
                // Copy collision properties from the texture definition
                element.hasCollision = texInfo.hasCollision;
                element.collisionShapePoints = texInfo.collisionShapePoints; // New line
                
                found = true;
                break;
            }
        }
        // If texture not found, use CENTER as fallback
        if (!found) {
            element.anchorPoint = AnchorPoint::CENTER;
            element.anchorOffsetX = anchorOffsetX;
            element.anchorOffsetY = anchorOffsetY;
        }
    } else {
        // Use the explicitly specified anchor point
        element.anchorPoint = anchorPoint;
        element.anchorOffsetX = anchorOffsetX;
        element.anchorOffsetY = anchorOffsetY;
    }
    
    // Set spritesheet animation properties
    element.spriteSheetPhase = spriteSheetPhase;
    element.spriteSheetFrame = spriteSheetFrame;
    element.isAnimated = isAnimated;
    element.animationSpeed = animationSpeed;
    element.currentFrameTime = 0.0f;
    
    // Calculate the number of frames in the current phase
    // First, find the texture info
    bool isSpritesheet = false;
    for (const auto& texInfo : elementTexturesToLoad) {
        if (texInfo.name == elementName) {
            if (texInfo.type == ElementTextureType::SPRITESHEET && 
                texInfo.spriteWidth > 0 && 
                textureDimensions.find(elementName) != textureDimensions.end()) {
                
                auto texDim = textureDimensions[elementName];
                int totalWidth = texDim.first;
                element.numFramesInPhase = totalWidth / texInfo.spriteWidth;
                isSpritesheet = true;
            }
            break;
        }
    }
    
    // Add element to vector and update the index map
    size_t newIndex = elements.size();
    elements.push_back(element);
    elementIndexMap[instanceName] = newIndex;
    
    std::cout << "Placed element: " << instanceName << " (Texture: " 
              << static_cast<int>(elementName) << ") at (" << x << ", " << y 
              << ") with scale " << scale;
              
    if (isSpritesheet) {
        std::cout << ", phase: " << spriteSheetPhase 
                  << ", frame: " << spriteSheetFrame
                  << ", animated: " << (isAnimated ? "yes" : "no")
                  << ", frames in phase: " << element.numFramesInPhase;
    }
    
if (DEBUG_LOGS) { std::cout << std::endl; }
}

bool ElementsOnMap::changeElementCoordinates(const std::string& instanceName, float newX, float newY, float newRotation) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for moving: " << instanceName << std::endl; }
        return false;
    }
    
    // Update position
    it->x = newX;
    it->y = newY;
    
    // Update rotation if provided (a value of -1.0f means keep the existing rotation)
    if (newRotation >= 0.0f) {
        it->rotation = newRotation;
    }
    
if (DEBUG_LOGS) { std::cout << "Moved element: " << instanceName << " to (" << newX << ", " << newY << ")" << std::endl; }
    return true;
}

bool ElementsOnMap::moveElement(const std::string& instanceName, float deltaX, float deltaY) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // First, find the element by name directly (not using the index map)
    // This fixes the issue with elements being sorted in drawElements
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
    
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for relative movement: " << instanceName << std::endl; }
        // List available elements to help debug
if (DEBUG_LOGS) { std::cout << "Available elements:" << std::endl; }
        for (const auto& elem : elements) {
if (DEBUG_LOGS) { std::cout << "  - " << elem.instanceName << " at (" << elem.x << ", " << elem.y << ")" << std::endl; }
        }
        return false;
    }
    
    // Get current position
    float currentX = it->x;
    float currentY = it->y;
    
    // Update position by adding the deltas
    it->x = currentX + deltaX;
    it->y = currentY + deltaY;
    
    std::cout << "Moved element: " << instanceName 
              << " from (" << currentX << ", " << currentY << ")"
              << " to (" << it->x << ", " << it->y << ")"
              << " (delta: " << deltaX << ", " << deltaY << ")" << std::endl;
    return true;
    
if (DEBUG_LOGS) { std::cerr << "Element index out of range for relative movement: " << instanceName << std::endl; }
    return false;
}

bool ElementsOnMap::getElementPosition(const std::string& instanceName, float& x, float& y) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find the element by name directly instead of using the index map
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for position query: " << instanceName << std::endl; }
        return false;
    }
    
    // Return position through reference parameters
    x = it->x;
    y = it->y;
    return true;
}

const PlacedElement* ElementsOnMap::getElementData(const std::string& instanceName) const {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find the element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
        return nullptr;
    }
    
    return &(*it);
}

bool ElementsOnMap::elementExists(const std::string& instanceName) const {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find the element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    return it != elements.end();
}

bool ElementsOnMap::changeElementScale(const std::string& instanceName, float newScale) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for scaling: " << instanceName << std::endl; }
        return false;
    }
    
    // Calculate anchor point offset based on the element's anchor point
    // Get texture info to determine default anchor point if needed
    AnchorPoint anchorPoint = it->anchorPoint;
    if (anchorPoint == AnchorPoint::USE_TEXTURE_DEFAULT) {
        // Look up the default anchor point from the texture definition
        for (const auto& texInfo : elementTexturesToLoad) {
            if (texInfo.name == it->elementName) {
                anchorPoint = texInfo.anchorPoint;
                break;
            }
        }
    }
    
    // Store original scale for calculation
    float oldScale = it->scale;
    
    // Calculate scale offsets based on the anchor point type and the change in scale
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    
    // Calculate the difference between old and new dimensions
    float widthDiff = (newScale - oldScale) * 0.5f;
    float heightDiff = widthDiff;
    
    // Only calculate offsets if scale is changing
    if (newScale != oldScale && oldScale != 0.0f) { // Prevent division by zero
        switch(anchorPoint) {
            case AnchorPoint::CENTER:
                // No offset needed for center anchor
                break;
            case AnchorPoint::TOP_LEFT_CORNER:
                offsetX = -widthDiff;
                offsetY = heightDiff;
                break;
            case AnchorPoint::TOP_RIGHT_CORNER:
                offsetX = widthDiff;
                offsetY = heightDiff;
                break;
            case AnchorPoint::BOTTOM_LEFT_CORNER:
                offsetX = -widthDiff;
                offsetY = -heightDiff;
                break;
            case AnchorPoint::BOTTOM_RIGHT_CORNER:
                offsetX = widthDiff;
                offsetY = -heightDiff;
                break;
            case AnchorPoint::BOTTOM_CENTER:
                offsetX = 0.0f; // Centered horizontally
                offsetY = -heightDiff; // Bottom aligned
                break;
            default:
                break;
        }
    }
    
    // Update the element's scale and scale offsets
    it->scale = newScale;
    it->scaleOffsetX = offsetX;
    it->scaleOffsetY = offsetY;
    
    std::cout << "Changed element scale: " << instanceName << " to " << newScale 
              << " with scale offsets (" << offsetX << ", " << offsetY << ")" << std::endl;
    return true;
}

bool ElementsOnMap::changeElementRotation(const std::string& instanceName, float newRotation) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for rotation: " << instanceName << std::endl; }
        return false;
    }
    
    it->rotation = newRotation;
if (DEBUG_LOGS) { std::cout << "Changed element rotation: " << instanceName << " to " << newRotation << " degrees" << std::endl; }
    return true;
}

bool ElementsOnMap::changeElementSpriteFrame(const std::string& instanceName, int newFrame) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for changing sprite frame: " << instanceName << std::endl; }
        return false;
    }
    
    // Ensure frame is within valid range
    if (it->numFramesInPhase > 0) {
        it->spriteSheetFrame = newFrame % it->numFramesInPhase;
if (DEBUG_LOGS) { std::cout << "Changed element sprite frame: " << instanceName << " to " << it->spriteSheetFrame << std::endl; }
        return true;
    } else {
if (DEBUG_LOGS) { std::cerr << "Element doesn't support sprite frames: " << instanceName << std::endl; }
        return false;
    }
}

bool ElementsOnMap::changeElementSpritePhase(const std::string& instanceName, int newPhase) {
    std::lock_guard<std::mutex> lock(elementsMutex);

    std::cout << "Changing sprite phase for element: " << instanceName << std::endl;
    
    // Find element by name directly to handle elements being sorted in drawElements
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for changing sprite phase: " << instanceName << std::endl; }
        return false;
    }
    
    // Look up texture info to check if it's a spritesheet
    for (const auto& texInfo : elementTexturesToLoad) {
        if (texInfo.name == it->elementName) {
            if (texInfo.type == ElementTextureType::SPRITESHEET && 
                textureDimensions.find(it->elementName) != textureDimensions.end()) {
                
                auto dims = textureDimensions[it->elementName];
                int totalHeight = dims.second;
                int numPhases = totalHeight / texInfo.spriteHeight;
                  // Check if phase is valid
                if (newPhase >= 0 && newPhase < numPhases) {
                    it->spriteSheetPhase = newPhase;
                    std::cout << "Changing sprite phase for element: " << instanceName << " to " << newPhase << std::endl;
                    return true;
                } else {
                    std::cerr << "Invalid sprite phase " << newPhase << " for element: " << instanceName 
                            << " (valid range: 0-" << (numPhases - 1) << ")" << std::endl;
                    return false;
                }
            } else {
if (DEBUG_LOGS) { std::cerr << "Element doesn't support sprite phases: " << instanceName << std::endl; }
                return false;
            }
        }
    }
    
if (DEBUG_LOGS) { std::cerr << "Couldn't find texture info for element: " << instanceName << std::endl; }
    return false;
}

bool ElementsOnMap::changeElementAnimationStatus(const std::string& instanceName, bool isAnimated) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for changing animation status: " << instanceName << std::endl; }
        return false;
    }
    
    it->isAnimated = isAnimated;
    std::cout << "Changed element animation status: " << instanceName 
            << " to " << (isAnimated ? "animated" : "static") << std::endl;
    return true;
}

bool ElementsOnMap::changeElementAnimationSpeed(const std::string& instanceName, float newSpeed) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for changing animation speed: " << instanceName << std::endl; }
        return false;
    }
    
    if (newSpeed >= 0.0f) {
        it->animationSpeed = newSpeed;
if (DEBUG_LOGS) { std::cout << "Changed element animation speed: " << instanceName << " to " << newSpeed << " FPS" << std::endl; }
        return true;
    } else {
if (DEBUG_LOGS) { std::cerr << "Invalid animation speed (must be non-negative): " << newSpeed << std::endl; }
        return false;
    }
}

int ElementsOnMap::getElementSpritePhase(const std::string& instanceName) const {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
if (DEBUG_LOGS) { std::cerr << "Element not found for getting sprite phase: " << instanceName << std::endl; }
        return -1; // Return -1 to indicate element not found
    }
    
    return it->spriteSheetPhase;
}

void ElementsOnMap::drawElements(float startX, float endX, float startY, float endY, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop, double deltaTime) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    if (elements.empty()) {
        return;
    }
    
    // Calculate the visible area size for camera view
    float viewWidth = cameraRight - cameraLeft;
    float viewHeight = cameraTop - cameraBottom;
    
    // Save current OpenGL state
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    GLint blendSrcFactor, blendDstFactor;
    glGetIntegerv(GL_BLEND_SRC, &blendSrcFactor);
    glGetIntegerv(GL_BLEND_DST, &blendDstFactor);
    
    // Enable blending for transparent textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);    // Sort elements by Y-coordinate (descending) to draw from back to front
    // Elements with larger Y (visually higher on screen) are drawn first (behind)
    // This means elements with smaller Y (lower on screen) will be drawn on top
    std::sort(elements.begin(), elements.end(), [](const PlacedElement& a, const PlacedElement& b) {
        return a.y > b.y;
    });
      // Calculate the grid cell dimensions in screen coordinates
    float cellWidth = (endX - startX) / viewWidth;
    float cellHeight = (endY - startY) / viewHeight;
    
    // Since the grid is already maintained as a square in main.cpp (through offsetX/offsetY),
    // we know that cellWidth and cellHeight should represent the same physical size.
    // In a perfect square grid, they would be exactly equal.
    
    // Direct OpenGL drawing (bypassing GLBI_Engine's texture limitations)
    for (auto& element : elements) { // Changed to non-const to update animation state
        // Get the texture ID
        auto it = textureIDs.find(element.elementName);
        if (it == textureIDs.end()) {
if (DEBUG_LOGS) { std::cerr << "Texture not found for element: " << element.instanceName << std::endl; }
            continue;
        }
        
        GLuint textureID = it->second;
        
        // Get the texture info to check if it's a spritesheet
        bool isSpritesheet = false;
        int spriteWidth = 0;
        int spriteHeight = 0;
        int textureWidth = 0;
        int textureHeight = 0;
        
        // Look up element texture info
        for (const auto& texInfo : elementTexturesToLoad) {
            if (texInfo.name == element.elementName) {
                isSpritesheet = (texInfo.type == ElementTextureType::SPRITESHEET);
                spriteWidth = texInfo.spriteWidth;
                spriteHeight = texInfo.spriteHeight;
                break;
            }
        }
        
        // Get total texture dimensions
        if (textureDimensions.find(element.elementName) != textureDimensions.end()) {
            auto dims = textureDimensions[element.elementName];
            textureWidth = dims.first;
            textureHeight = dims.second;
        }
          // Update animation frame if this is an animated spritesheet element
        if (isSpritesheet && element.isAnimated && element.numFramesInPhase > 0) {
            // Calculate frame time based on animation speed
            element.currentFrameTime += static_cast<float>(deltaTime);
            float frameTime = 1.0f / element.animationSpeed; // Time per frame in seconds
            
            if (element.currentFrameTime >= frameTime) {
                // Advance to next frame
                int advanceFrames = static_cast<int>(element.currentFrameTime / frameTime);
                element.spriteSheetFrame = (element.spriteSheetFrame + advanceFrames) % element.numFramesInPhase;
                element.currentFrameTime = fmod(element.currentFrameTime, frameTime); // Keep remainder
                  // Debug animation info disabled for normal gameplay
                // Uncomment if you need to debug animation frames
                /*
                std::cout << "Animating " << element.instanceName 
                          << ": phase=" << element.spriteSheetPhase
                          << ", frame=" << element.spriteSheetFrame 
                          << ", numFrames=" << element.numFramesInPhase
                          << ", deltaTime=" << deltaTime << std::endl;
                */
            }
        }        // Skip elements that are outside the camera view
        if (element.x < cameraLeft - element.scale || element.x > cameraRight + element.scale || 
            element.y < cameraBottom - element.scale || element.y > cameraTop + element.scale) {
            continue;
        }
        
        // Calculate the element's position by mapping from world coordinates to screen coordinates
        float normalizedX = (element.x - cameraLeft) / viewWidth;
        float normalizedY = (element.y - cameraBottom) / viewHeight;
        
        // Map from normalized [0,1] to screen coordinates
        float gridX = startX + normalizedX * (endX - startX);
        float gridY = startY + normalizedY * (endY - startY);
          // Apply scale offsets to maintain anchor position when scaling - scaled to camera view
        gridX += (element.scaleOffsetX / viewWidth) * (endX - startX);
        gridY += (element.scaleOffsetY / viewHeight) * (endY - startY);// Debug can be enabled below if needed for future troubleshooting
        // Currently disabled to prevent console spam
        /*
        static int debugCounter = 0;
        if (element.instanceName.find("terrain_bush_") != std::string::npos && debugCounter++ % 1000 == 0) {
            std::cout << "Drawing " << element.instanceName << " - World coords: ("
                      << element.x << ", " << element.y << "), Grid cell: ("
                      << static_cast<int>(element.x) << ", " << static_cast<int>(element.y) << ")" << std::endl;
        }
        */
          // Calculate UV coordinates based on whether this is a spritesheet
        float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
        float aspectRatio = 1.0f; // Default aspect ratio
        
        if (isSpritesheet && spriteWidth > 0 && spriteHeight > 0 && textureWidth > 0 && textureHeight > 0) {
            // Calculate UV coordinates for the current frame in the sprite sheet
            float frameWidthRatio = static_cast<float>(spriteWidth) / textureWidth;
            float frameHeightRatio = static_cast<float>(spriteHeight) / textureHeight;
            
            // Store the aspect ratio for the sprite (height/width)
            aspectRatio = static_cast<float>(spriteHeight) / spriteWidth;
              
            // Calculate horizontal position (frame index)
            u0 = element.spriteSheetFrame * frameWidthRatio;
            u1 = u0 + frameWidthRatio;
            
            // Calculate vertical position (animation phase/row)
            // With stbi_set_flip_vertically_on_load(true), the image is already flipped
            // so we need to adjust our sprite sheet row calculation accordingly
            int numRows = textureHeight / spriteHeight;
            if (numRows <= 0) numRows = 1; // Ensure we don't divide by zero
            
            // Since stbi_set_flip_vertically_on_load(true) flips the image,
            // in OpenGL coordinates, row 0 is at the bottom
            v0 = element.spriteSheetPhase * frameHeightRatio;
            v1 = v0 + frameHeightRatio;  // Move up one row height
              // Debug output disabled for normal gameplay
            // Uncomment if you need to debug UV coordinates
            /*
            std::cout << "Element " << element.instanceName 
                      << " UV coords: (" << u0 << "," << v0 << ") to (" 
                      << u1 << "," << v1 << "), numRows=" << numRows 
                      << ", phase=" << element.spriteSheetPhase << std::endl;
            */
        }
          // Calculate element quad dimensions
        float halfWidth_ndc = (cellWidth * element.scale) / 2.0f;
        float halfHeight_ndc = (cellHeight * element.scale) / 2.0f;
        
        // Apply aspect ratio for sprite sheets to maintain correct proportions
        if (isSpritesheet) {
            halfHeight_ndc *= aspectRatio;
        }
        
        // Calculate anchor point offset based on the selected anchor point
        float anchorX = 0.0f;
        float anchorY = 0.0f;
          switch(element.anchorPoint) {
            case AnchorPoint::CENTER:
                // Default center anchor - no offset needed
                break;
            case AnchorPoint::TOP_LEFT_CORNER:
                anchorX = -halfWidth_ndc;
                anchorY = halfHeight_ndc;
                break;
            case AnchorPoint::TOP_RIGHT_CORNER:
                anchorX = halfWidth_ndc;
                anchorY = halfHeight_ndc;
                break;
            case AnchorPoint::BOTTOM_LEFT_CORNER:
                anchorX = -halfWidth_ndc;
                anchorY = -halfHeight_ndc;
                break;
            case AnchorPoint::BOTTOM_RIGHT_CORNER:
                anchorX = halfWidth_ndc;
                anchorY = -halfHeight_ndc;
                break;
            case AnchorPoint::BOTTOM_CENTER:
                anchorX = 0.0f; // Centered horizontally
                anchorY = -halfHeight_ndc; // Bottom aligned
                break;
        }
          // Apply additional anchor offsets (in screen space) - scaled to camera view
        float additionalOffsetX = (element.anchorOffsetX / viewWidth) * (endX - startX);
        float additionalOffsetY = (element.anchorOffsetY / viewHeight) * (endY - startY);
        anchorX += additionalOffsetX;
        anchorY += additionalOffsetY;
        
        // Bind texture
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Set color to white to preserve texture colors
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        
        // Save matrix state
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        
        // Translate to element position
        glTranslatef(gridX, gridY, 0.0f);
        
        // Rotate if needed
        if (element.rotation != 0.0f) {
            glRotatef(element.rotation, 0.0f, 0.0f, 1.0f);
        }
        
        // Apply anchor point offset (this effectively changes where the element is positioned)
        glTranslatef(-anchorX, -anchorY, 0.0f);        // Draw textured quad centered at origin using calculated UV coordinates
        // Use a consistent vertex order that matches map rendering
        // Map renders in clockwise order starting from top-left
        glBegin(GL_QUADS);
            // Normal rendering
            // Top-left, top-right, bottom-right, bottom-left (clockwise order)
            glTexCoord2f(u0, v1); glVertex2f(-halfWidth_ndc,  halfHeight_ndc);  // Top-left
            glTexCoord2f(u1, v1); glVertex2f( halfWidth_ndc,  halfHeight_ndc);  // Top-right
            glTexCoord2f(u1, v0); glVertex2f( halfWidth_ndc, -halfHeight_ndc);  // Bottom-right
            glTexCoord2f(u0, v0); glVertex2f(-halfWidth_ndc, -halfHeight_ndc);  // Bottom-left
        glEnd();
        
        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);        // Draw anchor point indicator if visualization is enabled
        if (showAnchorPoints) {
            // Calculate the actual position of the anchor point
            float anchorPosX = anchorX;
            float anchorPosY = anchorY;
            
            // Draw the anchor point using the debug function
            drawAnchorPoint(anchorPosX, anchorPosY);
        }
          // Draw collision box if visualization is enabled and this element has collision
        if (isShowingCollisionBoxes() && element.hasCollision) {
            // Scale the collision radius to match the grid cell dimensions

            // Draw the collision polygon centered on the anchor point
            // The anchor point is at (anchorX, anchorY) before the -anchorX, -anchorY translation,
            // so in the current coordinate system, we need to draw at (anchorX, anchorY)
            
            // Set color for collision box (e.g., red)
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f); // Red color

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            // Translate to the anchor point of the element where the collision shape should be centered
            glTranslatef(anchorX, anchorY, 0.0f);

            // Draw the polygon using GL_LINE_LOOP
            glBegin(GL_LINE_LOOP);
            if (!element.collisionShapePoints.empty()) {
                for (const auto& point : element.collisionShapePoints) {
                    // Scale points by element.scale first, then by cell dimensions
                    // to convert from local element units (defined in collisionShapePoints)
                    // to screen/grid units for drawing.
                    float polyX = (point.first * element.scale) * cellWidth; 
                    float polyY = (point.second * element.scale) * cellHeight; 
                    glVertex2f(polyX, polyY);
                }
            }
            glEnd();

            glPopMatrix();
            // Restore original color if needed (e.g., white for textures)
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
        
        // Restore matrix state
        glPopMatrix();
    }
    
    // Restore previous OpenGL state
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    } else {
        glBlendFunc(blendSrcFactor, blendDstFactor);
    }
}

void ElementsOnMap::listElements() const {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
if (DEBUG_LOGS) { std::cout << "=== Current Elements (" << elements.size() << " total) ===" << std::endl; }
if (DEBUG_LOGS) { std::cout << "Index  | Name              | Type      | Position (X,Y)" << std::endl; }
if (DEBUG_LOGS) { std::cout << "-------+-------------------+-----------+------------------" << std::endl; }
      for (size_t i = 0; i < elements.size(); ++i) {
        const auto& element = elements[i];
        
        // Use magic_enum to convert enum to string
        std::string typeName = elementNameToString(element.elementName);
        
        // Print element details
        printf("%-6zu | %-17s | %-9s | (%.2f, %.2f)\n", 
               i, 
               element.instanceName.c_str(), 
               typeName.c_str(),
               element.x, 
               element.y);
    }
if (DEBUG_LOGS) { std::cout << "===================================" << std::endl; }
}

void ElementsOnMap::printElementPositions() const {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Print a header for position information
if (DEBUG_LOGS) { std::cout << "\n===== Element Positions (" << elements.size() << " elements) =====" << std::endl; }
if (DEBUG_LOGS) { std::cout << "Name                | Type       | Position (X,Y)  | Scale | Rotation | Anchor" << std::endl; }
if (DEBUG_LOGS) { std::cout << "-------------------+------------+----------------+-------+----------+--------------" << std::endl; }
      for (const auto& element : elements) {
        // Use magic_enum to convert enum to string
        std::string typeName = elementNameToString(element.elementName);
        
        // Convert anchor point to string using magic_enum
        std::string anchorName = std::string(magic_enum::enum_name(element.anchorPoint));
        
        // Print element details with formatted column widths
        printf("%-19s | %-10s | (%6.2f,%6.2f) | %5.2f | %8.2f | %s\n", 
               element.instanceName.c_str(), 
               typeName.c_str(),
               element.x, element.y,
               element.scale,
               element.rotation,
               anchorName.c_str());
    }
    
    // Add offsets info when available
    bool hasOffsets = false;
    for (const auto& element : elements) {
        if (element.scaleOffsetX != 0.0f || element.scaleOffsetY != 0.0f ||
            element.anchorOffsetX != 0.0f || element.anchorOffsetY != 0.0f) {
            
            if (!hasOffsets) {
if (DEBUG_LOGS) { std::cout << "\n--- Elements with Offsets ---" << std::endl; }
if (DEBUG_LOGS) { std::cout << "Name                | Scale Offsets (X,Y) | Anchor Offsets (X,Y)" << std::endl; }
if (DEBUG_LOGS) { std::cout << "-------------------+-------------------+---------------------" << std::endl; }
                hasOffsets = true;
            }
            
            printf("%-19s | (%6.2f,%6.2f)     | (%6.2f,%6.2f)\n", 
                   element.instanceName.c_str(),
                   element.scaleOffsetX, element.scaleOffsetY,
                   element.anchorOffsetX, element.anchorOffsetY);
        }
    }
if (DEBUG_LOGS) { std::cout << "==========================================================" << std::endl; }
}

// Implementation of removeElement method to remove an element by its instance name
bool ElementsOnMap::removeElement(const std::string& instanceName) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    // Find the element by name directly in the elements vector
    // This approach is immune to sorting operations in drawElements()
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
    
    if (it == elements.end()) {
        // Element not found, return false
        if (DEBUG_LOGS) {
            std::cerr << "Element not found for removal: " << instanceName << std::endl;
        }
        return false;
    }
    
    // Remove the element from the elements vector
    elements.erase(it);
    
    // Remove from the index map if it exists (cleanup)
    elementIndexMap.erase(instanceName);
    
    if (DEBUG_LOGS) {
        std::cout << "Successfully removed element: " << instanceName << std::endl;
    }
    
    return true;
}

// Implementation of removeAllElementsByCategory method to remove elements by category prefix
int ElementsOnMap::removeAllElementsByCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(elementsMutex);
    
    int removedCount = 0;
    
    // Create a temporary list of element names to remove
    std::vector<std::string> elementsToRemove;
    
    // Identify elements that match the category prefix
    for (const auto& element : elements) {
        // Check if the element's instanceName starts with the category prefix
        // For terrain elements, they have names like "terrain_coconut_tree_X"
        if (element.instanceName.find(category) == 0) {
            elementsToRemove.push_back(element.instanceName);
        }
    }
    
    // Now remove each identified element
    for (const auto& elementName : elementsToRemove) {
        if (removeElement(elementName)) {
            removedCount++;
        }
    }
    
    // Log how many elements were removed
    if (removedCount > 0) {
        std::cout << "Removed " << removedCount << " elements with category prefix '" 
                  << category << "'" << std::endl;
    }
    
    return removedCount;
}
