#include "gameMenus.h"
#include <iostream>
#include <string>
#include "glbasimac/glbi_texture.hpp"
// Include stb_image without defining STB_IMAGE_IMPLEMENTATION
// This avoids duplicate symbols since it's already defined in map.cpp
#include "../third_party/glbasimac/tools/stb_image.h"

// Global instance of the menu system
GameMenus gameMenus;

GameMenus::GameMenus() : m_currentMenuState(MenuState::NONE), m_enginePtr(nullptr) {
}

GameMenus::~GameMenus() {
    // Clean up textures
    for (const auto& texture : m_menuTextures) {
        glDeleteTextures(1, &texture.second);
    }
    m_menuTextures.clear();
}

bool GameMenus::initialize(glbasimac::GLBI_Engine& engine) {
    m_enginePtr = &engine;
    
    // Load all menu textures
    if (!loadMenuTexture("../assets/textures/ui/startMenu.png", MenuState::START_MENU)) {
        std::cerr << "Failed to load start menu texture!" << std::endl;
        return false;
    }
    
    // By default, show the start menu at initialization
    showMenu(MenuState::START_MENU);
    
    std::cout << "Game menu system initialized successfully" << std::endl;
    return true;
}

bool GameMenus::loadMenuTexture(const std::string& path, MenuState menuType) {
    GLuint textureID = 0;
    int width = 0, height = 0;
    
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
        
        std::cout << "Menu texture loaded successfully: " << path << std::endl;
        std::cout << "Dimensions: " << width << "x" << height << ", Channels: " << nrChannels << std::endl;
        
        m_menuTextures[menuType] = textureID;
        m_textureInfo[menuType] = {width, height};
        
        stbi_image_free(data);
        return true;
    } else {
        std::cerr << "Failed to load menu texture: " << path << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }
}

void GameMenus::showMenu(MenuState menuType) {
    // Ensure we have a texture for this menu type
    if (m_menuTextures.find(menuType) == m_menuTextures.end()) {
        std::cerr << "No texture loaded for menu type: " << static_cast<int>(menuType) << std::endl;
        return;
    }
    
    m_currentMenuState = menuType;
    std::cout << "Showing menu: " << static_cast<int>(menuType) << std::endl;
}

void GameMenus::hideMenu() {
    m_currentMenuState = MenuState::NONE;
    std::cout << "Menu hidden" << std::endl;
}

void GameMenus::render(float startX, float endX, float startY, float endY) {
    // Only render if a menu is active
    if (m_currentMenuState == MenuState::NONE || !m_enginePtr) {
        return;
    }
    
    // Get the texture for the current menu
    auto it = m_menuTextures.find(m_currentMenuState);
    if (it == m_menuTextures.end()) {
        return;
    }
    
    // Save current OpenGL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    // Set up OpenGL state similar to map.cpp
    glUseProgram(0); 
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Disable depth testing to ensure menu renders on top
    glDisable(GL_DEPTH_TEST);
    
    // Enable blending for transparent parts of the menu
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable texturing directly with OpenGL
    glEnable(GL_TEXTURE_2D);
    
    // Set texture environment mode to replace (important!)
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    // Bind the menu texture
    glBindTexture(GL_TEXTURE_2D, it->second);
    
    // Set up matrices for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);  // Set up orthographic projection
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Draw the menu texture as a quad
    glBegin(GL_QUADS);
    // Bottom-left
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(startX, startY);
    
    // Bottom-right
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(endX, startY);
    
    // Top-right
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(endX, endY);
    
    // Top-left
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(startX, endY);
    glEnd();
    
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
