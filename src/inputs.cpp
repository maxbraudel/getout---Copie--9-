#include "inputs.h"
#include "glad/glad.h"
#include "globals.h"
#include "map.h"
#include "elementsOnMap.h"
#include "entities.h"
#include "player.h"
#include "debug.h"
#include "camera.h"
#include "collision.h"
#include "terrainGeneration.h"
#include "enumDefinitions.h"
#include "threading.h"
#include "gameMenus.h" // Added include for game menu system
#include "glbasimac/glbi_engine.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include "enumDefinitions.h"

// Using namespace for MenuState is no longer needed since we're using the qualified name

// External references to global variables and objects
extern Map gameMap;
extern ElementsOnMap elementsManager;
extern EntitiesManager entitiesManager;
extern Camera gameCamera;
extern GameMenus gameMenus;

// Keyboard callback function
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Update key state arrays
    if (action == GLFW_PRESS) {
        keyPressedStates[key] = true;          // Handle immediate keyboard actions
        if (key == GLFW_KEY_X) {
            // Prevent quitting with X key during active gameplay
            if (GAME_STATE == GameState::GAMEPLAY) {
                std::cout << "Cannot quit with X key during active gameplay" << std::endl;
                return;
            }
            glfwSetWindowShouldClose(window, GLFW_TRUE); // Exit on X key press
        }// Pause/Resume game with Tab key
        else if (key == GLFW_KEY_ESCAPE) {
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
        }
        // Reset player position
        else if (key == GLFW_KEY_R) {
            teleportPlayer(10.0f, 10.0f);
            std::cout << "Player position reset to (10, 10)" << std::endl;
        }
        // Test collision handling by attempting to put player in water
        else if (key == GLFW_KEY_F) {
            // Find a water tile in the grid
            bool waterFound = false;
            for (int x = 0; x < GRID_SIZE && !waterFound; x++) {
                for (int y = 0; y < GRID_SIZE && !waterFound; y++) {
                    BlockName blockType = gameMap.getBlockNameByCoordinates(x, y);
                    if (isBlockNonTraversable(blockType)) {
                        // Found a non-traversable block (water), try to teleport player near it
                        std::cout << "Testing collision recovery - attempting to teleport player to water at ("
                                 << x << ", " << y << ")" << std::endl;
                        teleportPlayer(x + 0.5f, y + 0.5f); // Center of the block
                        waterFound = true;
                    }
                }
            }
            if (!waterFound) {
                std::cout << "No non-traversable blocks found to test collision recovery" << std::endl;
            }
        }
        // Test entity collision handling by attempting to put an entity in water
        else if (key == GLFW_KEY_E) {
            // Find a water tile in the grid
            bool waterFound = false;
            for (int x = 0; x < GRID_SIZE && !waterFound; x++) {
                for (int y = 0; y < GRID_SIZE && !waterFound; y++) {
                    BlockName blockType = gameMap.getBlockNameByCoordinates(x, y);
                    if (isBlockNonTraversable(blockType)) {
                        // Found a non-traversable block (water), try to teleport entity near it
                        std::cout << "Testing entity collision recovery - attempting to teleport antagonist to water at ("
                                << x << ", " << y << ")" << std::endl;
                        entitiesManager.teleportEntity("antagonist1", x + 0.5f, y + 0.5f); // Center of the block
                        waterFound = true;
                    }
                }
            }
            if (!waterFound) {
                std::cout << "No non-traversable blocks found to test entity collision recovery" << std::endl;
            }
        }
        // Toggle player animation
        else if (key == GLFW_KEY_T) {
            static bool isAnimating = true;
            isAnimating = !isAnimating;
            elementsManager.changeElementAnimationStatus("player1", isAnimating);
            std::cout << "Player animation " << (isAnimating ? "enabled" : "disabled") << std::endl;
        }
        // Toggle player debug info
        else if (key == GLFW_KEY_F3) {
            togglePlayerDebugMode();
        }
        
        // List all elements when F4 is pressed
        else if (key == GLFW_KEY_F4) {
            std::cout << "\n--- Current Elements List ---" << std::endl;
            elementsManager.listElements();
        }
        // Print detailed element positions when F6 is pressed
        else if (key == GLFW_KEY_F6) {
            elementsManager.printElementPositions();
        }
        // Toggle sand as non-traversable
        else if (key == GLFW_KEY_B) {
            static bool sandIsBlocked = false;
            sandIsBlocked = !sandIsBlocked;
            
            if (sandIsBlocked) {
                addNonTraversableBlock(BlockName::SAND);
                std::cout << "SAND blocks are now non-traversable" << std::endl;
            } else {
                removeNonTraversableBlock(BlockName::SAND);
                std::cout << "SAND blocks are now traversable" << std::endl;
            }
            
            // Print the updated list
            printNonTraversableBlocks();
        }
        // Print all non-traversable blocks
        else if (key == GLFW_KEY_N) {
            printNonTraversableBlocks();
        }
        // Toggle DEBUG_MAP mode with V key
        else if (key == GLFW_KEY_V) {
            DEBUG_MAP = !DEBUG_MAP;
            std::cout << "\nDEBUG MAP mode " << (DEBUG_MAP ? "enabled" : "disabled") << std::endl;
            
            // Regenerate the map with the new setting
            std::cout << "Regenerating terrain with DEBUG_MAP=" << (DEBUG_MAP ? "true" : "false") << "..." << std::endl;
            std::map<std::pair<int, int>, BlockName> generatedMap = generateTerrain(GRID_SIZE, GRID_SIZE, islandFeatureSize, seaFeatureSize, 0.55f, 0.65f);
            gameMap.placeBlocks(generatedMap);
            
            // Regenerate terrain elements
            elementsManager.removeAllElementsByCategory("decoration");
            placeTerrainElements(elementsManager, gameMap, GRID_SIZE, GRID_SIZE);
            
            std::cout << "Map regeneration complete." << std::endl;
        }
        // Show collision information
        else if (key == GLFW_KEY_F7) {
            // Display all collidable elements
            std::vector<std::string> collidables = getCollidableElementNames();
            std::cout << "\n--- Collidable Elements (" << collidables.size() << " total) ---" << std::endl;
            for (const auto& name : collidables) {
                float x, y;
                elementsManager.getElementPosition(name, x, y);
                std::cout << name << " at position (" << x << ", " << y << ")" << std::endl;
            }
            
            // Check if player is near any trees
            float playerX, playerY;
            if (getPlayerPosition(playerX, playerY)) {
                std::cout << "Player position: (" << playerX << ", " << playerY << ")" << std::endl;
                for (const auto& treeName : collidables) {
                    float treeX, treeY;
                    if (elementsManager.getElementPosition(treeName, treeX, treeY)) {
                        float dx = playerX - treeX;
                        float dy = playerY - treeY;
                        float distance = std::sqrt(dx*dx + dy*dy);
                        if (distance < 2.0f) {
                            std::cout << "Tree " << treeName << " at (" << treeX << ", " << treeY 
                                      << ") - distance: " << distance << std::endl;
                        }
                    }
                }
            }
        }        // Toggle path debugging with F8
        else if (key == GLFW_KEY_F8) {
            DEBUG_SHOW_PATHS = !DEBUG_SHOW_PATHS;
            std::cout << "Entity path debugging " << (DEBUG_SHOW_PATHS ? "enabled" : "disabled") << std::endl;
        }        // Toggle gameplay with Enter key
        else if (key == GLFW_KEY_ENTER) {
            extern bool gameplayActive;
            extern glbasimac::GLBI_Engine myEngine;
            bool startGameplay(glbasimac::GLBI_Engine& engine, GLFWwindow* window);
            void endGameplay();
            
            // Prevent stopping gameplay when in active GAMEPLAY state
            if (GAME_STATE == GameState::GAMEPLAY && gameplayActive) {
                std::cout << "Cannot stop gameplay with Enter key during active gameplay" << std::endl;
                return;
            }
            
            if (gameplayActive) {
                endGameplay();
                // Show start menu when gameplay ends
                gameMenus.placeUIElement(UIElementName::START_MENU, UIElementPosition::CENTER);
            } else {
                startGameplay(myEngine, window);
                // Hide menu when gameplay starts
                gameMenus.hideUIElement(UIElementName::START_MENU);
            }
        }
    } else if (action == GLFW_RELEASE) {
        keyPressedStates[key] = false;
    }
}

