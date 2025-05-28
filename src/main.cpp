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
#include "camera.h" // Added include for camera management
#include "globals.h" // Added include for global variables
#include "collision.h" // Added include for collision detection
#include "debug.h" // Added include for debugging features
#include "entities.h" // Added include for entity management
#include "inputs.h" // Added include for input handling
#include "threading.h" // Added include for threading system
#include "crashDebug.h" // Added include for crash debugging
#include <ctime> // For time(0) to seed random number generator
#include <cmath> // For sqrt function
#include <algorithm> // For std::min and std::max
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::seconds


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

/* Window close callback function */
void onWindowClose(GLFWwindow* window) {
    std::cout << "Window close callback triggered - starting cleanup..." << std::endl;
    
    // Signal thread manager to stop
    if (g_threadManager) {
        g_threadManager->setRunning(false);
    }
    
    // Shutdown async pathfinding system
    entitiesManager.shutdownAsyncPathfinding();
    
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
    GLFWwindow* window = glfwCreateWindow(1024, 1024, "Digger 2D Game", nullptr, nullptr);
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
	}
      // Initialize Rendering Engine first
	myEngine.initGL();
	
	// Then call the resize callback to set up the correct projection based on window size
    onWindowResize(window, windowWidth, windowHeight);
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
	
	// Only print final elements list before game loop
	// std::cout << "\n--- Elements after player creation ---" << std::endl;
	// elementsManager.listElements();
		// Generate the terrain first - this will be our base map
	std::cout << "Generating terrain..." << std::endl;
	std::map<std::pair<int, int>, TextureName> generatedMap = generateTerrain(GRID_SIZE, GRID_SIZE, islandFeatureSize, seaFeatureSize, 0.55f, 0.65f);
	// Apply the generated terrain - this is more efficient than placing blocks and then overwriting them
	std::cout << "Placing generated terrain..." << std::endl;
	gameMap.placeBlocks(generatedMap);	
	
	// Now add a few specific blocks (only if you need to override the terrain)
	// These are placed after the terrain generation, so they will override any blocks at the same positions
	/* std::map<std::pair<int, int>, TextureName> specialBlocks;
	specialBlocks[{0, 0}] = TextureName::SAND; // Special block at top-left
	specialBlocks[{5, 5}] = TextureName::SAND; // Special block at position
	gameMap.placeBlocks(specialBlocks);
		// Update the generated map with special blocks to keep it in sync with the actual map
	for (const auto& pair : specialBlocks) {
		generatedMap[pair.first] = pair.second;
	} */
	
	std::cout << "Map generation complete." << std::endl;
		// Automatically place terrain elements (bushes on sand blocks) with 1/50 chance
	// Instead of creating a separate map, we'll use the gameMap directly to ensure we see the actual blocks
	placeTerrainElements(elementsManager, gameMap, GRID_SIZE, GRID_SIZE);    
    
    // Initialize entity configurations from predefined types in entities.cpp
   

   

