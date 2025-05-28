#include "RenderSystem.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include "map.h"
#include "elementsOnMap.h"
#include "camera.h"
#include "crashDebug.h"

RenderSystem::RenderSystem() 
    : m_window(nullptr), m_width(0), m_height(0), m_initialized(false) {
}

RenderSystem::~RenderSystem() {
    shutdown();
}

bool RenderSystem::initialize(int width, int height, const std::string& title) {
    m_width = width;
    m_height = height;
    
    std::cout << "Initializing RenderSystem..." << std::endl;
    
    if (!initializeGLFW()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Create window
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
    
    if (!initializeOpenGL()) {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        return false;
    }
    
    setupCallbacks();
    
    m_initialized = true;
    std::cout << "RenderSystem initialized successfully" << std::endl;
    return true;
}

void RenderSystem::shutdown() {
    if (m_initialized) {
        std::cout << "Shutting down RenderSystem..." << std::endl;
        
        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        
        glfwTerminate();
        m_initialized = false;
        
        std::cout << "RenderSystem shutdown complete" << std::endl;
    }
}

void RenderSystem::render(const RenderState& state, Map* gameMap, ElementsOnMap* elementsManager, Camera* camera) {
    if (!m_initialized || !m_window) return;
    
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    
    // Render the game
    renderGame(state, gameMap, elementsManager, camera);
    
    // Swap buffers
    glfwSwapBuffers(m_window);
}

bool RenderSystem::shouldClose() const {
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

bool RenderSystem::initializeGLFW() {
    glfwSetErrorCallback(glfwErrorCallback);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    return true;
}

bool RenderSystem::initializeOpenGL() {
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // Set viewport
    glViewport(0, 0, m_width, m_height);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    
    return true;
}

void RenderSystem::setupCallbacks() {
    // Window resize callback
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });
}

void RenderSystem::renderGame(const RenderState& state, Map* gameMap, ElementsOnMap* elementsManager, Camera* camera) {
    DEBUG_VALIDATE_PTR(gameMap);
    DEBUG_VALIDATE_PTR(elementsManager);
    DEBUG_VALIDATE_PTR(camera);
    
    if (!gameMap || !elementsManager || !camera) {
        std::cerr << "RenderSystem: Invalid pointers for rendering" << std::endl;
        return;
    }
    
    try {
        // Update camera position
        camera->updateCameraPosition(state.playerX, state.playerY, m_width, m_height);
        
        // Render the map
        gameMap->renderMap(*camera);
        
        // Render all elements (entities, decorations, etc.)
        elementsManager->renderElements(*camera);
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during rendering: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception during rendering" << std::endl;
    }
}

void RenderSystem::glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << ") : " << description << std::endl;
}
