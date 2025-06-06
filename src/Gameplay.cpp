#include "Gameplay.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "map.h"
#include "terrainGeneration.h"
#include "elementsOnMap.h"
#include "player.h"
#include "camera.h"
#include "globals.h"
#include "entities.h"
#include "threading.h"
#include "debug.h"
#include "crashDebug.h"
#include <iostream>
#include <ctime>
#include <thread>
#include <chrono>
#include "gameMenus.h" // Include for game menu system
#include "enumDefinitions.h"

// Initialize static members
bool Gameplay::s_mapInitialized = false;
bool Gameplay::s_elementsInitialized = false;
bool Gameplay::s_entitiesInitialized = false;
bool Gameplay::s_threadingInitialized = false;
bool Gameplay::s_threadsStarted = false;

bool Gameplay::initialize(glbasimac::GLBI_Engine& engine, GLFWwindow* window) {
    std::cout << "=== GAMEPLAY INITIALIZATION ===" << std::endl;
    DEBUG_LOG_MEMORY("gameplay_init_start");
      try {
        // Initialize map with terrain generation
        if (!initializeMap(engine)) {
            std::cerr << "Failed to initialize game map!" << std::endl;
            return false;
        }
        
        // Initialize entity configurations BEFORE elements (needed for entity spawning during terrain generation)
        if (!initializeEntityConfigurations()) {
            std::cerr << "Failed to initialize entity configurations!" << std::endl;
            return false;
        }
        
        // Initialize elements manager and place terrain elements
        if (!initializeElements(engine)) {
            std::cerr << "Failed to initialize elements manager!" << std::endl;
            return false;
        }
        
        // Place initial entities
        if (!placeInitialEntities()) {
            std::cerr << "Failed to place initial entities!" << std::endl;
            return false;
        }
        
        // Initialize threading system
        if (!initializeThreading()) {
            std::cerr << "Failed to initialize threading system!" << std::endl;
            return false;
        }
        
        DEBUG_LOG_MEMORY("gameplay_init_complete");
        std::cout << "=== GAMEPLAY INITIALIZATION COMPLETE ===" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during gameplay initialization: " << e.what() << std::endl;
        DEBUG_LOG_MEMORY("gameplay_init_exception");
        return false;
    } catch (...) {
        std::cerr << "Unknown exception during gameplay initialization!" << std::endl;
        DEBUG_LOG_MEMORY("gameplay_init_unknown_exception");
        return false;
    }
}

bool Gameplay::initializeMap(glbasimac::GLBI_Engine& engine) {
    std::cout << "Initializing game map..." << std::endl;
    
    // Initialize our game map
    if (!gameMap.init(engine)) {
        std::cerr << "Failed to initialize map!" << std::endl;
        return false;
    }
    
    // Generate the terrain first - this will be our base map
    std::cout << "Generating terrain..." << std::endl;
    std::map<std::pair<int, int>, BlockName> generatedMap = generateTerrain(
        GRID_SIZE, GRID_SIZE, islandFeatureSize, seaFeatureSize, 0.55f, 0.65f
    );
    
    // Apply the generated terrain - this is more efficient than placing blocks and then overwriting them
    std::cout << "Placing generated terrain..." << std::endl;
    gameMap.placeBlocks(generatedMap);
    
    std::cout << "Map generation complete." << std::endl;
    DEBUG_LOG_MEMORY("map_initialization_complete");
    
    s_mapInitialized = true;
    return true;
}

bool Gameplay::initializeElements(glbasimac::GLBI_Engine& engine) {
    std::cout << "Initializing elements manager..." << std::endl;
    
    // Initialize the elements manager
    if (!elementsManager.init(engine)) {
        std::cerr << "Failed to initialize elements manager!" << std::endl;
        return false;
    }
      // Automatically place terrain elements (bushes on sand blocks) with 1/50 chance
    // Entity configurations are now already initialized, so entity spawning will work
    std::cout << "Placing terrain elements..." << std::endl;
    placeTerrainElements(elementsManager, gameMap, GRID_SIZE, GRID_SIZE);
    
    // Show elements count for confirmation
    std::cout << "Elements initialization complete with " << elementsManager.getElementsCount() << " elements placed" << std::endl;
    
    s_elementsInitialized = true;
    return true;
}

