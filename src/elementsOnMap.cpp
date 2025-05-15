#include "elementsOnMap.h"
#include <iostream>
#include <algorithm> // Added for std::find_if

// Include stb_image without defining STB_IMAGE_IMPLEMENTATION
// (It should already be defined in map.cpp)
#include "../third_party/glbasimac/tools/stb_image.h"

// Global instance
ElementsOnMap elementsManager;

// Define textures to load - using C++11 compatible initialization syntax
static std::vector<ElementTextureInfo> createElementTexturesToLoad() {
    std::vector<ElementTextureInfo> textures;
    
    // Static texture for bush
    ElementTextureInfo testTexture;
    testTexture.name = ElementTextureName::TEST;
    testTexture.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\blocks\\grass.png";
    testTexture.type = ElementTextureType::STATIC;
    textures.push_back(testTexture);

    // Static texture for bush
    ElementTextureInfo bushTexture;
    bushTexture.name = ElementTextureName::BUSH;
    bushTexture.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\decorations\\bush.png";
    bushTexture.type = ElementTextureType::STATIC;
    textures.push_back(bushTexture);
    
    // Sprite sheet texture for character
    ElementTextureInfo characterTexture;
    characterTexture.name = ElementTextureName::CHARACTER1;
    characterTexture.path = "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\entities\\player.png";
    characterTexture.type = ElementTextureType::SPRITESHEET;
    characterTexture.spriteWidth = 32;  // Assuming 32px width for each sprite frame
    characterTexture.spriteHeight = 48; // Assuming 32px height for each sprite frame
    textures.push_back(characterTexture);
    
    // Add more texture definitions here as needed
    
    return textures;
}

// Create textures vector using the function
static const std::vector<ElementTextureInfo> elementTexturesToLoad = createElementTexturesToLoad();

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
        ElementTextureInfo textureDetails = texInfo;
        
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
            std::cerr << "Failed to load element texture: " << texInfo.path << std::endl;
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
        std::cerr << "Failed to load texture: " << path << " (" << stbi_failure_reason() << ")" << std::endl;
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
    ElementTextureName currentTextureName = ElementTextureName::BUSH; // Default
    
    // Find which texture we're currently loading
    for (const auto& texInfo : elementTexturesToLoad) {
        if (texInfo.path == path) {
            currentTextureName = texInfo.name;
            break;
        }
    }
    
    textureDimensions[currentTextureName] = std::make_pair(width, height);
    
    // Free the image data
    stbi_image_free(data);
    
    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureID;
}

