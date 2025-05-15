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
static const int GRID_SIZE = 70;
static float islandFeatureSize = 0.1f; // Controls the size of islands. Smaller values = smaller, more numerous islands. Larger values = larger, fewer islands.
static float seaFeatureSize = 0.016f; // Controls the size of sea areas. Larger values = larger sea areas.
static float gridLineWidth = 1.0f;
static int windowWidth = 1024;
static int windowHeight = 1024;

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
        }
        // List all elements when F4 is pressed
        else if (key == GLFW_KEY_F4) {
            std::cout << "\n--- Current Elements List ---" << std::endl;
            elementsManager.listElements();
        }
    } else if (action == GLFW_RELEASE) {
        keyPressedStates[key] = false;
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
    
    // Get initial window size
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    
	// Intialize glad (loads the OpenGL functions)
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		return -1;
	}	// Initialize Rendering Engine
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
	
	std::cout << "\n--- Elements before player creation ---" << std::endl;
	elementsManager.listElements();
	
	// Create a single player character at position (10, 10)
	createPlayer(10.0f, 10.0f);
	
	std::cout << "\n--- Elements after player creation ---" << std::endl;
	elementsManager.listElements();
	
	// Place a sand block at position (0,0) - top left corner

	// Example usage of placeBlocks
	std::map<std::pair<int, int>, TextureName> blocksToPlace; // Changed GLuint to TextureName
	blocksToPlace[{1, 0}] = TextureName::SAND;
	blocksToPlace[{0, 1}] = TextureName::SAND;
	blocksToPlace[{1, 1}] = TextureName::SAND;
	blocksToPlace[{5, 5}] = TextureName::SAND;
	// blocksToPlace[{GRID_SIZE -1, GRID_SIZE -1}] = TextureName::SAND; // Bottom-right corner

	// Place an area of animated water
	
	gameMap.placeBlockArea(TextureName::WATER_0, 0, 0,GRID_SIZE -1, GRID_SIZE -1); // Right Column

	gameMap.placeBlockArea(TextureName::SAND, 4, 4,GRID_SIZE -5, GRID_SIZE -5); // Right Column

	gameMap.placeBlocks(blocksToPlace); // Apply the map of blocks

	gameMap.placeBlock(TextureName::SAND, 0, 0); // Overwrite top-left with sand again
	std::map<std::pair<int, int>, TextureName> generatedMap = generateTerrain(GRID_SIZE, GRID_SIZE, islandFeatureSize, seaFeatureSize, 0.55f, 0.7f);
	gameMap.placeBlocks(generatedMap);	// Place decorative elements on the map with unique names and different anchor points
	elementsManager.placeElement("test1", ElementTextureName::TEST, 1.0f, 10.0f, 10.0f,
	                           0.0f, 0, 0, false, 10.0f, AnchorPoint::TOP_RIGHT_CORNER); 
	                           
    elementsManager.placeElement("bush2", ElementTextureName::BUSH, 5.0f, 21.0f, 21.0f,
                               0.0f, 0, 0, false, 10.0f, AnchorPoint::BOTTOM_LEFT_CORNER); 
                               
    elementsManager.placeElement("bush3", ElementTextureName::BUSH, 5.0f, 25.0f, 25.0f,
                               0.0f, 0, 0, false, 10.0f, AnchorPoint::BOTTOM_RIGHT_CORNER);
    
    // Demonstrate moving an element
    elementsManager.changeElementCoordinates("bush2", 30.0f, 30.0f);  // Move bush2 to a new position
	
	// Print elements again to verify state
	std::cout << "\n--- Elements before game loop ---" << std::endl;
	elementsManager.listElements();
	
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
			movePlayer(playerMoveX, playerMoveY);
			
			// If player just started moving, enable animation
			if (!wasMoving) {
				elementsManager.changeElementAnimationStatus("player1", true);
			}
		} 
		// If player just stopped moving, disable animation and set to standing frame
		else if (wasMoving) {
			elementsManager.changeElementAnimationStatus("player1", false);
			elementsManager.changeElementSpriteFrame("player1", 0); // Set to standing frame
		}
		
		// Update movement state
		wasMoving = isMoving;
		
		/* Render here */
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
		glClear(GL_COLOR_BUFFER_BIT);        // Calculate grid size to maintain aspect ratio and fit within window
        float gridSize;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float gridWidth, gridHeight;
        
        if (windowWidth >= windowHeight) {
            // Window is wider than tall, fit to height
            gridSize = 2.0f; // Use full height (-1 to 1)
            gridWidth = gridSize * windowHeight / windowWidth; // Adjust width to maintain aspect ratio
            gridHeight = gridSize;
            offsetX = (2.0f - gridWidth) / 2.0f; // Center horizontally
        } else {
            // Window is taller than wide, fit to width
            gridSize = 2.0f; // Use full width (-1 to 1)
            gridWidth = gridSize;
            gridHeight = gridSize * windowWidth / windowHeight; // Adjust height to maintain aspect ratio
            offsetY = (2.0f - gridHeight) / 2.0f; // Center vertically
        }
        
        // Calculate actual positions of grid in normalized coordinates
        float startX = -1.0f + offsetX;
        float endX = 1.0f - offsetX;
        float startY = -1.0f + offsetY;
        float endY = 1.0f - offsetY;
        
        // Draw grid
        if (showGridLines) {
            myEngine.setFlatColor(1.0f, 1.0f, 1.0f); // White color for grid lines
            
            // Draw the grid lines
            glLineWidth(gridLineWidth);
            
            // Draw vertical lines
            glBegin(GL_LINES);
            for (int i = 0; i <= GRID_SIZE; i++) {
                float ratio = (float)i / GRID_SIZE;
                float x = startX + ratio * (endX - startX);
                glVertex2f(x, startY);
                glVertex2f(x, endY);
            }
                    // Draw horizontal lines
            for (int i = 0; i <= GRID_SIZE; i++) {
                float ratio = (float)i / GRID_SIZE;
                float y = startY + ratio * (endY - startY);
                glVertex2f(startX, y);
                glVertex2f(endX, y);
            }
            glEnd();        }
				// Draw the blocks (textured squares) on the grid
		// Draw the map blocks
		gameMap.drawBlocks(startX, endX, startY, endY, GRID_SIZE, deltaTime);
		
		// Reset to default state before drawing elements
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
				// Draw elements on top of the map tiles (freely placed decorations)
		elementsManager.drawElements(startX, endX, startY, endY, GRID_SIZE, deltaTime);
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
