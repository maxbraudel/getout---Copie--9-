#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "glbasimac/glbi_engine.hpp"
#include <iostream>
#include <vector>
#include "map.h"
#include "terrainGeneration.h"
#include "elementsOnMap.h" // Added include for element management
#include "player.h" // Added include for player management
#include <ctime> // For time(0) to seed random number generator
#include <cmath> // For sqrt function


using namespace glbasimac;

/* Minimal time wanted between two images */
static const double FRAMERATE_IN_SECONDS = 1. / 60.; // Changed to 60 FPS
static float aspectRatio = 1.0f;

/* Grid properties */
static const int GRID_SIZE = 170;
static float islandFeatureSize = 0.1f; // Controls the size of islands. Smaller values = smaller, more numerous islands. Larger values = larger, fewer islands.
static float seaFeatureSize = 0.016f; // Controls the size of sea areas. Larger values = larger sea areas.
static float gridLineWidth = 1.0f;
static int windowWidth = 2024;
static int windowHeight = 2024;

/* Camera properties */
static const float CAMERA_REGION = 20.0f; // Size of the visible region around the player (in grid units)

// Variables to store grid rendering parameters for coordinate conversion
static float g_startX = -1.0f;
static float g_endX = 1.0f;
static float g_startY = -1.0f;
static float g_endY = 1.0f;

// Add this boolean variable to control grid line visibility
static bool showGridLines = false;

/* OpenGL Engine */
GLBI_Engine myEngine;

/* Error handling function */
void onError(int error, const char* description) {
	std::cout << "GLFW Error ("<<error<<") : " << description << std::endl;
}

/* Window resize callback function */
void onWindowResize(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    
    // Calculate grid size to maintain aspect ratio and fit within window
    float gridSize, offsetX = 0.0f, offsetY = 0.0f;
    
    if (windowWidth >= windowHeight) {
        // Window is wider than tall, fit to height
        gridSize = 2.0f; // Use full height (-1 to 1)
        float gridWidth = gridSize * windowHeight / windowWidth; // Adjust width to maintain aspect ratio
        float gridHeight = gridSize;
        offsetX = (2.0f - gridWidth) / 2.0f; // Center horizontally
        offsetY = 0.0f;
    } else {
        // Window is taller than wide, fit to width
        gridSize = 2.0f; // Use full width (-1 to 1)
        float gridWidth = gridSize;
        float gridHeight = gridSize * windowWidth / windowHeight; // Adjust height to maintain aspect ratio
        offsetX = 0.0f;
        offsetY = (2.0f - gridHeight) / 2.0f; // Center vertically
    }
    
    // Update grid rendering parameters for coordinate conversion
    g_startX = -1.0f + offsetX;
    g_endX = 1.0f - offsetX;
    g_startY = -1.0f + offsetY;
    g_endY = 1.0f - offsetY;
}

// Constants for gameplay
const float PLAYER_BASE_SPEED = 4.0f;   // Base speed of player movement (grid units per second)
const float PLAYER_SPRINT_SPEED = 6.0f; // Sprint speed when shift is held (grid units per second)

// Global variables for input handling
bool keyPressedStates[GLFW_KEY_LAST + 1] = { false };