void ElementsOnMap::placeElement(const std::string& instanceName, ElementTextureName textureName, 
                               float scale, float x, float y, float rotation,
                               int spriteSheetPhase, int spriteSheetFrame,
                               bool isAnimated, float animationSpeed,
                               AnchorPoint anchorPoint, float anchorOffsetX, float anchorOffsetY) {
    // Check if an element with this name already exists
    auto mapIt = elementIndexMap.find(instanceName);
    if (mapIt != elementIndexMap.end()) {
        std::cerr << "WARNING: Element with name '" << instanceName << "' already exists at index " << mapIt->second << std::endl;
        
        // Print detailed information about the existing element
        size_t existingIndex = mapIt->second;
        if (existingIndex < elements.size()) {
            const auto& existingElement = elements[existingIndex];
            std::cerr << "  Details: position=(" << existingElement.x << "," << existingElement.y 
                      << "), texture=" << static_cast<int>(existingElement.textureName) << std::endl;
        }
        
        std::cerr << "To modify the existing element, use functions like changeElementCoordinates() instead." << std::endl;
        return;
    }    // Create a PlacedElement explicitly instead of using initializer list (C++11 compatibility)
    PlacedElement element;
    element.instanceName = instanceName;
    element.textureName = textureName;
    element.scale = scale;
    element.x = x;
    element.y = y;
    element.rotation = rotation;
    
    // Set anchor point properties
    element.anchorPoint = anchorPoint;
    element.anchorOffsetX = anchorOffsetX;
    element.anchorOffsetY = anchorOffsetY;
    
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
        if (texInfo.name == textureName) {
            if (texInfo.type == ElementTextureType::SPRITESHEET && 
                texInfo.spriteWidth > 0 && 
                textureDimensions.find(textureName) != textureDimensions.end()) {
                
                auto texDim = textureDimensions[textureName];
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
              << static_cast<int>(textureName) << ") at (" << x << ", " << y 
              << ") with scale " << scale;
              
    if (isSpritesheet) {
        std::cout << ", phase: " << spriteSheetPhase 
                  << ", frame: " << spriteSheetFrame
                  << ", animated: " << (isAnimated ? "yes" : "no")
                  << ", frames in phase: " << element.numFramesInPhase;
    }
    
    std::cout << std::endl;
}

bool ElementsOnMap::rechangeElementCoordinates(const std::string& instanceName) {
    // Find in index map first (O(1) lookup)
    auto mapIt = elementIndexMap.find(instanceName);
    if (mapIt == elementIndexMap.end()) {
        std::cerr << "Element not found: " << instanceName << std::endl;
        return false;
    }
    
    size_t index = mapIt->second;
    // Erase the element from the vector
    if (index < elements.size()) {
        // When removing an element, we need to update the indices in our map
        elements.erase(elements.begin() + index);
        elementIndexMap.erase(mapIt); // Remove the element we just deleted
        
        // Update indices for all elements that were after the removed element
        for (auto& pair : elementIndexMap) {
            if (pair.second > index) {
                pair.second--; // Decrement index because the vector now has one less element
            }
        }
        
        std::cout << "Removed element: " << instanceName << std::endl;
        return true;
    }
    
    std::cerr << "Element index out of range: " << instanceName << std::endl;
    return false;
}

bool ElementsOnMap::changeElementCoordinates(const std::string& instanceName, float newX, float newY, float newRotation) {
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
        std::cerr << "Element not found for moving: " << instanceName << std::endl;
        return false;
    }
    
    // Update position
    it->x = newX;
    it->y = newY;
    
    // Update rotation if provided (a value of -1.0f means keep the existing rotation)
    if (newRotation >= 0.0f) {
        it->rotation = newRotation;
    }
    
    std::cout << "Moved element: " << instanceName << " to (" << newX << ", " << newY << ")" << std::endl;
    return true;
}

bool ElementsOnMap::moveElement(const std::string& instanceName, float deltaX, float deltaY) {
    // First, find the element by name directly (not using the index map)
    // This fixes the issue with elements being sorted in drawElements
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
    
    if (it == elements.end()) {
        std::cerr << "Element not found for relative movement: " << instanceName << std::endl;
        // List available elements to help debug
        std::cout << "Available elements:" << std::endl;
        for (const auto& elem : elements) {
            std::cout << "  - " << elem.instanceName << " at (" << elem.x << ", " << elem.y << ")" << std::endl;
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
    
    std::cerr << "Element index out of range for relative movement: " << instanceName << std::endl;
    return false;
}

bool ElementsOnMap::getElementPosition(const std::string& instanceName, float& x, float& y) {
    // Find the element by name directly instead of using the index map
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
        std::cerr << "Element not found for position query: " << instanceName << std::endl;
        return false;
    }
    
    // Return position through reference parameters
    x = it->x;
    y = it->y;
    return true;
}

bool ElementsOnMap::changeElementScale(const std::string& instanceName, float newScale) {
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
        std::cerr << "Element not found for scaling: " << instanceName << std::endl;
        return false;
    }
    
    it->scale = newScale;
    std::cout << "Changed element scale: " << instanceName << " to " << newScale << std::endl;
    return true;
}

bool ElementsOnMap::changeElementRotation(const std::string& instanceName, float newRotation) {
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
        std::cerr << "Element not found for rotation: " << instanceName << std::endl;
        return false;
    }
    
    it->rotation = newRotation;
    std::cout << "Changed element rotation: " << instanceName << " to " << newRotation << " degrees" << std::endl;
    return true;
}

