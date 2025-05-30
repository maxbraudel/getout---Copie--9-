#ifndef INPUTS_H
#define INPUTS_H

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "enumDefinitions.h"


// Forward declarations
class Map;
class ElementsOnMap;
class EntitiesManager;

// Callback functions
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

// Input processing functions
void processPlayerMovement(double deltaTime, float& playerMoveX, float& playerMoveY);
void processDebugKeys(ElementsOnMap& elementsManager);
void processCameraControls();

// Input state management
void initializeInputs();
void cleanupInputs();

// Key state utility functions
bool isKeyPressed(int key);

// External key state array - accessible from other modules
extern bool keyPressedStates[GLFW_KEY_LAST + 1];

#endif // INPUTS_H
