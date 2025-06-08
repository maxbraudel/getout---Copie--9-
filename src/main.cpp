#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "glbasimac/glbi_engine.hpp"
#include <iostream>
#include <vector>
#include "player.h" // Added include for player management
#include "camera.h" // Added include for camera management
#include "globals.h" // Added include for global variables
#include "inputs.h" // Added include for input handling
#include "threading.h" // Added include for threading system
#include "crashDebug.h" // Added include for crash debugging
#include "Gameplay.h" // Added include for gameplay initialization
#include "map.h" // Added include for Map class
#include "elementsOnMap.h" // Added include for ElementsOnMap class
#include "entities.h" // Added include for EntitiesManager class
#include "gameMenus.h" // Added include for game menu system
#include <ctime> // For time(0) to seed random number generator
#include <cmath> // For sqrt function
#include <algorithm> // For std::min and std::max
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::seconds
#include "enumDefinitions.h"

// Global gameplay state
bool gameplayActive = false;

using namespace glbasimac;

/* OpenGL Engine */

/* OpenGL Engine */
GLBI_Engine myEngine;

/* Forward declarations for functions in player.cpp */
void updatePlayer(GLFWwindow* window, double deltaTime, Map& gameMap, ElementsOnMap& elementsManager);
bool getPlayerPosition(float& x, float& y);

/* Forward declarations for functions in camera.cpp */
void updateCameraToPlayerPosition();

/* Forward declarations for functions in debug.cpp */
bool isPlayerDebugModeActive();
void printPlayerDebugInfo(float playerX, float playerY, int mapWidth, int mapHeight);

/* Gameplay management functions */
bool startGameplay(GLBI_Engine& engine, GLFWwindow* window);
void endGameplay();

/* Gameplay management function implementations */
bool startGameplay(GLBI_Engine& engine, GLFWwindow* window) {
    if (gameplayActive) {
        return true; // Already active
    }
    
    std::cout << "Starting gameplay..." << std::endl;
    
    // Initialize all gameplay systems (map, entities, elements, threading)
    if (!Gameplay::initialize(engine, window)) {
        std::cerr << "Failed to initialize gameplay systems!" << std::endl;
        return false;
    }
    
    // Reset all entity movement states to fix movement issues after restart
    std::cout << "Resetting entity movement states..." << std::endl;
    Gameplay::getEntitiesManager().resetAllEntityMovementStates();
    
    // Ensure camera is properly positioned at player's starting position to prevent flicker
    float playerX, playerY;
    if (getPlayerPosition(playerX, playerY)) {
        std::cout << "Pre-positioning camera at player start position: (" << playerX << ", " << playerY << ")" << std::endl;
        gameCamera.updateCameraPosition(playerX, playerY, windowWidth, windowHeight);
    } else {
        // Use default player position coordinates if player entity not found yet
        playerX = 5.0f; // Should match the player entity placement in Gameplay::placeInitialEntities
        playerY = 45.0f;
        std::cout << "Pre-positioning camera at default player position: (" << playerX << ", " << playerY << ")" << std::endl;
        gameCamera.updateCameraPosition(playerX, playerY, windowWidth, windowHeight);
    }
    
    // Force a single frame render with the camera correctly positioned before starting threads
    // This eliminates the camera flicker during gameplay start
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    glClear(GL_COLOR_BUFFER_BIT);
    // Render just the loader animation, camera will be correctly positioned when gameplay appears
    gameMenus.render(0.016);
    glfwSwapBuffers(window);
    
    // Start game threads
    if (!Gameplay::startGameThreads()) {
        std::cerr << "Failed to start game threads!" << std::endl;
        Gameplay::cleanup();
        return false;
    }
    
    gameMenus.placeUIElement(UIElementName::HEALTH_BAR, UIElementPosition::TOP_LEFT_CORNER);
    gameMenus.placeUIElement(UIElementName::COCONUTS, UIElementPosition::TOP_RIGHT_CORNER);


    
    // Set game state to GAMEPLAY
    GAME_STATE = GameState::GAMEPLAY;
    std::cout << "Game state set to: " << gameStateToString(GAME_STATE) << std::endl;
    
    gameplayActive = true;
    std::cout << "Gameplay started successfully" << std::endl;
    return true;
}

