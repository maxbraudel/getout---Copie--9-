#include "gameMenus.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
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
    uiElements.push_back(optionsMenu);    // Health Bar - example sprite sheet UI element
    UIElementInfo healthBar;
    healthBar.name = UIElementName::HEALTH_BAR;
    healthBar.texturePath = "../assets/textures/ui/hearts.png";
    healthBar.scale = 5.0f;
    healthBar.type = UIElementTextureType::SPRITESHEET;
    healthBar.spriteWidth = 110;  // Width of each heart sprite
    healthBar.spriteHeight = 28; // Height of each heart sprite
    healthBar.defaultSpriteSheetPhase = 4;  // Default to full hearts
    healthBar.defaultSpriteSheetFrame = 0;  // Default to first frame
    healthBar.isAnimated = false;            // Animate the hearts
    healthBar.animationSpeed = 2.0f;        // Slow animation for hearts
    // Add margins for positioning offset
    uiElements.push_back(healthBar);

    UIElementInfo coconuts;
    coconuts.name = UIElementName::COCONUTS;
    coconuts.texturePath = "../assets/textures/ui/coconuts.png";
    coconuts.scale = 3.0f;
    coconuts.type = UIElementTextureType::SPRITESHEET;
    coconuts.spriteWidth = 80;  // Width of each heart sprite
    coconuts.spriteHeight = 51; // Height of each heart sprite
    coconuts.defaultSpriteSheetPhase = 3;  // Default to full hearts
    coconuts.defaultSpriteSheetFrame = 0;  // Default to first frame
    coconuts.isAnimated = false;            // Animate the hearts
    coconuts.animationSpeed = 2.0f;        // Slow animation for hearts
    // Add margins for positioning offset
    coconuts.marginTop = 10.0f;     // 30 pixels from top
    coconuts.marginRight = 10.0f;   // 30 pixels from right
    uiElements.push_back(coconuts);
    
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

    placeUIElement(UIElementName::COCONUTS, UIElementPosition::TOP_RIGHT_CORNER);
    
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
      // Set sprite sheet properties if this is a spritesheet texture
    instance.type = elementInfo.type;
    instance.spriteWidth = elementInfo.spriteWidth;
    instance.spriteHeight = elementInfo.spriteHeight;
    instance.totalWidth = width;
    instance.totalHeight = height;
    instance.spriteSheetPhase = elementInfo.defaultSpriteSheetPhase;
    instance.spriteSheetFrame = elementInfo.defaultSpriteSheetFrame;
    instance.isAnimated = elementInfo.isAnimated;
    instance.animationSpeed = elementInfo.animationSpeed;
    instance.currentFrameTime = 0.0f;
    
    // Set margin parameters
    instance.marginTop = elementInfo.marginTop;
    instance.marginBottom = elementInfo.marginBottom;
    instance.marginLeft = elementInfo.marginLeft;
    instance.marginRight = elementInfo.marginRight;
    
    // Calculate number of frames in the current phase for sprite sheets
    if (instance.type == UIElementTextureType::SPRITESHEET && instance.spriteWidth > 0) {
        instance.numFramesInPhase = instance.totalWidth / instance.spriteWidth;
    } else {
        instance.numFramesInPhase = 0;
    }
    
    m_activeElements.push_back(instance);
    
    std::cout << "Placed UI element: " << static_cast<int>(elementName) 
              << " at position: " << static_cast<int>(position);
    if (instance.type == UIElementTextureType::SPRITESHEET) {
        std::cout << " (sprite sheet: " << instance.spriteWidth << "x" << instance.spriteHeight 
                  << ", phase: " << instance.spriteSheetPhase 
                  << ", frame: " << instance.spriteSheetFrame
                  << ", frames in phase: " << instance.numFramesInPhase
                  << ", animated: " << (instance.isAnimated ? "yes" : "no") << ")";
    }
    std::cout << std::endl;
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

