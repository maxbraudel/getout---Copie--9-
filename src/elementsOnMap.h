#pragma once

#define GLFW_INCLUDE_NONE
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glbasimac/glbi_engine.hpp"
#include <string>
#include <vector>
#include <map>

// Define an enum for element texture types
enum class ElementTextureName {
    BUSH
    // Add more element texture types as needed
};

// Struct to hold texture information
struct ElementTextureInfo {
    ElementTextureName name;
    std::string path;
    // Add more fields as needed (e.g., animationFrames for animated elements)
};

// Struct to hold placed element information
struct PlacedElement {
    std::string instanceName; // Unique name for this instance (e.g., "bush1")
    ElementTextureName textureName;
    float scale;
    float x; // Grid-relative float coordinates (e.g., 0.5 for center of cell 0)
    float y;
    // Optional: rotation angle in degrees
    float rotation = 0.0f;
};

// Main class to handle elements on the map
class ElementsOnMap {
public:
    ElementsOnMap();
    ~ElementsOnMap();

    // Initialize the manager and load textures
    bool init(glbasimac::GLBI_Engine& engine);
    
    // Place an element at the specified coordinates
    void placeElement(const std::string& instanceName, ElementTextureName textureName, 
                      float scale, float x, float y, float rotation = 0.0f);
    
    // Remove an element by its instance name
    bool removeElement(const std::string& instanceName);
    
    // Draw all placed elements
    void drawElements(float startX, float endX, float startY, float endY, int gridSize);

private:
    // Modified texture loading that doesn't rely on activateTexturing
    GLuint loadTexture(const std::string& path);

    std::vector<PlacedElement> elements;
    std::map<ElementTextureName, GLuint> textureIDs; // Direct OpenGL texture handles
    
    // Store width and height for aspect ratio calculation
    std::map<ElementTextureName, std::pair<int, int>> textureDimensions;
};

// Global instance
extern ElementsOnMap elementsManager;
