#include "gameMenus.h"
#include <iostream>
#include <string>
#include <algorithm>
#include "glbasimac/glbi_texture.hpp"
// Include stb_image without defining STB_IMAGE_IMPLEMENTATION
// This avoids duplicate symbols since it's already defined in map.cpp
#include "../third_party/glbasimac/tools/stb_image.h"

// Define UI elements to load - using C++11 compatible initialization syntax
static std::vector<UIElementInfo> createUIElementsToLoad() {
    std::vector<UIElementInfo> uiElements;
    
    // Start Menu
    UIElementInfo startMenu;
    startMenu.name = UIElementName::START_MENU;
    startMenu.texturePath = "../assets/textures/ui/startMenu.png";
    startMenu.scale = 1.0f;
    uiElements.push_back(startMenu);
    
    // Pause Menu  
    UIElementInfo pauseMenu;
    pauseMenu.name = UIElementName::PAUSE_MENU;
    pauseMenu.texturePath = "../assets/textures/ui/pauseMenu.png";
    pauseMenu.scale = 1.0f;
    uiElements.push_back(pauseMenu);
    
    // Game Over
    UIElementInfo gameOver;
    gameOver.name = UIElementName::GAME_OVER;
    gameOver.texturePath = "../assets/textures/ui/gameOver.png";
    gameOver.scale = 1.0f;
    uiElements.push_back(gameOver);
    
    // Options Menu
    UIElementInfo optionsMenu;
    optionsMenu.name = UIElementName::OPTIONS_MENU;
    optionsMenu.texturePath = "../assets/textures/ui/options.png";
    optionsMenu.scale = 1.0f;
    uiElements.push_back(optionsMenu);
    
    // Health Bar
    UIElementInfo healthBar;
    healthBar.name = UIElementName::HEALTH_BAR;
    healthBar.texturePath = "../assets/textures/ui/hearts.png";
    healthBar.scale = 5.0f;
    uiElements.push_back(healthBar);
    
    return uiElements;
}

// Create UI elements vector using the function
static const std::vector<UIElementInfo> uiElementsToLoad = createUIElementsToLoad();

// Global instance of the menu system
GameMenus gameMenus;

GameMenus::GameMenus() : m_enginePtr(nullptr), m_screenWidth(800), m_screenHeight(600) {
}

GameMenus::~GameMenus() {
    // Clean up textures from active elements
    for (const auto& element : m_activeElements) {
        glDeleteTextures(1, &element.textureID);
    }
    m_activeElements.clear();
}

std::vector<UIElementInfo> GameMenus::createUIElementsToLoad() {
    return uiElementsToLoad;
}

bool GameMenus::initialize(glbasimac::GLBI_Engine& engine) {
    m_enginePtr = &engine;
    
    // Load UI elements using vector-based pattern
    for (const auto& uiElementInfo : uiElementsToLoad) {
        // Register UI element in map
        m_registeredElements[uiElementInfo.name] = uiElementInfo;
        std::cout << "Registered UI element: " << static_cast<int>(uiElementInfo.name) << std::endl;
    }
    
    // Example: Place START_MENU at CENTER position (as requested)
    if (!placeUIElement(UIElementName::START_MENU, UIElementPosition::CENTER)) {
        std::cerr << "Failed to place START_MENU UI element!" << std::endl;
        return false;
    }

    placeUIElement(UIElementName::HEALTH_BAR, UIElementPosition::TOP_LEFT_CORNER);
    
    std::cout << "Game UI system initialized successfully" << std::endl;
    return true;
}