// Keyboard callback function
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Update key state arrays
    if (action == GLFW_PRESS) {
        keyPressedStates[key] = true;
        
        // Handle immediate keyboard actions
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE); // Exit on ESC press
        } 
        // Reset player position
        else if (key == GLFW_KEY_R) {
            teleportPlayer(10.0f, 10.0f);
            std::cout << "Player position reset to (10, 10)" << std::endl;
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
        }        // List all elements when F4 is pressed
        else if (key == GLFW_KEY_F4) {
            std::cout << "\n--- Current Elements List ---" << std::endl;
            elementsManager.listElements();
        }
        // Print detailed element positions when F6 is pressed
        else if (key == GLFW_KEY_F6) {
            elementsManager.printElementPositions();
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
          // COORDINATE SYSTEM: The game now uses a coordinate system where (0,0) is at bottom-left
        // and Y increases upward, matching mathematical conventions.
        
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
              // Calculate grid Y coordinate
            // Our grid now has (0,0) at bottom-left, same as OpenGL
            // Normalized y = -1.0 should map to grid y = 0, and normalized y = 1.0 should map to grid y = GRID_SIZE-1
            int gridYInt = static_cast<int>(gridY);
              // Reduced debug output - only show the essential information
            // std::cout << "Debug: Mouse at normalized (" << normalizedX << ", " << normalizedY << ")" << std::endl;
            // std::cout << "Debug: Grid area from (" << g_startX << "," << g_startY << ") to (" 
            //           << g_endX << "," << g_endY << ")" << std::endl;            // Calculate world position for reference (but don't print)
            float worldX = gridXInt + 0.5f;  // Center of the block
            float worldY = gridYInt + 0.5f;  // Center of the block
            
            // Get block name at these coordinates
            TextureName blockName = gameMap.getBlockNameByCoordinates(gridXInt, gridYInt);
            
            // Convert enum to string for better output
            std::string blockNameStr = "UNKNOWN";
            switch (blockName) {
                case TextureName::GRASS_0: blockNameStr = "GRASS_0"; break;
                case TextureName::GRASS_1: blockNameStr = "GRASS_1"; break;
                case TextureName::GRASS_2: blockNameStr = "GRASS_2"; break;
                case TextureName::GRASS_3: blockNameStr = "GRASS_3"; break;
                case TextureName::GRASS_4: blockNameStr = "GRASS_4"; break;
                case TextureName::GRASS_5: blockNameStr = "GRASS_5"; break;
                case TextureName::SAND: blockNameStr = "SAND"; break;
                case TextureName::WATER_0: blockNameStr = "WATER_0"; break;
                case TextureName::WATER_1: blockNameStr = "WATER_1"; break;
                case TextureName::WATER_2: blockNameStr = "WATER_2"; break;
                case TextureName::WATER_3: blockNameStr = "WATER_3"; break;
                case TextureName::WATER_4: blockNameStr = "WATER_4"; break;
                default: blockNameStr = "UNKNOWN"; break;
            }
            
            // Output the block information
            std::cout << "Clicked on grid cell: (" << gridXInt << ", " << gridYInt << ")" << std::endl;
            std::cout << "Block type: " << blockNameStr << " (enum value: " << static_cast<int>(blockName) << ")" << std::endl;
        }
    }
}

