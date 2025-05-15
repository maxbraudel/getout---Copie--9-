#include "elementsOnMap.h"
#include <iostream>
#include <algorithm> // Added for std::find_if

// Include stb_image without defining STB_IMAGE_IMPLEMENTATION
// (It should already be defined in map.cpp)
#include "../third_party/glbasimac/tools/stb_image.h"

// Global instance
ElementsOnMap elementsManager;

// Define textures to load 
static const std::vector<ElementTextureInfo> elementTexturesToLoad = {
    // Temporarily using an existing grass texture as a placeholder for the bush
    {ElementTextureName::BUSH, "C:\\Users\\famillebraudel\\Documents\\Developpement\\getout\\assets\\textures\\decorations\\bush.png"}
    // Add more texture definitions here as needed
};

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
}

bool ElementsOnMap::init(glbasimac::GLBI_Engine& engine) {
    bool allLoaded = true;
    
    for (const auto& texInfo : elementTexturesToLoad) {
        GLuint textureID = loadTexture(texInfo.path);
        if (textureID > 0) {
            textureIDs[texInfo.name] = textureID;
            std::cout << "Loaded element texture: " << texInfo.path << " (ID: " << textureID << ")" << std::endl;
        } else {
            std::cerr << "Failed to load element texture: " << texInfo.path << std::endl;
            allLoaded = false;
        }
    }
    
    return allLoaded;
}

GLuint ElementsOnMap::loadTexture(const std::string& path) {
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
                               float scale, float x, float y, float rotation) {
    // Check if an element with this name already exists
    auto mapIt = elementIndexMap.find(instanceName);
    if (mapIt != elementIndexMap.end()) {
        std::cerr << "Element with name '" << instanceName << "' already exists. Use moveElement to change position." << std::endl;
        return;
    }

    // Create a PlacedElement explicitly instead of using initializer list (C++11 compatibility)
    PlacedElement element;
    element.instanceName = instanceName;
    element.textureName = textureName;
    element.scale = scale;
    element.x = x;
    element.y = y;
    element.rotation = rotation;
    
    // Add element to vector and update the index map
    size_t newIndex = elements.size();
    elements.push_back(element);
    elementIndexMap[instanceName] = newIndex;
    
    std::cout << "Placed element: " << instanceName << " (Texture: " 
              << static_cast<int>(textureName) << ") at (" << x << ", " << y 
              << ") with scale " << scale << std::endl;
}

bool ElementsOnMap::removeElement(const std::string& instanceName) {
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

bool ElementsOnMap::moveElement(const std::string& instanceName, float newX, float newY, float newRotation) {
    // Find in index map first (O(1) lookup)
    auto mapIt = elementIndexMap.find(instanceName);
    if (mapIt == elementIndexMap.end()) {
        std::cerr << "Element not found for moving: " << instanceName << std::endl;
        return false;
    }
    
    size_t index = mapIt->second;
    if (index < elements.size()) {
        // Update position
        elements[index].x = newX;
        elements[index].y = newY;
        
        // Update rotation if provided (a value of -1.0f means keep the existing rotation)
        if (newRotation >= 0.0f) {
            elements[index].rotation = newRotation;
        }
        
        std::cout << "Moved element: " << instanceName << " to (" << newX << ", " << newY << ")" << std::endl;
        return true;
    }
    
    std::cerr << "Element index out of range for moving: " << instanceName << std::endl;
    return false;
}

void ElementsOnMap::drawElements(float startX, float endX, float startY, float endY, int gridSize) {
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
    for (const auto& element : elements) {
        // Get the texture ID
        auto it = textureIDs.find(element.textureName);
        if (it == textureIDs.end()) {
            std::cerr << "Texture not found for element: " << element.instanceName << std::endl;
            continue;
        }
        
        GLuint textureID = it->second;
          // Calculate the element's position based on its grid coordinates
        // We need to properly map the element's float coordinates to the visible grid
        float gridX = startX + (element.x / gridSize) * (endX - startX);
        float gridY = startY + (element.y / gridSize) * (endY - startY);
        
        // Get element dimensions
        float aspectRatio = 1.0f; // Default 1:1 aspect ratio
        
        // Look up texture dimensions if available (though not used for quad shape in this approach)
        if (textureDimensions.find(element.textureName) != textureDimensions.end()) {
            auto dims = textureDimensions[element.textureName];
            if (dims.first > 0 && dims.second > 0) {
                aspectRatio = static_cast<float>(dims.first) / dims.second;
            }
        }
        
        // Calculate element quad dimensions to be like map blocks.
        // The quad will have the same NDC shape as (element.scale) grid cells.
        // Since a grid cell (defined by cellWidth x cellHeight in NDC) already appears square on screen,
        // this element quad will also appear as a scaled square on screen.
        // The element's texture is mapped fully to this quad, potentially stretching/squashing it
        // if the texture's aspect ratio is not 1:1, similar to block textures.
        float halfWidth_ndc = (cellWidth * element.scale) / 2.0f;
        float halfHeight_ndc = (cellHeight * element.scale) / 2.0f;
        
        // Bind texture
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Set color to white to preserve texture colors
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        
        // Save matrix state
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
          // Translate to element center
        glTranslatef(gridX, gridY, 0.0f);
        
        // Rotate if needed
        if (element.rotation != 0.0f) {
            glRotatef(element.rotation, 0.0f, 0.0f, 1.0f);
        }
        
        // Draw textured quad centered at origin
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-halfWidth_ndc, -halfHeight_ndc);
            glTexCoord2f(1.0f, 0.0f); glVertex2f( halfWidth_ndc, -halfHeight_ndc);
            glTexCoord2f(1.0f, 1.0f); glVertex2f( halfWidth_ndc,  halfHeight_ndc);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-halfWidth_ndc,  halfHeight_ndc);
        glEnd();
        
        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        
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
