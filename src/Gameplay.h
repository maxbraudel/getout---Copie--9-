#pragma once

#include "glbasimac/glbi_engine.hpp"

// Forward declarations
struct GLFWwindow;
class Map;
class ElementsOnMap;
class EntitiesManager;
class Camera;

/**
 * Gameplay module responsible for initializing and managing game-specific functionality.
 * This separates game logic initialization from main window/rendering setup.
 */
class Gameplay {
public:
    /**
     * Initialize all gameplay systems including map, entities, elements, and threading.
     * @param engine Reference to the OpenGL engine
     * @param window GLFW window pointer for input/threading setup
     * @return true if initialization successful, false otherwise
     */
    static bool initialize(glbasimac::GLBI_Engine& engine, GLFWwindow* window);
    
    /**
     * Cleanup all gameplay systems in proper order.
     * Should be called before application shutdown.
     */
    static void cleanup();
    
    /**
     * Start all game threads after initialization is complete.
     * @return true if threads started successfully, false otherwise
     */
    static bool startGameThreads();
      /**
     * Stop all game threads and perform thread cleanup.
     */
    static void stopGameThreads();
    
    /**
     * Get access to the global game map instance.
     * @return Reference to the global Map instance
     */
    static Map& getGameMap();
    
    /**
     * Get access to the global elements manager instance.
     * @return Reference to the global ElementsOnMap instance
     */
    static ElementsOnMap& getElementsManager();
    
    /**
     * Get access to the global entities manager instance.
     * @return Reference to the global EntitiesManager instance
     */
    static EntitiesManager& getEntitiesManager();

private:
    /**
     * Initialize the game map with terrain generation.
     * @param engine Reference to the OpenGL engine
     * @return true if map initialization successful, false otherwise
     */
    static bool initializeMap(glbasimac::GLBI_Engine& engine);
    
    /**
     * Initialize the elements manager and place terrain elements.
     * @param engine Reference to the OpenGL engine
     * @return true if elements initialization successful, false otherwise
     */
    static bool initializeElements(glbasimac::GLBI_Engine& engine);
      /**
     * Initialize entity configurations only (before terrain generation).
     * @return true if entity configurations initialized successfully, false otherwise
     */
    static bool initializeEntityConfigurations();
    
    /**
     * Place initial entities after terrain and elements are ready.
     * @return true if entity placement successful, false otherwise
     */
    static bool placeInitialEntities();
    
    /**
     * Initialize the threading system with all game managers.
     * @return true if threading initialization successful, false otherwise
     */
    static bool initializeThreading();
    
    // Static flags to track initialization state
    static bool s_mapInitialized;
    static bool s_elementsInitialized;
    static bool s_entitiesInitialized;
    static bool s_threadingInitialized;
    static bool s_threadsStarted;
};
