#pragma once

#define GLFW_INCLUDE_NONE
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glbasimac/glbi_engine.hpp"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include "enumDefinitions.h"
#include "collisionCache.h"


// Define an enum for texture types
enum class ElementTextureType {
    STATIC,
    SPRITESHEET
};

// Define an enum for anchor point positioning
enum class AnchorPoint {
    CENTER,              // Default - anchor at center of texture
    TOP_LEFT_CORNER,     // Anchor at top left
    TOP_RIGHT_CORNER,    // Anchor at top right
    BOTTOM_LEFT_CORNER,  // Anchor at bottom left
    BOTTOM_RIGHT_CORNER, // Anchor at bottom right
    BOTTOM_CENTER,       // Anchor at bottom center (useful for characters)
    USE_TEXTURE_DEFAULT  // Special value to use the default anchor point from texture configuration
};

// Struct to hold texture information
struct ElementInfo {
    ElementName name;
    std::string path;
    ElementTextureType type = ElementTextureType::STATIC; // Type of texture (static or spritesheet)
    int spriteWidth = 0;  // Width of a single sprite in a spritesheet
    int spriteHeight = 0; // Height of a single sprite in a spritesheet
    int totalWidth = 0;   // Total width of the texture (for calculating UV coordinates)
    int totalHeight = 0;  // Total height of the texture (for calculating UV coordinates)
    GLuint textureID = 0; // OpenGL texture handle
    AnchorPoint anchorPoint = AnchorPoint::CENTER; // Default anchor point for this texture
    float anchorOffsetX = 0.0f;                    // Default X offset from anchor point
    float anchorOffsetY = 0.0f;                    // Default Y offset from anchor point
    bool hasCollision = false;                     // Whether this element has collision detection
    std::vector<std::pair<float, float>> collisionShapePoints; // Points defining the collision polygon, relative to anchor
};

// Struct to hold placed element information
struct PlacedElement {
    std::string instanceName; // Unique name for this instance (e.g., "bush1")
    ElementName elementName;
    float scale;
    float x; // Grid-relative float coordinates (e.g., 0.5 for center of cell 0)
    float y;
    float rotation = 0.0f; // Optional: rotation angle in degrees
    
    // Anchor point properties
    AnchorPoint anchorPoint = AnchorPoint::CENTER; // Default to center
    float anchorOffsetX = 0.0f;                    // Additional X offset from anchor point
    float anchorOffsetY = 0.0f;                    // Additional Y offset from anchor point
      // Scale offset for maintaining anchor points when scaling
    float scaleOffsetX = 0.0f;  // X offset to maintain anchor when scaling
    float scaleOffsetY = 0.0f;  // Y offset to maintain anchor when scaling
    
    // Spritesheet animation properties
    int spriteSheetPhase = 0;     // Which animation row to use in the spritesheet (0-indexed)
    int spriteSheetFrame = 0;     // Current frame in the animation (0-indexed)
    bool isAnimated = false;      // Whether to automatically animate this element
    float animationSpeed = 10.0f; // Frames per second for animation
    float currentFrameTime = 0.0f; // Time accumulator for animation
    int numFramesInPhase = 0;     // Number of frames in current animation phase (calculated)
      // Collision properties
    bool hasCollision = false;    // Whether this element has collision detection
    std::vector<std::pair<float, float>> collisionShapePoints; // Points defining the collision polygon
    
    // Pre-calculated collision box for performance optimization
    mutable PreCalculatedCollisionBox cachedCollisionBox;
};

// Main class to handle elements on the map
class ElementsOnMap {
public:
    ElementsOnMap();
    ~ElementsOnMap();

    // Initialize the manager and load textures
    bool init(glbasimac::GLBI_Engine& engine);
      // Debug functions
    void listElements() const;
    
