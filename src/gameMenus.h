#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glbasimac/glbi_engine.hpp"
#include "glbasimac/glbi_texture.hpp"
#include <string>
#include <map>
#include <vector>

// UI Element names enum
enum class UIElementName {
    START_MENU,
    PAUSE_MENU,
    GAME_OVER,
    WIN_MENU,
    HEALTH_BAR,
    SCORE_DISPLAY,
    BUTTON_START,
    BUTTON_QUIT,
    COCONUTS,
    LOGO
};

// UI Element texture types enum
enum class UIElementTextureType {
    STATIC,
    SPRITESHEET
};

// UI Element positions enum
enum class UIElementPosition {
    TOP_LEFT_CORNER,
    TOP_RIGHT_CORNER,
    BOTTOM_LEFT_CORNER,
    BOTTOM_RIGHT_CORNER,
    CENTER
};

// UI Element information structure
struct UIElementInfo {
    UIElementName name;
    std::string texturePath;
    float scale;
    UIElementTextureType type = UIElementTextureType::STATIC; // Type of texture (static or spritesheet)
    int spriteWidth = 0;  // Width of a single sprite in a spritesheet
    int spriteHeight = 0; // Height of a single sprite in a spritesheet
    int defaultSpriteSheetPhase = 0;  // Default sprite sheet phase (row)
    int defaultSpriteSheetFrame = 0;  // Default sprite sheet frame (column)
    bool isAnimated = false;          // Whether the UI element should animate
    float animationSpeed = 10.0f;     // Animation speed in frames per second
    
    // Margin parameters for positioning offset
    float marginTop = 0.0f;           // Top margin offset in pixels
    float marginBottom = 0.0f;        // Bottom margin offset in pixels
    float marginLeft = 0.0f;          // Left margin offset in pixels
    float marginRight = 0.0f;         // Right margin offset in pixels
    
    // Default constructor for std::map usage
    UIElementInfo() : name(UIElementName::START_MENU), texturePath(""), scale(1.0f) {}
    
    UIElementInfo(UIElementName n, const std::string& path, float s = 1.0f) 
        : name(n), texturePath(path), scale(s) {}
};

// Active UI Element instance
struct UIElementInstance {
    UIElementName name;
    UIElementPosition position;
    GLuint textureID;
    int width;
    int height;
    float scale;
    bool visible;
    
    // Sprite sheet properties (for spritesheet textures)
    UIElementTextureType type;
    int spriteWidth;
    int spriteHeight;
    int totalWidth;              // Total texture width
    int totalHeight;             // Total texture height
    int spriteSheetPhase;        // Current phase (row) in sprite sheet
    int spriteSheetFrame;        // Current frame (column) in sprite sheet
    bool isAnimated;             // Whether this element is animated
    float animationSpeed;        // Animation speed in FPS
    float currentFrameTime;      // Time accumulator for animation
    int numFramesInPhase;        // Number of frames in current phase
    
    // Margin parameters for positioning offset
    float marginTop;             // Top margin offset in pixels
    float marginBottom;          // Bottom margin offset in pixels
    float marginLeft;            // Left margin offset in pixels
    float marginRight;           // Right margin offset in pixels
    
    UIElementInstance(UIElementName n, UIElementPosition pos, GLuint tex, int w, int h, float s)
        : name(n), position(pos), textureID(tex), width(w), height(h), scale(s), visible(true),
          type(UIElementTextureType::STATIC), spriteWidth(0), spriteHeight(0), 
          totalWidth(w), totalHeight(h), spriteSheetPhase(0), spriteSheetFrame(0),
          isAnimated(false), animationSpeed(10.0f), currentFrameTime(0.0f), numFramesInPhase(0),
          marginTop(0.0f), marginBottom(0.0f), marginLeft(0.0f), marginRight(0.0f) {}
};

class GameMenus {
public:
    GameMenus();
    ~GameMenus();
    
    // Initialize the menu system and load UI elements
    bool initialize(glbasimac::GLBI_Engine& engine);
    
    // Create the vector of UI elements to load (similar to elementsOnMap pattern)
    static std::vector<UIElementInfo> createUIElementsToLoad();
    
    // Place a UI element at a specific position (creates an instance)
    bool placeUIElement(UIElementName elementName, UIElementPosition position);
    
    // Remove a UI element instance
    void removeUIElement(UIElementName elementName);
    
    // Show/hide a specific UI element
    void showUIElement(UIElementName elementName, bool visible = true);
    void hideUIElement(UIElementName elementName);
      // Check if a UI element is visible
    bool isUIElementVisible(UIElementName elementName) const;
      // Sprite sheet manipulation functions
    bool changeUIElementSpriteSheetPhase(UIElementName elementName, int newPhase);
    bool changeUIElementSpriteSheetFrame(UIElementName elementName, int newFrame);
    bool changeUIElementAnimationStatus(UIElementName elementName, bool isAnimated);
    bool changeUIElementAnimationSpeed(UIElementName elementName, float newSpeed);
    
    // Health bar update function
    void updateHealthBar(int currentHealth);
    
    // Render all visible UI elements
    void render(double deltaTime = 0.0);
    
    // Clear all UI elements
    void clearAllUIElements();
    
private:
    // Load a texture for a UI element
    bool loadUIElementTexture(const UIElementInfo& elementInfo, GLuint& textureID, int& width, int& height);
      // Calculate position coordinates for a UI element (with margin support)
    void calculateElementPosition(UIElementPosition position, int elementWidth, int elementHeight, 
                                float scale, float marginTop, float marginBottom, 
                                float marginLeft, float marginRight,
                                float& x, float& y, float& width, float& height) const;
    
    // Render a single UI element
    void renderUIElement(const UIElementInstance& element) const;
    
    glbasimac::GLBI_Engine* m_enginePtr;
    std::map<UIElementName, UIElementInfo> m_registeredElements;
    std::vector<UIElementInstance> m_activeElements;
    
    // Screen dimensions (updated during render)
    mutable int m_screenWidth;
    mutable int m_screenHeight;
};

// Global instance of the menu system
extern GameMenus gameMenus;