void GameMenus::render(double deltaTime) {
    if (!m_enginePtr || m_activeElements.empty()) {
        return;
    }
    
    // Update screen dimensions
    glfwGetWindowSize(glfwGetCurrentContext(), &m_screenWidth, &m_screenHeight);
    
    // Update animations for sprite sheet elements
    for (auto& element : m_activeElements) { // Changed to non-const to update animation state
        if (element.type == UIElementTextureType::SPRITESHEET && 
            element.isAnimated && 
            element.numFramesInPhase > 0 && 
            element.animationSpeed > 0.0f) {
            
            // Update animation frame time
            element.currentFrameTime += static_cast<float>(deltaTime);
            
            // Calculate frame time based on animation speed
            float frameTime = 1.0f / element.animationSpeed; // Time per frame in seconds
            
            if (element.currentFrameTime >= frameTime) {
                // Advance to next frame
                int advanceFrames = static_cast<int>(element.currentFrameTime / frameTime);
                element.spriteSheetFrame = (element.spriteSheetFrame + advanceFrames) % element.numFramesInPhase;
                element.currentFrameTime = fmod(element.currentFrameTime, frameTime); // Keep remainder
            }
        }
    }
    
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
                                        float scale, float marginTop, float marginBottom, 
                                        float marginLeft, float marginRight,
                                        float& x, float& y, float& width, float& height) const {
    // Apply scaling
    width = elementWidth * scale;
    height = elementHeight * scale;
    
    // Calculate base position based on enum
    switch (position) {
        case UIElementPosition::TOP_LEFT_CORNER:
            x = 0 + marginLeft;
            y = m_screenHeight - height - marginTop;
            break;
            
        case UIElementPosition::TOP_RIGHT_CORNER:
            x = m_screenWidth - width - marginRight;
            y = m_screenHeight - height - marginTop;
            break;
            
        case UIElementPosition::BOTTOM_LEFT_CORNER:
            x = 0 + marginLeft;
            y = 0 + marginBottom;
            break;
            
        case UIElementPosition::BOTTOM_RIGHT_CORNER:
            x = m_screenWidth - width - marginRight;
            y = 0 + marginBottom;
            break;
            
        case UIElementPosition::CENTER:
        default:
            x = (m_screenWidth - width) / 2.0f + marginLeft - marginRight;
            y = (m_screenHeight - height) / 2.0f + marginBottom - marginTop;
            break;
    }
}

void GameMenus::renderUIElement(const UIElementInstance& element) const {
    // Calculate position and size
    float x, y, width, height;
    
    // For sprite sheets, use sprite dimensions instead of full texture dimensions
    int renderWidth = element.width;
    int renderHeight = element.height;
    
    if (element.type == UIElementTextureType::SPRITESHEET && 
        element.spriteWidth > 0 && element.spriteHeight > 0) {
        renderWidth = element.spriteWidth;
        renderHeight = element.spriteHeight;
    }
      calculateElementPosition(element.position, renderWidth, renderHeight, 
                           element.scale, element.marginTop, element.marginBottom,
                           element.marginLeft, element.marginRight, x, y, width, height);
    
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, element.textureID);
    
    // Calculate UV coordinates based on whether this is a spritesheet
    float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
    
    if (element.type == UIElementTextureType::SPRITESHEET && 
        element.spriteWidth > 0 && element.spriteHeight > 0 &&
        element.totalWidth > 0 && element.totalHeight > 0) {
        
        // Calculate UV coordinates for the current frame in the sprite sheet
        float frameWidthRatio = static_cast<float>(element.spriteWidth) / element.totalWidth;
        float frameHeightRatio = static_cast<float>(element.spriteHeight) / element.totalHeight;
        
        // Calculate horizontal position (frame index)
        u0 = element.spriteSheetFrame * frameWidthRatio;
        u1 = u0 + frameWidthRatio;
        
        // Calculate vertical position (sprite sheet phase/row)
        // With stbi_set_flip_vertically_on_load(true), adjust row calculation
        v0 = element.spriteSheetPhase * frameHeightRatio;
        v1 = v0 + frameHeightRatio;
    }
    
    // Draw the UI element as a quad
    glBegin(GL_QUADS);
    // Bottom-left
    glTexCoord2f(u0, v0);
    glVertex2f(x, y);
    
    // Bottom-right
    glTexCoord2f(u1, v0);
    glVertex2f(x + width, y);
    
    // Top-right
    glTexCoord2f(u1, v1);
    glVertex2f(x + width, y + height);
    
    // Top-left
    glTexCoord2f(u0, v1);
    glVertex2f(x, y + height);
    glEnd();
}

