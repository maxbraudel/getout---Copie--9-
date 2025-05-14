#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "glbasimac/glbi_engine.hpp"
#include <iostream>
#include <vector>
#include "map.h"
#include "terrainGeneration.h"
#include "elementsOnMap.h" // Added include for element management
#include <ctime> // For time(0) to seed random number generator


using namespace glbasimac;

/* Minimal time wanted between two images */
static const double FRAMERATE_IN_SECONDS = 1. / 60.; // Changed to 60 FPS
static float aspectRatio = 1.0f;

/* Grid properties */
static const int GRID_SIZE = 60;
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

	std::map<std::pair<int, int>, TextureName> generatedMap = generateTerrain(GRID_SIZE, GRID_SIZE, islandFeatureSize, seaFeatureSize, 0.55f, 1.0f);
	gameMap.placeBlocks(generatedMap);

	// Place decorative elements on the map
	elementsManager.placeElement("bush1", ElementTextureName::BUSH, 1.0f, 10.0f, 10.0f); // Centered in cell (0,0)

	elementsManager.placeElement("bush2", ElementTextureName::BUSH, 0.75f, 5.2f, 3.8f); // Another bush with different scale

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Get time (in second) at loop beginning */
		double startTime = glfwGetTime();		/* Render here */
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
            glEnd();
        }
				// Draw the blocks (textured squares) on the grid
		// Get time for animations
        double currentTime = glfwGetTime();
        static double lastTime = currentTime;
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

		// Draw the map blocks
		gameMap.drawBlocks(startX, endX, startY, endY, GRID_SIZE, deltaTime);
		
		// Reset to default state before drawing elements
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		// Draw elements on top of the map tiles (freely placed decorations)
		elementsManager.drawElements(startX, endX, startY, endY, GRID_SIZE);

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
