#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "glbasimac/glbi_engine.hpp"
#include <iostream>
#include <vector>
#include "map.h"

using namespace glbasimac;

/* Minimal time wanted between two images */
static const double FRAMERATE_IN_SECONDS = 1. / 60.; // Changed to 60 FPS
static float aspectRatio = 1.0f;

/* Grid properties */
static const int GRID_SIZE = 20;
static float gridLineWidth = 1.0f;
static int windowWidth = 1024;
static int windowHeight = 1024;

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
	
	// Place a sand block at position (0,0) - top left corner

	// Example usage of placeBlocks
	std::map<std::pair<int, int>, GLuint> blocksToPlace;
	blocksToPlace[{1, 0}] = gameMap.getTexture(TextureType::SAND);
	blocksToPlace[{0, 1}] = gameMap.getTexture(TextureType::SAND);
	blocksToPlace[{1, 1}] = gameMap.getTexture(TextureType::SAND);
	blocksToPlace[{5, 5}] = gameMap.getTexture(TextureType::SAND);
	// blocksToPlace[{GRID_SIZE -1, GRID_SIZE -1}] = gameMap.getTexture(TextureType::SAND); // Bottom-right corner

	gameMap.placeBlockArea(gameMap.getTexture(TextureType::WATER), 0, 0, GRID_SIZE -1, GRID_SIZE -1); // Place a block area from (2,2) to (4,4)
	gameMap.placeBlocks(blocksToPlace);

	gameMap.placeBlock(gameMap.getTexture(TextureType::SAND), 0, 0);

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
		
		// Draw the blocks (textured squares) on the grid
		gameMap.drawBlocks(startX, endX, startY, endY, GRID_SIZE);

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
