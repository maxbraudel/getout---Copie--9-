#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <functional>
#include <array>
#include <mutex>

struct GLFWwindow;

/**
 * Input state structure that holds all current input information
 */
struct InputState {
    float moveX = 0.0f;
    float moveY = 0.0f;
    std::array<bool, 10> debugKeys = {};
    std::array<bool, 5> cameraControls = {};
    bool stateUpdated = false;
    
    // Reset the state
    void reset() {
        moveX = moveY = 0.0f;
        debugKeys.fill(false);
        cameraControls.fill(false);
        stateUpdated = false;
    }
};

/**
 * Input Manager - handles all input processing and GLFW callbacks
 * Provides clean interface for input state management
 */
class InputManager {
public:
    InputManager();
    ~InputManager();
    
    bool initialize(GLFWwindow* window);
    void pollEvents();
    InputState getCurrentInput();
    
    // Callback setters
    void setWindowCloseCallback(std::function<void()> callback);
    
    // Static methods for GLFW callbacks
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void windowCloseCallback(GLFWwindow* window);
    
private:
    GLFWwindow* m_window;
    InputState m_currentInput;
    std::mutex m_inputMutex;
    std::function<void()> m_windowCloseCallback;
    
    // Internal input processing
    void processKeyInput(int key, int action);
    void updateMovementInput();
};

#endif // INPUT_MANAGER_H