bool GameMenus::changeUIElementSpriteSheetPhase(UIElementName elementName, int newPhase) {
    // Find the UI element instance
    auto it = std::find_if(m_activeElements.begin(), m_activeElements.end(),
        [elementName](const UIElementInstance& element) {
            return element.name == elementName;
        });
    
    if (it == m_activeElements.end()) {
        std::cerr << "UI element not found for changing sprite phase: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    // Check if this is a spritesheet element
    if (it->type != UIElementTextureType::SPRITESHEET) {
        std::cerr << "UI element doesn't support sprite phases: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    // Check if phase is valid (calculate number of phases based on texture height)
    if (it->spriteHeight > 0 && it->totalHeight > 0) {
        int numPhases = it->totalHeight / it->spriteHeight;
        if (newPhase >= 0 && newPhase < numPhases) {
            it->spriteSheetPhase = newPhase;
            std::cout << "Changed UI element sprite phase: " << static_cast<int>(elementName) 
                      << " to " << newPhase << std::endl;
            return true;
        } else {
            std::cerr << "Invalid sprite phase " << newPhase << " for UI element: " << static_cast<int>(elementName)
                      << " (valid range: 0-" << (numPhases - 1) << ")" << std::endl;
            return false;
        }
    }
    
    std::cerr << "Invalid sprite sheet configuration for UI element: " << static_cast<int>(elementName) << std::endl;
    return false;
}

bool GameMenus::changeUIElementSpriteSheetFrame(UIElementName elementName, int newFrame) {
    // Find the UI element instance
    auto it = std::find_if(m_activeElements.begin(), m_activeElements.end(),
        [elementName](const UIElementInstance& element) {
            return element.name == elementName;
        });
    
    if (it == m_activeElements.end()) {
        std::cerr << "UI element not found for changing sprite frame: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    // Check if this is a spritesheet element
    if (it->type != UIElementTextureType::SPRITESHEET) {
        std::cerr << "UI element doesn't support sprite frames: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    // Ensure frame is within valid range
    if (it->numFramesInPhase > 0) {
        it->spriteSheetFrame = newFrame % it->numFramesInPhase;
        std::cout << "Changed UI element sprite frame: " << static_cast<int>(elementName) 
                  << " to " << it->spriteSheetFrame << std::endl;
        return true;
    } else {
        std::cerr << "UI element doesn't support sprite frames: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
}

bool GameMenus::changeUIElementAnimationStatus(UIElementName elementName, bool isAnimated) {
    // Find the UI element instance
    auto it = std::find_if(m_activeElements.begin(), m_activeElements.end(),
        [elementName](const UIElementInstance& element) {
            return element.name == elementName;
        });
    
    if (it == m_activeElements.end()) {
        std::cerr << "UI element not found for changing animation status: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    // Check if this is a spritesheet element
    if (it->type != UIElementTextureType::SPRITESHEET) {
        std::cerr << "UI element doesn't support animation: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    it->isAnimated = isAnimated;
    if (isAnimated) {
        it->currentFrameTime = 0.0f; // Reset animation timer
    }
    
    std::cout << "Changed UI element animation status: " << static_cast<int>(elementName) 
              << " to " << (isAnimated ? "animated" : "static") << std::endl;
    return true;
}

bool GameMenus::changeUIElementAnimationSpeed(UIElementName elementName, float newSpeed) {
    // Find the UI element instance
    auto it = std::find_if(m_activeElements.begin(), m_activeElements.end(),
        [elementName](const UIElementInstance& element) {
            return element.name == elementName;
        });
    
    if (it == m_activeElements.end()) {
        std::cerr << "UI element not found for changing animation speed: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    // Check if this is a spritesheet element
    if (it->type != UIElementTextureType::SPRITESHEET) {
        std::cerr << "UI element doesn't support animation: " << static_cast<int>(elementName) << std::endl;
        return false;
    }
    
    if (newSpeed >= 0.0f) {
        it->animationSpeed = newSpeed;
        std::cout << "Changed UI element animation speed: " << static_cast<int>(elementName) 
                  << " to " << newSpeed << " FPS" << std::endl;
        return true;
    } else {
        std::cerr << "Invalid animation speed (must be non-negative): " << newSpeed << std::endl;
        return false;
    }
}