bool ElementsOnMap::changeElementSpriteFrame(const std::string& instanceName, int newFrame) {
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
        std::cerr << "Element not found for changing sprite frame: " << instanceName << std::endl;
        return false;
    }
    
    // Ensure frame is within valid range
    if (it->numFramesInPhase > 0) {
        it->spriteSheetFrame = newFrame % it->numFramesInPhase;
        std::cout << "Changed element sprite frame: " << instanceName << " to " << it->spriteSheetFrame << std::endl;
        return true;
    } else {
        std::cerr << "Element doesn't support sprite frames: " << instanceName << std::endl;
        return false;
    }
}

bool ElementsOnMap::changeElementSpritePhase(const std::string& instanceName, int newPhase) {
    // Find element by name directly to handle elements being sorted in drawElements
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
        std::cerr << "Element not found for changing sprite phase: " << instanceName << std::endl;
        return false;
    }
    
    // Look up texture info to check if it's a spritesheet
    for (const auto& texInfo : elementTexturesToLoad) {
        if (texInfo.name == it->textureName) {
            if (texInfo.type == ElementTextureType::SPRITESHEET && 
                textureDimensions.find(it->textureName) != textureDimensions.end()) {
                
                auto dims = textureDimensions[it->textureName];
                int totalHeight = dims.second;
                int numPhases = totalHeight / texInfo.spriteHeight;
                
                // Check if phase is valid
                if (newPhase >= 0 && newPhase < numPhases) {
                    it->spriteSheetPhase = newPhase;
                    std::cout << "Changed element sprite phase: " << instanceName << " to " << newPhase << std::endl;
                    return true;
                } else {
                    std::cerr << "Invalid sprite phase " << newPhase << " for element: " << instanceName 
                            << " (valid range: 0-" << (numPhases - 1) << ")" << std::endl;
                    return false;
                }
            } else {
                std::cerr << "Element doesn't support sprite phases: " << instanceName << std::endl;
                return false;
            }
        }
    }
    
    std::cerr << "Couldn't find texture info for element: " << instanceName << std::endl;
    return false;
}

bool ElementsOnMap::changeElementAnimationStatus(const std::string& instanceName, bool isAnimated) {
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
        std::cerr << "Element not found for changing animation status: " << instanceName << std::endl;
        return false;
    }
    
    it->isAnimated = isAnimated;
    std::cout << "Changed element animation status: " << instanceName 
            << " to " << (isAnimated ? "animated" : "static") << std::endl;
    return true;
}

bool ElementsOnMap::changeElementAnimationSpeed(const std::string& instanceName, float newSpeed) {
    // Find element by name directly
    auto it = std::find_if(elements.begin(), elements.end(),
        [&instanceName](const PlacedElement& element) {
            return element.instanceName == instanceName;
        });
        
    if (it == elements.end()) {
        std::cerr << "Element not found for changing animation speed: " << instanceName << std::endl;
        return false;
    }
    
    if (newSpeed >= 0.0f) {
        it->animationSpeed = newSpeed;
        std::cout << "Changed element animation speed: " << instanceName << " to " << newSpeed << " FPS" << std::endl;
        return true;
    } else {
        std::cerr << "Invalid animation speed (must be non-negative): " << newSpeed << std::endl;
        return false;
    }
}