/* 
    elementsManager.placeElement("bush0",ElementTextureName::COCONUT_TREE_1,10.0f,0,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);

    elementsManager.placeElement("bush1",ElementTextureName::COCONUT_TREE_1,10.0f,1,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush2",ElementTextureName::COCONUT_TREE_1,10.0f,2,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush3",ElementTextureName::COCONUT_TREE_1,10.0f,3,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush4",ElementTextureName::COCONUT_TREE_1,10.0f,4,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush5",ElementTextureName::COCONUT_TREE_1,10.0f,5,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush6",ElementTextureName::COCONUT_TREE_1,10.0f,6,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush7",ElementTextureName::COCONUT_TREE_1,10.0f,7,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush8",ElementTextureName::COCONUT_TREE_1,10.0f,8,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush9",ElementTextureName::COCONUT_TREE_1,10.0f,9,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush10",ElementTextureName::COCONUT_TREE_1,10.0f,10,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush11",ElementTextureName::COCONUT_TREE_1,10.0f,11,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush12",ElementTextureName::COCONUT_TREE_1,10.0f,12,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush13",ElementTextureName::COCONUT_TREE_1,10.0f,13,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush14",ElementTextureName::COCONUT_TREE_1,10.0f,14,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush15",ElementTextureName::COCONUT_TREE_1,10.0f,15,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush16",ElementTextureName::COCONUT_TREE_1,10.0f,16,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush17",ElementTextureName::COCONUT_TREE_1,10.0f,17,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush18",ElementTextureName::COCONUT_TREE_1,10.0f,18,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);

    elementsManager.placeElement("bush19",ElementTextureName::COCONUT_TREE_1,10.0f,19,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush20",ElementTextureName::COCONUT_TREE_1,10.0f,20,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush21",ElementTextureName::COCONUT_TREE_1,10.0f,21,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    
    elementsManager.placeElement("bush22",ElementTextureName::COCONUT_TREE_1,10.0f,22,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush23",ElementTextureName::COCONUT_TREE_1,10.0f,23,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush24",ElementTextureName::COCONUT_TREE_1,10.0f,24,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush25",ElementTextureName::COCONUT_TREE_1,10.0f,25,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush26",ElementTextureName::COCONUT_TREE_1,10.0f,26,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush27",ElementTextureName::COCONUT_TREE_1,10.0f,27,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush28",ElementTextureName::COCONUT_TREE_1,10.0f,28,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush29",ElementTextureName::COCONUT_TREE_1,10.0f,29,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush30",ElementTextureName::COCONUT_TREE_1,10.0f,30,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush31",ElementTextureName::COCONUT_TREE_1,10.0f,31,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush32",ElementTextureName::COCONUT_TREE_1,10.0f,32,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush33",ElementTextureName::COCONUT_TREE_1,10.0f,33,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush34",ElementTextureName::COCONUT_TREE_1,10.0f,34,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush35",ElementTextureName::COCONUT_TREE_1,10.0f,35,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush36",ElementTextureName::COCONUT_TREE_1,10.0f,36,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush37",ElementTextureName::COCONUT_TREE_1,10.0f,37,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush38",ElementTextureName::COCONUT_TREE_1,10.0f,38,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush39",ElementTextureName::COCONUT_TREE_1,10.0f,39,40,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);

    elementsManager.placeElement("bush40",ElementTextureName::COCONUT_TREE_1,10.0f,39,42,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush41",ElementTextureName::COCONUT_TREE_1,10.0f,39,43,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush42",ElementTextureName::COCONUT_TREE_1,10.0f,39,44,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush43",ElementTextureName::COCONUT_TREE_1,10.0f,39,45,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush44",ElementTextureName::COCONUT_TREE_1,10.0f,39,46,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush45",ElementTextureName::COCONUT_TREE_1,10.0f,39,47,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush46",ElementTextureName::COCONUT_TREE_1,10.0f,39,48,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush47",ElementTextureName::COCONUT_TREE_1,10.0f,39,49,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush48",ElementTextureName::COCONUT_TREE_1,10.0f,39,50,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);

    elementsManager.placeElement("bush40",ElementTextureName::COCONUT_TREE_1,10.0f,11,42,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush41",ElementTextureName::COCONUT_TREE_1,10.0f,11,43,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush42",ElementTextureName::COCONUT_TREE_1,10.0f,11,44,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush43",ElementTextureName::COCONUT_TREE_1,10.0f,11,45,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush44",ElementTextureName::COCONUT_TREE_1,10.0f,11,46,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush45",ElementTextureName::COCONUT_TREE_1,10.0f,11,47,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush46",ElementTextureName::COCONUT_TREE_1,10.0f,11,48,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush47",ElementTextureName::COCONUT_TREE_1,10.0f,11,49,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);
    elementsManager.placeElement("bush48",ElementTextureName::COCONUT_TREE_1,10.0f,11,50,0.0f,0,0,false,10.0f,AnchorPoint::USE_TEXTURE_DEFAULT,0.0f,0.0f);

    gameMap.placeBlockArea(TextureName::WATER_3, 0, 33, 50, 33);
    gameMap.placeBlockArea(TextureName::GRASS_2, 5, 33, 10, 33); */


    DEBUG_LOG_MEMORY("map_initialization_complete");
    
    // Place the antagonist entity at coordinates (10, 10)
    std::cout << "Initializing entities..." << std::endl;
    entitiesManager.initializeEntityConfigurations();
    DEBUG_LOG_MEMORY("entity_configs_initialized");

    entitiesManager.placeEntityByType("antagonist1", "antagonist", 5.0f, 30.0f);
    entitiesManager.placeEntityByType("antagonist2", "antagonist", 6.0f, 30.0f);
    entitiesManager.placeEntityByType("antagonist3", "antagonist", 7.0f, 30.0f);

    entitiesManager.placeEntityByType("player1", "player", 5.0f, 45.0f);;
    
    DEBUG_LOG_MEMORY("entities_placed");

    // wait 2 seconds
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // entitiesManager.moveEntity("antagonist1",10.0f, 48.0f); // Move the antagonist entity to a new position


	// Only show elements count rather than full list for cleaner output
	std::cout << "Game ready with " << elementsManager.getElementsCount() << " elements placed" << std::endl;
	
	// Display brief coordinate system information
	std::cout << "\nGame uses a coordinate system with (0,0) at bottom-left, Y increasing upward" << std::endl;    // Initialize threading system
    DEBUG_LOG_MEMORY("before_threading_init");
    std::cout << "Initializing threading system..." << std::endl;
    
    if (!initializeThreading(&gameMap, &elementsManager, &entitiesManager, &gameCamera)) {
        std::cerr << "CRASH FIX: Failed to initialize threading system!" << std::endl;
        DEBUG_LOG_MEMORY("threading_init_failed");
        glfwTerminate();
        return -1;
    }
    
    DEBUG_LOG_MEMORY("after_threading_init");
    
    // Start game threads
    std::cout << "Starting game threads..." << std::endl;
    startGameThreads();
    
    DEBUG_LOG_MEMORY("after_threads_started");
    std::cout << "Threading system started - entering render loop" << std::endl;
	/* Main render loop - runs until user closes the window */
	int frameCount = 0;
	while (!glfwWindowShouldClose(window) && g_threadManager->isRunning())
	{
		frameCount++;
		
		// CRASH FIX: Add periodic memory monitoring
		if (frameCount % 300 == 0) { // Every ~5 seconds at 60 FPS
			DEBUG_LOG_MEMORY("game_loop_frame_" + std::to_string(frameCount));
		}
		
		try {
			/* Get time (in second) at loop beginning */
			double startTime = glfwGetTime();
			
			// CRASH FIX: Validate thread manager before use
			DEBUG_VALIDATE_PTR(g_threadManager);
			
			// Get current game state from thread manager
			auto gameState = g_threadManager->getGameState();
        
        // Process input and send to thread manager
        float playerMoveX = 0.0f;
        float playerMoveY = 0.0f;
        processPlayerMovement(gameState.deltaTime, playerMoveX, playerMoveY);
        
        // Prepare input arrays for thread manager
        bool debugKeys[10] = {false};
        bool cameraControls[5] = {false};
        
        // Set input state in thread manager
        g_threadManager->setInputState(playerMoveX, playerMoveY, debugKeys, cameraControls);

        /* Render here */
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
		glClear(GL_COLOR_BUFFER_BIT);
        
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
		
		// Draw the blocks (textured squares) on the grid
		// Now we pass the camera boundaries to drawBlocks instead of the GRID_SIZE
		gameMap.drawBlocks(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop, gameState.deltaTime);
		
		// Reset to default state before drawing elements
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		// Draw elements on top of the map tiles (freely placed decorations)
		elementsManager.drawElements(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop, gameState.deltaTime);
		
        // Draw entity debug paths if enabled
        if (DEBUG_SHOW_PATHS) {
            entitiesManager.drawDebugPaths(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop);
        }

        // Draw entity collision radii if collision visualization is enabled
        entitiesManager.drawDebugCollisionRadii(startX, endX, startY, endY, cameraLeft, cameraRight, cameraBottom, cameraTop);

        // Disable scissor test when rendering is complete
        if (hideOutsideGrid) {
            glDisable(GL_SCISSOR_TEST);
        }
        
        // Check escape key to close the window
        if (keyPressedStates[GLFW_KEY_ESCAPE]) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            g_threadManager->setRunning(false);
        }
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
		// Stop game threads
    std::cout << "Stopping threads..." << std::endl;
    try {
        stopGameThreads();
        DEBUG_LOG_MEMORY("threads_stopped");
    } catch (const std::exception& e) {
        std::cerr << "CRASH FIX: Exception stopping threads: " << e.what() << std::endl;
    }
    
    // Cleanup threading system
    std::cout << "Cleaning up threading system..." << std::endl;
    try {
        cleanupThreading();
        DEBUG_LOG_MEMORY("threading_cleaned");
    } catch (const std::exception& e) {
        std::cerr << "CRASH FIX: Exception cleaning up threading: " << e.what() << std::endl;
    }

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
      } catch (const std::exception& e) {
        std::cerr << "CRASH FIX: Unhandled exception in main: " << e.what() << std::endl;
        DEBUG_LOG_MEMORY("main_exception");
        
        // Emergency cleanup
        std::cout << "Performing emergency cleanup..." << std::endl;
        try {
            if (g_threadManager) {
                g_threadManager->setRunning(false);
                stopGameThreads();
                cleanupThreading();
            }
            cleanupInputs();
            glfwTerminate();
        } catch (...) {
            std::cerr << "Exception during emergency cleanup" << std::endl;
        }
        
        std::cout << "Emergency cleanup completed" << std::endl;
        return -1;
        
    } catch (...) {
        std::cerr << "CRASH FIX: Unknown exception caught in main!" << std::endl;
        DEBUG_LOG_MEMORY("main_unknown_exception");
        
        // Emergency cleanup
        std::cout << "Performing emergency cleanup..." << std::endl;
        try {
            if (g_threadManager) {
                g_threadManager->setRunning(false);
                stopGameThreads();
                cleanupThreading();
            }
            cleanupInputs();
            glfwTerminate();
        } catch (...) {
            std::cerr << "Exception during emergency cleanup" << std::endl;
        }
        
        std::cout << "Emergency cleanup completed" << std::endl;
        return -1;
    }
}
