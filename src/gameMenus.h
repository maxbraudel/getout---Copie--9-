#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glbasimac/glbi_engine.hpp"
#include "glbasimac/glbi_texture.hpp"
#include <string>
#include <map>

enum class MenuState {
    NONE,
    START_MENU,
    PAUSE_MENU,
    GAME_OVER,
    OPTIONS
};

class GameMenus {
public:
    GameMenus();
    ~GameMenus();
    
    // Initialize the menu system and load all menu textures
    bool initialize(glbasimac::GLBI_Engine& engine);
    
    // Show a specific menu
    void showMenu(MenuState menuType);
    
    // Hide current menu
    void hideMenu();
    
    // Render the current active menu (if any)
    void render(float startX, float endX, float startY, float endY);
    
    // Check if any menu is currently displayed
    bool isMenuActive() const { return m_currentMenuState != MenuState::NONE; }
    
    // Get the current menu state
    MenuState getCurrentMenuState() const { return m_currentMenuState; }
    
private:
    // Load a menu texture from file
    bool loadMenuTexture(const std::string& path, MenuState menuType);
    
    MenuState m_currentMenuState;
    std::map<MenuState, GLuint> m_menuTextures;
    glbasimac::GLBI_Engine* m_enginePtr;
    
    // Store texture dimensions for proper scaling
    struct TextureInfo {
        int width;
        int height;
    };
    std::map<MenuState, TextureInfo> m_textureInfo;
};

// Global instance of the menu system
extern GameMenus gameMenus;
