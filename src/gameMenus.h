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
    OPTIONS_MENU,
    HEALTH_BAR,
    SCORE_DISPLAY,
    BUTTON_START,
    BUTTON_QUIT,
    LOGO
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
    
    UIElementInstance(UIElementName n, UIElementPosition pos, GLuint tex, int w, int h, float s)
        : name(n), position(pos), textureID(tex), width(w), height(h), scale(s), visible(true) {}
};

class GameMenus {
public:
    GameMenus();
    ~GameMenus();
    
    // Initialize the menu system and register UI elements
    bool initialize(glbasimac::GLBI_Engine& engine);
    
    // Register a new UI element type
    void registerUIElement(const UIElementInfo& elementInfo);
    
    // Place a UI element at a specific position (creates an instance)
    bool placeUIElement(UIElementName elementName, UIElementPosition position);
    
    // Remove a UI element instance
    void removeUIElement(UIElementName elementName);
    
    // Show/hide a specific UI element
    void showUIElement(UIElementName elementName, bool visible = true);
    void hideUIElement(UIElementName elementName);
    
    // Check if a UI element is visible
    bool isUIElementVisible(UIElementName elementName) const;
    
    // Render all visible UI elements
    void render();
    
    // Clear all UI elements
    void clearAllUIElements();
    
private:
    // Load a texture for a UI element
    bool loadUIElementTexture(const UIElementInfo& elementInfo, GLuint& textureID, int& width, int& height);
    
    // Calculate position coordinates for a UI element
    void calculateElementPosition(UIElementPosition position, int elementWidth, int elementHeight, 
                                float scale, float& x, float& y, float& width, float& height) const;
    
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