bool Gameplay::initializeEntityConfigurations() {
    std::cout << "Initializing entity configurations..." << std::endl;
    
    // Initialize entity configurations before terrain element placement (needed for entity spawning)
    entitiesManager.initializeEntityConfigurations();
    DEBUG_LOG_MEMORY("entity_configs_initialized");
    
    // Initialize async pathfinding system for entity movement
    std::cout << "Initializing async pathfinding system..." << std::endl;
    entitiesManager.initializeAsyncPathfinding();
    DEBUG_LOG_MEMORY("async_pathfinding_initialized");
    
    std::cout << "Entity configurations and async pathfinding initialized." << std::endl;
    
    s_entitiesInitialized = true;
    return true;
}

bool Gameplay::placeInitialEntities() {
    std::cout << "Placing initial entities..." << std::endl;
    
    // Place sharks
    entitiesManager.placeEntityByTypeSafely("shark1", EntityName::SHARK, 42.0f, 31.0f);
    entitiesManager.placeEntityByTypeSafely("shark2", EntityName::SHARK, 41.0f, 31.0f);
    
    // Place player
    entitiesManager.placeEntityByTypeSafely("player1", EntityName::PLAYER, 5.0f, 45.0f);
    
    DEBUG_LOG_MEMORY("entities_placed");
    
    // Wait 2 seconds to allow entity placement to stabilize
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "Initial entities placed successfully." << std::endl;
    // Display brief coordinate system information
    std::cout << "\nGame uses a coordinate system with (0,0) at bottom-left, Y increasing upward" << std::endl;
    
    return true;
}

bool Gameplay::initializeThreading() {
    std::cout << "Initializing threading system..." << std::endl;
    DEBUG_LOG_MEMORY("before_threading_init");
    
    if (!::initializeThreading(&gameMap, &elementsManager, &entitiesManager, &gameCamera)) {
        std::cerr << "Failed to initialize threading system!" << std::endl;
        DEBUG_LOG_MEMORY("threading_init_failed");
        return false;
    }
    
    DEBUG_LOG_MEMORY("after_threading_init");
    std::cout << "Threading system initialized successfully." << std::endl;
    
    s_threadingInitialized = true;
    return true;
}

bool Gameplay::startGameThreads() {
    if (!s_threadingInitialized) {
        std::cerr << "Cannot start game threads: threading system not initialized!" << std::endl;
        return false;
    }
    
    std::cout << "Starting game threads..." << std::endl;
    
    try {
        ::startGameThreads();
        DEBUG_LOG_MEMORY("after_threads_started");
        std::cout << "Game threads started successfully." << std::endl;
        
        s_threadsStarted = true;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception starting game threads: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown exception starting game threads!" << std::endl;
        return false;
    }
}

void Gameplay::stopGameThreads() {
    if (!s_threadsStarted) {
        std::cout << "Game threads not started, skipping stop." << std::endl;
        return;
    }
    
    std::cout << "Stopping game threads..." << std::endl;
    
    try {
        ::stopGameThreads();
        DEBUG_LOG_MEMORY("threads_stopped");
        std::cout << "Game threads stopped successfully." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception stopping threads: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception stopping threads!" << std::endl;
    }
    
    s_threadsStarted = false;
}

void Gameplay::cleanup() {
    std::cout << "=== GAMEPLAY CLEANUP ===" << std::endl;
    DEBUG_LOG_MEMORY("gameplay_cleanup_start");
    
    try {
        // Stop threads first
        if (s_threadsStarted) {
            stopGameThreads();
        }
        
        // Cleanup threading system
        if (s_threadingInitialized) {
            std::cout << "Cleaning up threading system..." << std::endl;
            ::cleanupThreading();
            DEBUG_LOG_MEMORY("threading_cleaned");
            s_threadingInitialized = false;
        }
        
        // Shutdown async pathfinding system
        if (s_entitiesInitialized) {
            std::cout << "Shutting down entity async pathfinding..." << std::endl;
            entitiesManager.shutdownAsyncPathfinding();
            s_entitiesInitialized = false;
        }
        
        // Mark other systems as cleaned up
        s_elementsInitialized = false;
        s_mapInitialized = false;
        
        DEBUG_LOG_MEMORY("gameplay_cleanup_complete");
        std::cout << "=== GAMEPLAY CLEANUP COMPLETE ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during gameplay cleanup: " << e.what() << std::endl;
        DEBUG_LOG_MEMORY("gameplay_cleanup_exception");
    } catch (...) {
        std::cerr << "Unknown exception during gameplay cleanup!" << std::endl;        DEBUG_LOG_MEMORY("gameplay_cleanup_unknown_exception");
    }
}

Map& Gameplay::getGameMap() {
    return gameMap;
}

ElementsOnMap& Gameplay::getElementsManager() {
    return elementsManager;
}

EntitiesManager& Gameplay::getEntitiesManager() {
    return entitiesManager;
}