void endGameplay() {
    if (!gameplayActive) {
        return; // Already inactive
    }

	gameMenus.removeUIElement(UIElementName::HEALTH_BAR);
    gameMenus.removeUIElement(UIElementName::COCONUTS);
    gameMenus.removeUIElement(UIElementName::PAUSE_MENU);
    gameMenus.removeUIElement(UIElementName::GAME_OVER);
    gameMenus.removeUIElement(UIElementName::WIN_MENU);
    
    std::cout << "Stopping gameplay..." << std::endl;
    
    // If the game is paused, resume it first to ensure proper cleanup
    if (g_threadManager && g_threadManager->isPaused()) {
        std::cout << "Game is paused - resuming before cleanup..." << std::endl;
        g_threadManager->resumeGame();
    }
      // Cleanup gameplay systems (threads, entities, etc.)
    Gameplay::cleanup();

    gameMenus.removeUIElement(UIElementName::HEALTH_BAR);
    gameMenus.removeUIElement(UIElementName::COCONUTS);
    gameMenus.removeUIElement(UIElementName::PAUSE_MENU);
    gameMenus.removeUIElement(UIElementName::GAME_OVER);
    gameMenus.removeUIElement(UIElementName::WIN_MENU);


    gameMenus.placeUIElement(UIElementName::START_MENU, UIElementPosition::CENTER);
    
    // Set game state back to START
    GAME_STATE = GameState::START;
    std::cout << "Game state set to: " << gameStateToString(GAME_STATE) << std::endl;
    
    gameplayActive = false;
    std::cout << "Gameplay stopped" << std::endl;

	gameMenus.removeUIElement(UIElementName::HEALTH_BAR);
    gameMenus.removeUIElement(UIElementName::COCONUTS);
    gameMenus.removeUIElement(UIElementName::PAUSE_MENU);
    gameMenus.removeUIElement(UIElementName::GAME_OVER);
    gameMenus.removeUIElement(UIElementName::WIN_MENU);
}

/* Window close callback function */
void onWindowClose(GLFWwindow* window) {
    std::cout << "Window close callback triggered - starting cleanup..." << std::endl;
    
    // Signal thread manager to stop
    if (g_threadManager) {
        g_threadManager->setRunning(false);
    }
    
    // Cleanup gameplay systems including async pathfinding
    Gameplay::cleanup();
    
    std::cout << "Cleanup initiated from window close callback" << std::endl;
}

/* Error handling function */
void onError(int error, const char* description) {
	std::cout << "GLFW Error ("<<error<<") : " << description << std::endl;
}

/* Window resize callback function */
void onWindowResize(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    
    // Calculate aspect ratio adjusted projection
    float aspectRatio = (float)width / (float)height;
    float projLeft, projRight, projBottom, projTop;
    
    if (aspectRatio >= 1.0f) {
        // Window is wider than tall - expand projection width
        projLeft = -aspectRatio;
        projRight = aspectRatio;
        projBottom = -1.0f;
        projTop = 1.0f;
    } else {
        // Window is taller than wide - expand projection height
        projLeft = -1.0f;
        projRight = 1.0f;
        projBottom = -1.0f / aspectRatio;
        projTop = 1.0f / aspectRatio;
    }
    
    // Update the projection matrix to match window's aspect ratio
    // Only do this if we have a valid OpenGL context
    if (glfwGetCurrentContext() != nullptr) {
        myEngine.set2DProjection(projLeft, projRight, projBottom, projTop);
    }
      // Update camera to adapt to new window size (works even when game is paused)
    // This ensures the camera view adjusts properly during window resize regardless of pause state
    float playerX, playerY;
    if (getPlayerPosition(playerX, playerY)) {
        // Player exists - update camera with current player position
        gameCamera.updateCameraPosition(playerX, playerY, width, height);
    } else if (gameCamera.hasLastKnownPlayerPosition()) {
        // Player doesn't exist but we have a last known position - use that
        gameCamera.getLastKnownPlayerPosition(playerX, playerY);
        gameCamera.updateCameraPosition(playerX, playerY, width, height);
        std::cout << "Using last known player position for camera: (" << playerX << ", " << playerY << ")" << std::endl;
    } else {
        // No player and no last known position - use center of the map
        gameCamera.updateCameraPosition(GRID_SIZE / 2.0f, GRID_SIZE / 2.0f, width, height);
    }
    
    // Keep grid rendering parameters for coordinate conversion covering the full NDC space
    g_startX = -1.0f;
    g_endX = 1.0f;
    g_startY = -1.0f;    g_endY = 1.0f;
}