// Mouse button callback function
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Get cursor position
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        
        // Convert to normalized screen coordinates (-1 to 1)
        double normalizedX = (2.0 * mouseX / windowWidth) - 1.0;
        double normalizedY = (2.0 * mouseY / windowHeight) - 1.0; // Y increases upward
        
        // Compute grid coordinates based on current grid rendering parameters
        // Check if the click is inside the grid area
        if (normalizedX >= g_startX && normalizedX <= g_endX && 
            normalizedY >= g_startY && normalizedY <= g_endY) {
            
            // Map normalized coordinates to grid coordinates
            float gridX = (normalizedX - g_startX) / (g_endX - g_startX) * GRID_SIZE;
            float gridY = (normalizedY - g_startY) / (g_endY - g_startY) * GRID_SIZE;
            
            // Convert to integer grid coordinates
            int gridXInt = static_cast<int>(gridX);
            int gridYInt = static_cast<int>(gridY);
            
            // Calculate world position for reference
            float worldX = gridXInt + 0.5f;  // Center of the block
            float worldY = gridYInt + 0.5f;  // Center of the block
            
            // Get block name at these coordinates
            BlockName blockName = gameMap.getBlockNameByCoordinates(gridXInt, gridYInt);
            
            // Convert enum to string for better output
            std::string blockNameStr = "UNKNOWN";
            switch (blockName) {
                case BlockName::GRASS_0: blockNameStr = "GRASS_0"; break;
                case BlockName::GRASS_1: blockNameStr = "GRASS_1"; break;
                case BlockName::GRASS_2: blockNameStr = "GRASS_2"; break;
                case BlockName::GRASS_3: blockNameStr = "GRASS_3"; break;
                case BlockName::GRASS_4: blockNameStr = "GRASS_4"; break;
                case BlockName::GRASS_5: blockNameStr = "GRASS_5"; break;
                case BlockName::SAND: blockNameStr = "SAND"; break;
                case BlockName::WATER_0: blockNameStr = "WATER_0"; break;
                case BlockName::WATER_1: blockNameStr = "WATER_1"; break;
                case BlockName::WATER_2: blockNameStr = "WATER_2"; break;
                case BlockName::WATER_3: blockNameStr = "WATER_3"; break;
                case BlockName::WATER_4: blockNameStr = "WATER_4"; break;
                default: blockNameStr = "UNKNOWN"; break;
            }
            
            // Output the block information
            std::cout << "Clicked on grid cell: (" << gridXInt << ", " << gridYInt << ")" << std::endl;
            std::cout << "Block type: " << blockNameStr << " (enum value: " << static_cast<int>(blockName) << ")" << std::endl;
        }
    }
}

