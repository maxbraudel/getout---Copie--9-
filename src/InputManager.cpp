#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "enumDefinitions.h"
#include "threading.h"


// Static instance for callback access
static InputManager* s_inputManagerInstance = nullptr;

InputManager::InputManager() : m_window(nullptr) {
    s_inputManagerInstance = this;
}

InputManager::~InputManager() {
    if (s_inputManagerInstance == this) {
        s_inputManagerInstance = nullptr;
    }
}

bool InputManager::initialize(GLFWwindow* window) {
    if (!window) {
        std::cerr << "InputManager: Invalid window pointer" << std::endl;
        return false;
    }
    
    m_window = window;
    
    // Set up GLFW callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetWindowCloseCallback(m_window, windowCloseCallback);
    
    std::cout << "InputManager initialized successfully" << std::endl;
    return true;
}

void InputManager::pollEvents() {
    glfwPollEvents();
    updateMovementInput();
}

InputState InputManager::getCurrentInput() {
    std::lock_guard<std::mutex> lock(m_inputMutex);
    InputState current = m_currentInput;
    m_currentInput.stateUpdated = false; // Reset the flag after reading
    return current;
}

void InputManager::setWindowCloseCallback(std::function<void()> callback) {
    m_windowCloseCallback = callback;
}

void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (inputManager) {
        inputManager->processKeyInput(key, action);
    }
}

void InputManager::windowCloseCallback(GLFWwindow* window) {
    InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (inputManager && inputManager->m_windowCloseCallback) {
        inputManager->m_windowCloseCallback();
    }
}

void InputManager::processKeyInput(int key, int action) {
    std::lock_guard<std::mutex> lock(m_inputMutex);
    
    // Process debug keys (F1-F10)
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F10) {
        int debugIndex = key - GLFW_KEY_F1;
        if (action == GLFW_PRESS) {
            m_currentInput.debugKeys[debugIndex] = true;
            m_currentInput.stateUpdated = true;
        }
    }
    
    // Process camera control keys
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_I: // Camera up
                m_currentInput.cameraControls[0] = true;
                m_currentInput.stateUpdated = true;
                break;
            case GLFW_KEY_K: // Camera down
                m_currentInput.cameraControls[1] = true;
                m_currentInput.stateUpdated = true;
                break;
            case GLFW_KEY_J: // Camera left
                m_currentInput.cameraControls[2] = true;
                m_currentInput.stateUpdated = true;
                break;
            case GLFW_KEY_L: // Camera right
                m_currentInput.cameraControls[3] = true;
                m_currentInput.stateUpdated = true;
                break;            case GLFW_KEY_R: // Camera reset
                m_currentInput.cameraControls[4] = true;
                m_currentInput.stateUpdated = true;
                break;            case GLFW_KEY_ESCAPE: // Pause/Resume game
                if (g_threadManager) {
                    if (g_threadManager->isPaused()) {
                        // Check if game is in WIN or DEFEAT state before allowing resume
                        if (GAME_STATE == GameState::WIN) {
                            std::cout << "Cannot resume game - player has won!" << std::endl;
                        } else if (GAME_STATE == GameState::DEFEAT) {
                            std::cout << "Cannot resume game - player has been defeated!" << std::endl;
                        } else {
                            g_threadManager->resumeGame();
                            // Remove pause menu when resuming
                            extern GameMenus gameMenus;
                            gameMenus.removeUIElement(UIElementName::PAUSE_MENU);
                            std::cout << "Game resumed with Tab key" << std::endl;
                        }
                    } else {
                        g_threadManager->pauseGame();
                        // Show pause menu when pausing
                        extern GameMenus gameMenus;
                        gameMenus.placeUIElement(UIElementName::PAUSE_MENU, UIElementPosition::CENTER);
                        std::cout << "Game paused with Tab key" << std::endl;
                    }
                }
                break;
        }
    }
}

void InputManager::updateMovementInput() {
    if (!m_window) return;
    
    std::lock_guard<std::mutex> lock(m_inputMutex);
    
    float oldMoveX = m_currentInput.moveX;
    float oldMoveY = m_currentInput.moveY;
    
    // Reset movement
    m_currentInput.moveX = 0.0f;
    m_currentInput.moveY = 0.0f;
    
    // Don't collect movement input when game is paused
    if (g_threadManager && g_threadManager->isPaused()) {
        // Check if movement state changed (should be reset to 0 when paused)
        if (oldMoveX != m_currentInput.moveX || oldMoveY != m_currentInput.moveY) {
            m_currentInput.stateUpdated = true;
        }
        return;
    }
    
    // Check movement keys
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS) {
        m_currentInput.moveY = 1.0f;
    }
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        m_currentInput.moveY = -1.0f;
    }
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        m_currentInput.moveX = -1.0f;
    }
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        m_currentInput.moveX = 1.0f;
    }
    
    // Check if movement state changed
    if (oldMoveX != m_currentInput.moveX || oldMoveY != m_currentInput.moveY) {
        m_currentInput.stateUpdated = true;
    }
}