bool GameMenus::placeUIElement(UIElementName elementName, UIElementPosition position) {
    // Check if element is already active
    auto existingIt = std::find_if(m_activeElements.begin(), m_activeElements.end(),
        [elementName](const UIElementInstance& element) {
            return element.name == elementName;
        });
    
    if (existingIt != m_activeElements.end()) {
        std::cerr << "UI element already active: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    // Find the registered element
    auto it = m_registeredElements.find(elementName);
    if (it == m_registeredElements.end()) {
        std::cerr << "UI element not registered: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    const UIElementInfo& elementInfo = it->second;
    
    // Load texture
    GLuint textureID;
    int width, height;
    if (!loadUIElementTexture(elementInfo, textureID, width, height)) {
        std::cerr << "Failed to load texture for UI element: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    // Create UI element instance
    UIElementInstance instance(elementName, position, textureID, width, height, elementInfo.scale);
    m_activeElements.push_back(instance);
    
    std::cout << "Placed UI element: " << static_cast<int>(elementName) 
              << " at position: " << static_cast<int>(position) << std::endl;
    return true;
}

void GameMenus::removeUIElement(UIElementName elementName) {
    auto it = std::find_if(m_activeElements.begin(), m_activeElements.end(),
        [elementName](const UIElementInstance& element) {
            return element.name == elementName;
        });
    
    if (it != m_activeElements.end()) {
        // Clean up texture
        glDeleteTextures(1, &it->textureID);
        m_activeElements.erase(it);
        std::cout << "Removed UI element: " << static_cast<int>(elementName) << std::endl;
    }
}

void GameMenus::showUIElement(UIElementName elementName, bool visible) {
    auto it = std::find_if(m_activeElements.begin(), m_activeElements.end(),
        [elementName](UIElementInstance& element) {
            return element.name == elementName;
        });
    
    if (it != m_activeElements.end()) {
        it->visible = visible;
        std::cout << "UI element " << static_cast<int>(elementName) 
                  << " visibility set to: " << visible << std::endl;
    }
}

void GameMenus::hideUIElement(UIElementName elementName) {
    showUIElement(elementName, false);
}

bool GameMenus::isUIElementVisible(UIElementName elementName) const {
    auto it = std::find_if(m_activeElements.begin(), m_activeElements.end(),
        [elementName](const UIElementInstance& element) {
            return element.name == elementName;
        });
    
    return it != m_activeElements.end() && it->visible;
}

void GameMenus::render() {
    if (!m_enginePtr || m_activeElements.empty()) {
        return;
    }
    
    // Update screen dimensions
    glfwGetWindowSize(glfwGetCurrentContext(), &m_screenWidth, &m_screenHeight);
    
    // Save current OpenGL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    // Set up OpenGL state similar to map.cpp
    glUseProgram(0); 
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Disable depth testing to ensure UI renders on top
    glDisable(GL_DEPTH_TEST);
    
    // Enable blending for transparent parts
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable texturing directly with OpenGL
    glEnable(GL_TEXTURE_2D);
    
    // Set texture environment mode to replace (important!)
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    // Set up matrices for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, m_screenWidth, 0, m_screenHeight, -1.0, 1.0);  // Use screen coordinates
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Render all visible UI elements
    for (const auto& element : m_activeElements) {
        if (element.visible) {
            renderUIElement(element);
        }
    }
    
    // Restore matrices
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    
    // Restore previous OpenGL state
    glPopAttrib();
}

void GameMenus::clearAllUIElements() {
    // Clean up all textures
    for (const auto& element : m_activeElements) {
        glDeleteTextures(1, &element.textureID);
    }
    m_activeElements.clear();
    std::cout << "Cleared all UI elements" << std::endl;
}

bool GameMenus::loadUIElementTexture(const UIElementInfo& elementInfo, GLuint& textureID, int& width, int& height) {
    textureID = 0;
    width = 0;
    height = 0;
    
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
    unsigned char* data = stbi_load(elementInfo.texturePath.c_str(), &width, &height, &nrChannels, 0);
    
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
        
        std::cout << "UI texture loaded successfully: " << elementInfo.texturePath << std::endl;
        std::cout << "Dimensions: " << width << "x" << height << ", Channels: " << nrChannels << std::endl;
        
        stbi_image_free(data);
        return true;
    } else {
        std::cerr << "Failed to load UI texture: " << elementInfo.texturePath << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }
}

void GameMenus::calculateElementPosition(UIElementPosition position, int elementWidth, int elementHeight, 
                                        float scale, float& x, float& y, float& width, float& height) const {
    // Apply scaling
    width = elementWidth * scale;
    height = elementHeight * scale;
    
    // Calculate position based on enum
    switch (position) {
        case UIElementPosition::TOP_LEFT_CORNER:
            x = 0;
            y = m_screenHeight - height;
            break;
            
        case UIElementPosition::TOP_RIGHT_CORNER:
            x = m_screenWidth - width;
            y = m_screenHeight - height;
            break;
            
        case UIElementPosition::BOTTOM_LEFT_CORNER:
            x = 0;
            y = 0;
            break;
            
        case UIElementPosition::BOTTOM_RIGHT_CORNER:
            x = m_screenWidth - width;
            y = 0;
            break;
            
        case UIElementPosition::CENTER:
        default:
            x = (m_screenWidth - width) / 2.0f;
            y = (m_screenHeight - height) / 2.0f;
            break;
    }
}

void GameMenus::renderUIElement(const UIElementInstance& element) const {
    // Calculate position and size
    float x, y, width, height;
    calculateElementPosition(element.position, element.width, element.height, 
                           element.scale, x, y, width, height);
    
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, element.textureID);
    
    // Draw the UI element as a quad
    glBegin(GL_QUADS);
    // Bottom-left
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x, y);
    
    // Bottom-right
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + width, y);
    
    // Top-right
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + width, y + height);
    
    // Top-left
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x, y + height);
    glEnd();
}