// Process player movement based on current key states
void processPlayerMovement(double deltaTime, float& playerMoveX, float& playerMoveY) {
    playerMoveX = 0.0f;
    playerMoveY = 0.0f;
    
    // Apply speed based on whether shift is held
    float currentSpeed = 0.0f;
      // Use player entity configuration for speeds
    const EntityConfiguration* playerConfig = entitiesManager.getConfiguration(EntityName::PLAYER);
    if (playerConfig) {
        // Use speeds from player entity configuration
        if (keyPressedStates[GLFW_KEY_LEFT_SHIFT] || keyPressedStates[GLFW_KEY_RIGHT_SHIFT]) {
            currentSpeed = playerConfig->sprintWalkingSpeed; // Use sprint speed from entity config
        } else {
            currentSpeed = playerConfig->normalWalkingSpeed; // Use normal speed from entity config
        }
    } else {
        // No fallback to constants - log error instead
        std::cerr << "ERROR: Player configuration not found! Movement will be disabled." << std::endl;
        return;
    }
    
    // Check arrow keys for player movement
    if (keyPressedStates[GLFW_KEY_UP] || keyPressedStates[GLFW_KEY_W]) {
        playerMoveY += currentSpeed * (float)deltaTime;
    }
    if (keyPressedStates[GLFW_KEY_DOWN] || keyPressedStates[GLFW_KEY_S]) {
        playerMoveY -= currentSpeed * (float)deltaTime;
    }
    if (keyPressedStates[GLFW_KEY_LEFT] || keyPressedStates[GLFW_KEY_A]) {
        playerMoveX -= currentSpeed * (float)deltaTime;
    }
    if (keyPressedStates[GLFW_KEY_RIGHT] || keyPressedStates[GLFW_KEY_D]) {
        playerMoveX += currentSpeed * (float)deltaTime;
    }
    
    // Normalize diagonal movement to prevent faster diagonal speed
    if (playerMoveX != 0.0f && playerMoveY != 0.0f) {
        // Scale by 1/sqrt(2) to maintain consistent speed in all directions
        float normalizeFactor = 0.7071f; // 1/sqrt(2)
        playerMoveX *= normalizeFactor;
        playerMoveY *= normalizeFactor;
    }
}