int main() {
    // Seed the random number generator ONCE at the start of the program
    // You can change time(0) to a specific unsigned int for a fixed, repeatable map.
    unsigned int terrainSeed = static_cast<unsigned int>(time(0));
    // unsigned int terrainSeed = 12345; // Example of a fixed seed
    srand(terrainSeed);

    // Initialize the library
    if (!glfwInit()) {
        return -1;
    }    /* Callback to a function if an error is rised by GLFW */
	glfwSetErrorCallback(onError);
    
    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(1024, 1024, "Digger 2D Game", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }    // Make the window's context current
    glfwMakeContextCurrent(window);    // Set the window resize callback
    glfwSetWindowSizeCallback(window, onWindowResize);
    
    // Set the keyboard callback
    glfwSetKeyCallback(window, keyCallback);
    
    // Set the mouse button callback
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
      // Get initial window size
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    
	// Intialize glad (loads the OpenGL functions)
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		return -1;
	}
    
    // Call the resize callback once to initialize grid parameters
    onWindowResize(window, windowWidth, windowHeight);// Initialize Rendering Engine
	myEngine.initGL();
	
	// Setup 2D projection for our grid - mapping -1 to 1 range to the window
	myEngine.set2DProjection(-1.0f, 1.0f, -1.0f, 1.0f);
		// Initialize our game map
	if (!gameMap.init(myEngine)) {
		std::cerr << "Failed to initialize map!" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Initialize the elements manager
	if (!elementsManager.init(myEngine)) {
		std::cerr << "Failed to initialize elements manager!" << std::endl;
		glfwTerminate();
		return -1;
	}
		// Create a single player character at position (10, 10)
	createPlayer(10.0f, 10.0f);
	
	// Only print final elements list before game loop
	// std::cout << "\n--- Elements after player creation ---" << std::endl;
	// elementsManager.listElements();
		// Generate the terrain first - this will be our base map
	std::cout << "Generating terrain..." << std::endl;
	std::map<std::pair<int, int>, TextureName> generatedMap = generateTerrain(GRID_SIZE, GRID_SIZE, islandFeatureSize, seaFeatureSize, 0.55f, 0.7f);
	// Apply the generated terrain - this is more efficient than placing blocks and then overwriting them
	std::cout << "Placing generated terrain..." << std::endl;
	gameMap.placeBlocks(generatedMap);	
	
	// Now add a few specific blocks (only if you need to override the terrain)
	// These are placed after the terrain generation, so they will override any blocks at the same positions
	std::map<std::pair<int, int>, TextureName> specialBlocks;
	specialBlocks[{0, 0}] = TextureName::SAND; // Special block at top-left
	specialBlocks[{5, 5}] = TextureName::SAND; // Special block at position
	gameMap.placeBlocks(specialBlocks);
		// Update the generated map with special blocks to keep it in sync with the actual map
	for (const auto& pair : specialBlocks) {
		generatedMap[pair.first] = pair.second;
	}
	
	std::cout << "Map generation complete." << std::endl;
		// Automatically place terrain elements (bushes on sand blocks) with 1/50 chance
	// Instead of creating a separate map, we'll use the gameMap directly to ensure we see the actual blocks
	placeTerrainElements(elementsManager, gameMap, GRID_SIZE, GRID_SIZE);
	// Place additional decorative elements on the map with unique names using texture-defined anchor points
  	// Note: The coordinate system has (0,0) at bottom-left, with Y increasing upward
	elementsManager.placeElement("test1", ElementTextureName::COCONUT_TREE_1, 5.0f, 1.0f, 1.0f,
                               0.0f, 0, 0, false, 10.0f); // Using default anchor point from texture
    
    // Demonstrate moving an element
    elementsManager.changeElementCoordinates("bush2", 30.0f, 30.0f);  // Move bush2 to a new position
		// Only show elements count rather than full list for cleaner output
	std::cout << "Game ready with " << elementsManager.getElementsCount() << " elements placed" << std::endl;
		// Skip block test output for cleaner console
	          	// Display brief coordinate system information
	std::cout << "\nGame uses a coordinate system with (0,0) at bottom-left, Y increasing upward" << std::endl;
	
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Get time (in second) at loop beginning */
		double startTime = glfwGetTime();
		
		// Get time for animations
        double currentTime = glfwGetTime();
        static double lastTime = currentTime;
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
		
		// Process keyboard input for player movement
		float playerMoveX = 0.0f;
		float playerMoveY = 0.0f;
		
			// First check if the player exists before processing movements
		float playerX, playerY;
		bool playerExists = elementsManager.getElementPosition("player1", playerX, playerY);
		
		if (!playerExists) {
			std::cerr << "WARNING: Player doesn't exist in movement processing! Listing elements:" << std::endl;
			elementsManager.listElements();
			std::cerr << "Creating new player since the original one is missing..." << std::endl;
			createPlayer(10.0f, 10.0f);
			playerExists = true;
		}
		
		// Apply speed based on whether shift is held
		float currentSpeed = PLAYER_BASE_SPEED;
		if (keyPressedStates[GLFW_KEY_LEFT_SHIFT] || keyPressedStates[GLFW_KEY_RIGHT_SHIFT]) {
			currentSpeed = PLAYER_SPRINT_SPEED; // Use sprint speed when shift is held
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
		
		// Track if player is moving
		static bool wasMoving = false;
		bool isMoving = (playerMoveX != 0.0f || playerMoveY != 0.0f);
				// Move the player if any movement key is pressed
		if (isMoving) {
			// If player just started moving, print debug message
			if (!wasMoving) {
				std::cout << "Player started moving" << std::endl;
			}
			movePlayer(playerMoveX, playerMoveY);
		}// If player just stopped moving, disable animation and set to standing frame
		else if (wasMoving) {
			std::cout << "Player stopped moving - disabling animation" << std::endl;
			elementsManager.changeElementAnimationStatus("player1", false);
			elementsManager.changeElementSpriteFrame("player1", 0); // Set to standing frame
		}
				// Update movement state
		wasMoving = isMoving;
        
        /* Render here */
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
		glClear(GL_COLOR_BUFFER_BIT);
          // Get the player position for camera centering
        // Reusing playerX and playerY variables that were declared earlier
        getPlayerPosition(playerX, playerY);
        
        // Calculate camera view boundaries in world coordinates
        float cameraLeft = playerX - CAMERA_REGION;
        float cameraRight = playerX + CAMERA_REGION;
        float cameraBottom = playerY - CAMERA_REGION;
        float cameraTop = playerY + CAMERA_REGION;
        
        // Ensure the camera doesn't go out of bounds
        if (cameraLeft < 0) {
            cameraLeft = 0;
            cameraRight = CAMERA_REGION * 2;
        }
        if (cameraRight > GRID_SIZE) {
            cameraRight = GRID_SIZE;
            cameraLeft = GRID_SIZE - CAMERA_REGION * 2;
        }
        if (cameraBottom < 0) {
            cameraBottom = 0;
            cameraTop = CAMERA_REGION * 2;
        }
        if (cameraTop > GRID_SIZE) {
            cameraTop = GRID_SIZE;
            cameraBottom = GRID_SIZE - CAMERA_REGION * 2;
        }
          // Grid positions for rendering - these will be used to map from world to screen
        // We still use the aspect ratio correction from g_startX etc.
        float startX = g_startX;
        float endX = g_endX;
        float startY = g_startY;
        float endY = g_endY;        // Calculate the width and height of the view in world coordinates
        float viewWidth = cameraRight - cameraLeft;
        float viewHeight = cameraTop - cameraBottom;
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
        
        // Enable scissor test to hide pixels outside the map grid
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorX, scissorY, scissorWidth, scissorHeight);
        
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
            glEnd();        }
				// Draw the blocks (textured squares) on the grid
		// Now we pass the camera boundaries to drawBlocks instead of the GRID_SIZE
		gameMap.drawBlocks(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop, deltaTime);
		
		// Reset to default state before drawing elements
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();		// Draw elements on top of the map tiles (freely placed decorations)
		elementsManager.drawElements(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop, deltaTime);
        
        // Disable scissor test when rendering is complete
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_SCISSOR_TEST);
        
		// Check escape key to close the window
        if (keyPressedStates[GLFW_KEY_ESCAPE]) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        
        // Toggle grid lines with G key
        static bool gKeyPressed = false;
        if (keyPressedStates[GLFW_KEY_G] && !gKeyPressed) {
            showGridLines = !showGridLines;
            gKeyPressed = true;
        } else if (!keyPressedStates[GLFW_KEY_G]) {
            gKeyPressed = false;
        }
        
        // Handle special key presses (one-time actions on key press)
        static bool lastFrameDebugKeyState = false;
        if (keyPressedStates[GLFW_KEY_F3] && !lastFrameDebugKeyState) {
            // Toggle debug mode (not currently implemented)
            // Add debug functionality here if needed
        }
        lastFrameDebugKeyState = keyPressedStates[GLFW_KEY_F3];
      // Toggle grid lines with F2
        static bool lastFrameGridKeyState = false;
        if (keyPressedStates[GLFW_KEY_F2] && !lastFrameGridKeyState) {
            showGridLines = !showGridLines;
            std::cout << "Grid lines " << (showGridLines ? "enabled" : "disabled") << std::endl;
        }
        lastFrameGridKeyState = keyPressedStates[GLFW_KEY_F2];
        
        // Toggle anchor point visualization with F5
        static bool lastFrameAnchorPointKeyState = false;
        if (keyPressedStates[GLFW_KEY_F5] && !lastFrameAnchorPointKeyState) {
            elementsManager.toggleAnchorPointVisualization();
        }
        lastFrameAnchorPointKeyState = keyPressedStates[GLFW_KEY_F5];
        
        // Print elements list when F4 is pressed
        static bool lastFrameDebugElementsState = false;
        if (keyPressedStates[GLFW_KEY_F4] && !lastFrameDebugElementsState) {
            std::cout << "\n--- Current Elements List ---" << std::endl;
            elementsManager.listElements();
        }
        lastFrameDebugElementsState = keyPressedStates[GLFW_KEY_F4];

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		/* Elapsed time computation from loop begining */
		double elapsedTime = glfwGetTime() - startTime;
		/* If to few time is spend vs our wanted FPS, we wait */
		while(elapsedTime < FRAMERATE_IN_SECONDS)
		{
			glfwWaitEventsTimeout(FRAMERATE_IN_SECONDS-elapsedTime);
			elapsedTime = glfwGetTime() - startTime;
		}
	}

    glfwTerminate();
    return 0;
}