void ElementsOnMap::drawElements(float startX, float endX, float startY, float endY, int gridSize, double deltaTime) {
    if (elements.empty() || gridSize <= 0) {
        return;
    }
    
    // Save current OpenGL state
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    GLint blendSrcFactor, blendDstFactor;
    glGetIntegerv(GL_BLEND_SRC, &blendSrcFactor);
    glGetIntegerv(GL_BLEND_DST, &blendDstFactor);
    
    // Enable blending for transparent textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Sort elements by Y-coordinate (descending) to draw from back to front
    // Elements with larger Y (visually higher on screen) are drawn first (behind)
    std::sort(elements.begin(), elements.end(), [](const PlacedElement& a, const PlacedElement& b) {
        return a.y > b.y;
    });
    
    // Calculate the grid cell dimensions in NDC coordinates
    float cellWidth = (endX - startX) / gridSize;
    float cellHeight = (endY - startY) / gridSize;
    
    // Since the grid is already maintained as a square in main.cpp (through offsetX/offsetY),
    // we know that cellWidth and cellHeight should represent the same physical size.
    // In a perfect square grid, they would be exactly equal.
    
    // Direct OpenGL drawing (bypassing GLBI_Engine's texture limitations)
    for (auto& element : elements) { // Changed to non-const to update animation state
        // Get the texture ID
        auto it = textureIDs.find(element.textureName);
        if (it == textureIDs.end()) {
            std::cerr << "Texture not found for element: " << element.instanceName << std::endl;
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
            if (texInfo.name == element.textureName) {
                isSpritesheet = (texInfo.type == ElementTextureType::SPRITESHEET);
                spriteWidth = texInfo.spriteWidth;
                spriteHeight = texInfo.spriteHeight;
                break;
            }
        }
        
        // Get total texture dimensions
        if (textureDimensions.find(element.textureName) != textureDimensions.end()) {
            auto dims = textureDimensions[element.textureName];
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
        }
        
        // Calculate the element's position based on its grid coordinates
        float gridX = startX + (element.x / gridSize) * (endX - startX);
        float gridY = startY + (element.y / gridSize) * (endY - startY);
        
        // Calculate UV coordinates based on whether this is a spritesheet
        float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
        
        if (isSpritesheet && spriteWidth > 0 && spriteHeight > 0 && textureWidth > 0 && textureHeight > 0) {
            // Calculate UV coordinates for the current frame in the sprite sheet
            float frameWidthRatio = static_cast<float>(spriteWidth) / textureWidth;
            float frameHeightRatio = static_cast<float>(spriteHeight) / textureHeight;
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
        }
        
        // Apply additional anchor offsets (in NDC space)
        float additionalOffsetX = (element.anchorOffsetX / gridSize) * (endX - startX);
        float additionalOffsetY = (element.anchorOffsetY / gridSize) * (endY - startY);
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
        glTranslatef(-anchorX, -anchorY, 0.0f);
        
        // Draw textured quad centered at origin using calculated UV coordinates
        // Use a consistent vertex order that matches map rendering
        // Map renders in clockwise order starting from top-left
        glBegin(GL_QUADS);
            // Top-left, top-right, bottom-right, bottom-left (clockwise order)
            glTexCoord2f(u0, v1); glVertex2f(-halfWidth_ndc,  halfHeight_ndc);  // Top-left
            glTexCoord2f(u1, v1); glVertex2f( halfWidth_ndc,  halfHeight_ndc);  // Top-right
            glTexCoord2f(u1, v0); glVertex2f( halfWidth_ndc, -halfHeight_ndc);  // Bottom-right
            glTexCoord2f(u0, v0); glVertex2f(-halfWidth_ndc, -halfHeight_ndc);  // Bottom-left
        glEnd();
        
        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        
        // Draw anchor point indicator if visualization is enabled
        if (showAnchorPoints) {
            // Draw a small crosshair to indicate the anchor point
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);  // Red color
            glLineWidth(3.0f);
            
            // Draw horizontal line
            glBegin(GL_LINES);
            glVertex2f(-0.02f, 0.0f);
            glVertex2f(0.02f, 0.0f);
            glEnd();
            
            // Draw vertical line
            glBegin(GL_LINES);
            glVertex2f(0.0f, -0.02f);
            glVertex2f(0.0f, 0.02f);
            glEnd();
            
            glLineWidth(1.0f);  // Reset line width
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
    std::cout << "=== Current Elements (" << elements.size() << " total) ===" << std::endl;
    std::cout << "Index  | Name              | Type      | Position (X,Y)" << std::endl;
    std::cout << "-------+-------------------+-----------+------------------" << std::endl;
    
    for (size_t i = 0; i < elements.size(); ++i) {
        const auto& element = elements[i];
        std::string typeName;
        
        // Convert enum to string for display
        switch (element.textureName) {
            case ElementTextureName::BUSH:
                typeName = "BUSH";
                break;
            case ElementTextureName::CHARACTER1:
                typeName = "CHARACTER1";
                break;
            default:
                typeName = "UNKNOWN";
                break;
        }
        
        // Print element details
        printf("%-6zu | %-17s | %-9s | (%.2f, %.2f)\n", 
               i, 
               element.instanceName.c_str(), 
               typeName.c_str(),
               element.x, 
               element.y);
    }
    std::cout << "===================================" << std::endl;
}