// Process debug keys that need to be checked each frame
void processDebugKeys(ElementsOnMap& elementsManager) {
    // Toggle grid lines with G key
    static bool gKeyPressed = false;
    if (keyPressedStates[GLFW_KEY_G] && !gKeyPressed) {
        showGridLines = !showGridLines;
        gKeyPressed = true;
    } else if (!keyPressedStates[GLFW_KEY_G]) {
        gKeyPressed = false;
    }
    
    // Log camera dimensions when F1 is pressed (for debugging)
    static bool lastF1KeyState = false;
    if (keyPressedStates[GLFW_KEY_F1] && !lastF1KeyState) {
        float cameraWidth = gameCamera.getWidth();
        float cameraHeight = gameCamera.getHeight();
        float windowAspectRatio = (float)windowWidth / (float)windowHeight;
        
        std::cout << "Window size: " << windowWidth << "x" << windowHeight 
                  << ", Aspect ratio: " << windowAspectRatio 
                  << ", Camera size: " << cameraWidth << "x" << cameraHeight 
                  << " grid units" << std::endl;
    }
    lastF1KeyState = keyPressedStates[GLFW_KEY_F1];
    
    // Toggle grid lines with F2
    static bool lastFrameGridKeyState = false;
    if (keyPressedStates[GLFW_KEY_F2] && !lastFrameGridKeyState) {
        showGridLines = !showGridLines;
        std::cout << "Grid lines " << (showGridLines ? "enabled" : "disabled") << std::endl;
    }
    lastFrameGridKeyState = keyPressedStates[GLFW_KEY_F2];
    
    // Handle debug key functionalities (F5 for anchor points, F7 for collision boxes)
    handleDebugKeys(elementsManager, keyPressedStates);
    
    // Print elements list when F4 is pressed
    static bool lastFrameDebugElementsState = false;
    if (keyPressedStates[GLFW_KEY_F4] && !lastFrameDebugElementsState) {
        std::cout << "\n--- Current Elements List ---" << std::endl;
        elementsManager.listElements();
    }
    lastFrameDebugElementsState = keyPressedStates[GLFW_KEY_F4];
    
    // Toggle hide pixels outside map grid with F6
    static bool lastFrameHideOutsideGridState = false;
    if (keyPressedStates[GLFW_KEY_F6] && !lastFrameHideOutsideGridState) {
        hideOutsideGrid = !hideOutsideGrid;
        std::cout << "Hiding pixels outside map grid: " << (hideOutsideGrid ? "enabled" : "disabled") << std::endl;
    }
    lastFrameHideOutsideGridState = keyPressedStates[GLFW_KEY_F6];
}

// Process camera controls
void processCameraControls() {
    // Adjust camera region with P and M keys using smooth transitions
    static bool lastPKeyState = false;
    static bool lastMKeyState = false;
    
    // P key - zoom in (decrease camera region smoothly)
    if (keyPressedStates[GLFW_KEY_P] && !lastPKeyState) {
        gameCamera.decreaseCameraRegionSmoothly(10.0f, 0.5f); // 1.0 units over 0.5 seconds
    }
    lastPKeyState = keyPressedStates[GLFW_KEY_P];
    
    // M key - zoom out (increase camera region smoothly)
    if (keyPressedStates[GLFW_KEY_M] && !lastMKeyState) {
        gameCamera.increaseCameraRegionSmoothly(10.0f, 0.5f); // 1.0 units over 0.5 seconds
    }
    lastMKeyState = keyPressedStates[GLFW_KEY_M];
}

// Initialize input system
void initializeInputs() {
    // Initialize key state array
    for (int i = 0; i <= GLFW_KEY_LAST; i++) {
        keyPressedStates[i] = false;
    }
}

// Cleanup input system
void cleanupInputs() {
    // Nothing to cleanup currently
}

// Utility function to check if a key is currently pressed
bool isKeyPressed(int key) {
    if (key >= 0 && key <= GLFW_KEY_LAST) {
        return keyPressedStates[key];
    }
    return false;
}
