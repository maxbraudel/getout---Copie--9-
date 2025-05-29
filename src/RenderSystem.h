#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <memory>
#include <string>
#include "enumDefinitions.h"


struct GLFWwindow;
class Map;
class ElementsOnMap;
class Camera;

/**
 * Render state structure containing all data needed for rendering
 */
struct RenderState {
    float playerX = 0.0f;
    float playerY = 0.0f;
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    double currentTime = 0.0;
    bool playerMoving = false;
};

/**
 * Render System - handles all OpenGL rendering and window management
 * Encapsulates all graphics-related functionality
 */
class RenderSystem {
public:
    RenderSystem();
    ~RenderSystem();
    
    bool initialize(int width, int height, const std::string& title);
    void shutdown();
    
    void render(const RenderState& state, Map* gameMap, ElementsOnMap* elementsManager, Camera* camera);
    bool shouldClose() const;
    
    GLFWwindow* getWindow() { return m_window; }
    
    // Window properties
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
private:
    GLFWwindow* m_window;
    int m_width, m_height;
    bool m_initialized;
    
    bool initializeGLFW();
    bool initializeOpenGL();
    void setupCallbacks();
    void renderGame(const RenderState& state, Map* gameMap, ElementsOnMap* elementsManager, Camera* camera);
    
    // Error handling
    static void glfwErrorCallback(int error, const char* description);
};

#endif // RENDER_SYSTEM_H