int main() {
    // CRASH FIX: Initialize crash handler first for comprehensive crash debugging
    installCrashHandler();
    setCrashLogPath("game_crash_log.txt");
    
    std::cout << "=== GAME STARTUP ===" << std::endl;
    DEBUG_LOG_MEMORY("main_start");
    
    // CRASH FIX: Add global exception handler
    try {
        // Seed the random number generator ONCE at the start of the program
        // You can change time(0) to a specific unsigned int for a fixed, repeatable map.
        unsigned int terrainSeed = static_cast<unsigned int>(time(0));
        // unsigned int terrainSeed = 12345; // Example of a fixed seed
        srand(terrainSeed);
        
        // Initialize the library
        if (!glfwInit()) {
            std::cerr << "CRASH FIX: Failed to initialize GLFW" << std::endl;
            return -1;
        }
        
        // CRASH FIX: Set up GLFW error callback before any other GLFW calls
        glfwSetErrorCallback(onError);
        
        // Initialize input system
        initializeInputs();/* Callback to a function if an error is rised by GLFW */
	glfwSetErrorCallback(onError);
    
    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(1024, 1024, "Sorbet Coco", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }    // Make the window's context current
    glfwMakeContextCurrent(window);    // Set the window resize callback
    glfwSetWindowSizeCallback(window, onWindowResize);
    
    // Set the window close callback
    glfwSetWindowCloseCallback(window, onWindowClose);
    
    // Set the keyboard callback
    glfwSetKeyCallback(window, keyCallback);
    
    // Set the mouse button callback
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
      // Get initial window size
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    	// Intialize glad (loads the OpenGL functions)
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		return -1;
	}      // Initialize Rendering Engine first
	myEngine.initGL();
		// Then call the resize callback to set up the correct projection based on window size
    onWindowResize(window, windowWidth, windowHeight);
		// Initialize the menu system
	if (!gameMenus.initialize(myEngine)) {
		std::cerr << "Failed to initialize game menu system!" << std::endl;
		return -1;
	}
	
	std::cout << "Game engine initialization complete - press Enter to start gameplay" << std::endl;
	/* Main render loop - runs until user closes the window */
	int frameCount = 0;
	while (!glfwWindowShouldClose(window))
	{
		frameCount++;
		
		// CRASH FIX: Add periodic memory monitoring
		if (frameCount % 300 == 0) { // Every ~5 seconds at 60 FPS
			DEBUG_LOG_MEMORY("game_loop_frame_" + std::to_string(frameCount));
		}
				try {
			/* Get time (in second) at loop beginning */
			double startTime = glfwGetTime();
			
			/* Render here */
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
			glClear(GL_COLOR_BUFFER_BIT);
			
			// Only render gameplay content if gameplay is active
			if (gameplayActive && g_threadManager) {
				// CRASH FIX: Validate thread manager before use
				DEBUG_VALIDATE_PTR(g_threadManager);
				
				// Get current game state from thread manager
				auto gameState = g_threadManager->getGameState();
				
				// Process input and send to thread manager
				float playerMoveX = 0.0f;
				float playerMoveY = 0.0f;
				
				// Check if player entity exists before processing movement input
				float tempX, tempY;
				bool playerExists = getPlayerPosition(tempX, tempY);
				
				if (playerExists) {
					// Get normalized input direction (not multiplied by speed or deltaTime)
					// Check arrow keys for player movement direction
					if (keyPressedStates[GLFW_KEY_UP] || keyPressedStates[GLFW_KEY_W]) {
						playerMoveY += 1.0f;
					}
					if (keyPressedStates[GLFW_KEY_DOWN] || keyPressedStates[GLFW_KEY_S]) {
						playerMoveY -= 1.0f;
					}
					if (keyPressedStates[GLFW_KEY_LEFT] || keyPressedStates[GLFW_KEY_A]) {
						playerMoveX -= 1.0f;
					}
					if (keyPressedStates[GLFW_KEY_RIGHT] || keyPressedStates[GLFW_KEY_D]) {
						playerMoveX += 1.0f;
					}
				}
				// If player doesn't exist, playerMoveX and playerMoveY remain 0.0f
				
				// Check sprint state for player movement
				bool sprint = keyPressedStates[GLFW_KEY_LEFT_SHIFT] || keyPressedStates[GLFW_KEY_RIGHT_SHIFT];
				
				// Set player movement input directly to the player movement manager
				g_threadManager->setPlayerMovementInput(playerMoveX, playerMoveY, sprint);
				
				// Handle spacebar for ICE block placement
				static bool lastSpaceState = false;
				bool currentSpaceState = keyPressedStates[GLFW_KEY_SPACE];
				if (currentSpaceState && !lastSpaceState && playerExists) {
					// Spacebar was just pressed - place ICE block
					placeIceBlockInFront();
				}
				lastSpaceState = currentSpaceState;
				
				// Prepare input arrays for thread manager (debug keys and camera controls only)
				bool debugKeys[10] = {false};
				bool cameraControls[5] = {false};
				
				// Set input state in thread manager (now excludes player movement)
				g_threadManager->setInputState(0.0f, 0.0f, debugKeys, cameraControls);
				
				// Grid positions for rendering - these will be used to map from world to screen
				// We still use the aspect ratio correction from g_startX etc.
				float startX = g_startX;
				float endX = g_endX;
				float startY = g_startY;
				float endY = g_endY;
				
				// Get camera boundaries (camera position is updated by game thread)
				float cameraLeft = gameCamera.getLeft();
				float cameraRight = gameCamera.getRight();
				float cameraBottom = gameCamera.getBottom();
				float cameraTop = gameCamera.getTop();
				
				// Calculate the width and height of the view in world coordinates
				float viewWidth = gameCamera.getWidth();
				float viewHeight = gameCamera.getHeight();
				  
				// Calculate the map grid boundaries in window coordinates for the scissor test
				// Convert from normalized device coordinates (-1 to 1) to window coordinates (0 to windowWidth/Height)
				// First, determine what portion of screen space is occupied by the grid
				float gridScreenWidth = endX - startX;   // Width in NDC space
				float gridScreenHeight = endY - startY;  // Height in NDC space
				
				// Convert NDC coordinates (-1 to 1) to window coordinates (0 to windowWidth/Height)
				// In OpenGL, (0,0) is the bottom-left corner of the window
				int scissorX = (int)((startX + 1.0f) * 0.5f * windowWidth);
				int scissorY = (int)((startY + 1.0f) * 0.5f * windowHeight);
				int scissorWidth = (int)(gridScreenWidth * 0.5f * windowWidth);
				int scissorHeight = (int)(gridScreenHeight * 0.5f * windowHeight);
				  
				// Make sure scissor dimensions are positive (required by OpenGL)
				scissorWidth = scissorWidth > 0 ? scissorWidth : 0;
				scissorHeight = scissorHeight > 0 ? scissorHeight : 0;
				
				// Enable scissor test to hide pixels outside the map grid, but only if the feature is enabled
				if (hideOutsideGrid) {
					glEnable(GL_SCISSOR_TEST);
					glScissor(scissorX, scissorY, scissorWidth, scissorHeight);
				}
				  
				// Draw grid
				if (showGridLines) {
					myEngine.setFlatColor(1.0f, 1.0f, 1.0f); // White color for grid lines
					
					// Draw the grid lines
					glLineWidth(gridLineWidth);
					
					// Draw vertical lines for the visible camera region
					glBegin(GL_LINES);
					for (int i = (int)cameraLeft; i <= (int)cameraRight + 1; i++) {
						// Convert from world coordinates to screen coordinates
						float worldRatio = (float)(i - cameraLeft) / (cameraRight - cameraLeft);
						float screenX = startX + worldRatio * (endX - startX);
						glVertex2f(screenX, startY);
						glVertex2f(screenX, endY);
					}
							
					// Draw horizontal lines for the visible camera region
					for (int i = (int)cameraBottom; i <= (int)cameraTop + 1; i++) {
						// Convert from world coordinates to screen coordinates
						float worldRatio = (float)(i - cameraBottom) / (cameraTop - cameraBottom);
						float screenY = startY + worldRatio * (endY - startY);
						glVertex2f(startX, screenY);
						glVertex2f(endX, screenY);
					}
					glEnd();
				}
				
				// Update block transformations (only when game is not paused)
				if (!g_threadManager->isPaused()) {
					Gameplay::getGameMap().updateBlockTransformations(gameState.deltaTime);
				}
				
				// Draw the blocks (textured squares) on the grid
				// Now we pass the camera boundaries to drawBlocks instead of the GRID_SIZE
				Gameplay::getGameMap().drawBlocks(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop, gameState.deltaTime);
				
				// Reset to default state before drawing elements
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				
				// Draw elements on top of the map tiles (freely placed decorations)
				Gameplay::getElementsManager().drawElements(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop, gameState.deltaTime);
				
				// Draw entity debug paths if enabled
				if (DEBUG_SHOW_PATHS) {
					Gameplay::getEntitiesManager().drawDebugPaths(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop);
				}

				// Draw entity collision radii if collision visualization is enabled
				Gameplay::getEntitiesManager().drawDebugCollisionRadii(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop);

				// Disable scissor test when rendering is complete
				if (hideOutsideGrid) {
					glDisable(GL_SCISSOR_TEST);
				}
						// Check escape key to close the window (but not during active gameplay)
				if (keyPressedStates[GLFW_KEY_X]) {
					if (GAME_STATE == GameState::GAMEPLAY) {
						std::cout << "Cannot quit with X key during active gameplay" << std::endl;
					} else {
						glfwSetWindowShouldClose(window, GLFW_TRUE);
						g_threadManager->setRunning(false);
					}
				}
			} else {
				// Gameplay not active - just show black screen and handle escape key
				if (keyPressedStates[GLFW_KEY_X]) {
					glfwSetWindowShouldClose(window, GLFW_TRUE);
				}			}
					// Check if WIN menu should be displayed (from main thread for proper OpenGL context)
			if (SHOULD_SHOW_WIN_MENU) {
				gameMenus.placeUIElement(UIElementName::WIN_MENU, UIElementPosition::CENTER);
				SHOULD_SHOW_WIN_MENU = false; // Reset flag to prevent repeated calls
				std::cout << "WIN menu displayed from main thread" << std::endl;
			}
			
			// Check if GAME_OVER menu should be displayed (from main thread for proper OpenGL context)
			if (SHOULD_SHOW_GAME_OVER) {
				gameMenus.placeUIElement(UIElementName::GAME_OVER, UIElementPosition::CENTER);
				SHOULD_SHOW_GAME_OVER = false; // Reset flag to prevent repeated calls
				std::cout << "GAME_OVER menu displayed from main thread" << std::endl;
			}
			
			// Always render the UI/menus on top of everything else
			// This ensures menus are visible regardless of gameplay state
			gameMenus.render();
			
		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		/* Elapsed time computation from loop beginning */
		double elapsedTime = glfwGetTime() - startTime;
		/* If too little time is spent vs our wanted FPS, we wait */
		while(elapsedTime < FRAMERATE_IN_SECONDS)
		{
			glfwWaitEventsTimeout(FRAMERATE_IN_SECONDS - elapsedTime);
			elapsedTime = glfwGetTime() - startTime;
		}
		
		} catch (const std::exception& e) {
			std::cerr << "CRASH FIX: Exception in game loop frame " << frameCount << ": " << e.what() << std::endl;
			DEBUG_LOG_MEMORY("game_loop_exception");
			// Continue running unless it's a critical error
		} catch (...) {
			std::cerr << "CRASH FIX: Unknown exception in game loop frame " << frameCount << std::endl;
			DEBUG_LOG_MEMORY("game_loop_unknown_exception");
			// Continue running unless it's a critical error
		}
	}
		std::cout << "=== GAME SHUTDOWN INITIATED ===" << std::endl;
	DEBUG_LOG_MEMORY("shutdown_start");
	
	// Cleanup gameplay systems (threads, entities, etc.)
	Gameplay::cleanup();

    // Cleanup input system
    std::cout << "Cleaning up input system..." << std::endl;
    try {
        cleanupInputs();
        DEBUG_LOG_MEMORY("inputs_cleaned");
    } catch (const std::exception& e) {
        std::cerr << "CRASH FIX: Exception cleaning up inputs: " << e.what() << std::endl;
    }
    
    std::cout << "Terminating GLFW..." << std::endl;
    glfwTerminate();
    
    DEBUG_LOG_MEMORY("shutdown_complete");
    std::cout << "=== GAME SHUTDOWN COMPLETE ===" << std::endl;
    return 0;
      } catch (const std::exception& e) {        std::cerr << "CRASH FIX: Unhandled exception in main: " << e.what() << std::endl;
        DEBUG_LOG_MEMORY("main_exception");
        
        // Emergency cleanup
        std::cout << "Performing emergency cleanup..." << std::endl;
        try {
            Gameplay::cleanup();
            cleanupInputs();
            glfwTerminate();
        } catch (...) {
            std::cerr << "Exception during emergency cleanup" << std::endl;
        }
        
        std::cout << "Emergency cleanup completed" << std::endl;
        return -1;
        
    } catch (...) {        std::cerr << "CRASH FIX: Unknown exception caught in main!" << std::endl;
        DEBUG_LOG_MEMORY("main_unknown_exception");
        
        // Emergency cleanup
        std::cout << "Performing emergency cleanup..." << std::endl;
        try {
            Gameplay::cleanup();
            cleanupInputs();
            glfwTerminate();
        } catch (...) {
            std::cerr << "Exception during emergency cleanup" << std::endl;
        }
        
        std::cout << "Emergency cleanup completed" << std::endl;
        return -1;
    }
}