    // Place an element at the specified coordinates
    void placeElement(const std::string& instanceName, ElementName elementName, 
                      float scale, float x, float y, float rotation = 0.0f,
                      int spriteSheetPhase = 0, int spriteSheetFrame = 0,
                      bool isAnimated = false, float animationSpeed = 10.0f,
                      AnchorPoint anchorPoint = AnchorPoint::USE_TEXTURE_DEFAULT,
                      float anchorOffsetX = 0.0f, float anchorOffsetY = 0.0f);
      // Remove an element by its instance name
    bool removeElement(const std::string& instanceName);
    
    // Remove all elements with a specific category prefix in their instanceName
    int removeAllElementsByCategory(const std::string& category);
    
    // Move an existing element to a new position
    bool changeElementCoordinates(const std::string& instanceName, float newX, float newY, float newRotation = -1.0f);
      
    // Move an element relative to its current position
    bool moveElement(const std::string& instanceName, float deltaX, float deltaY);
      // Get element position
    bool getElementPosition(const std::string& instanceName, float& x, float& y);
    
    // Get element data by instance name
    const PlacedElement* getElementData(const std::string& instanceName) const;
    
    // Check if an element exists by instance name
    bool elementExists(const std::string& instanceName) const;
    
    // Change element scale
    bool changeElementScale(const std::string& instanceName, float newScale);
    
    // Change element rotation
    bool changeElementRotation(const std::string& instanceName, float newRotation);
    
    // Change sprite sheet frame
    bool changeElementSpriteFrame(const std::string& instanceName, int newFrame);
    
    // Change sprite sheet phase (row)
    bool changeElementSpritePhase(const std::string& instanceName, int newPhase);
    
    // Toggle element animation on/off
    bool changeElementAnimationStatus(const std::string& instanceName, bool isAnimated);
    
    // Change animation speed
    bool changeElementAnimationSpeed(const std::string& instanceName, float newSpeed);
    
    // Get current sprite phase of an element
    int getElementSpritePhase(const std::string& instanceName) const;

    // Draw all placed elements
    void drawElements(float startX, float endX, float startY, float endY, float cameraLeft, float cameraRight, float cameraBottom, float cameraTop, double deltaTime = 0.0);
    
    // Get texture dimensions for the specified texture
    std::pair<int, int> getTextureDimensions(ElementName elementName) const {
        auto it = textureDimensions.find(elementName);
        if (it != textureDimensions.end()) {
            return it->second;
        }
        return std::make_pair(0, 0); // Return zeros if texture not found
    }
      // Get the total number of elements
    size_t getElementsCount() const {
        std::lock_guard<std::mutex> lock(elementsMutex);
        return elements.size();
    }
      // Toggle debug visualization of anchor points
    void toggleAnchorPointVisualization() {
        showAnchorPoints = !showAnchorPoints;
        std::cout << "Anchor point visualization " << (showAnchorPoints ? "enabled" : "disabled") << std::endl;
    }
    
    // Check if anchor points are being visualized
    bool isShowingAnchorPoints() const {
        return showAnchorPoints;
    }
      // Print detailed position information for all placed elements
    void printElementPositions() const;
      // Get the elements vector (for collision detection)
    // Returns a copy to ensure thread safety
    std::vector<PlacedElement> getElements() const {
        std::lock_guard<std::mutex> lock(elementsMutex);
        return elements;
    }

private:
    // Modified texture loading that doesn't rely on activateTexturing
    GLuint loadTexture(const std::string& path);

    std::vector<PlacedElement> elements;
    std::map<ElementName, GLuint> textureIDs; // Direct OpenGL texture handles
    std::map<std::string, size_t> elementIndexMap; // Maps element name to index in elements vector
    
    // Store width and height for aspect ratio calculation
    std::map<ElementName, std::pair<int, int>> textureDimensions;

    // Debug visualization flag
    bool showAnchorPoints = false;
    
    // Mutex to protect concurrent access to elements vector
    mutable std::mutex elementsMutex;
};

// Global instance
extern ElementsOnMap elementsManager;
