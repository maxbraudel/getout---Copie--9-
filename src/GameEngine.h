#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <memory>
#include <string>

// Forward declarations
class Map;
class ElementsOnMap;
class EntitiesManager;
class Camera;
class InputManager;
class RenderSystem;
class ThreadManager;
class GameLogic;

/**
 * Main Game Engine class that orchestrates all game systems
 * Follows clean architecture principles with clear separation of concerns
 */
class GameEngine {
public:
    GameEngine();
    ~GameEngine();
    
    // Main engine lifecycle
    bool initialize(int width, int height, const std::string& title);
    void run();
    void shutdown();
    
    // State management
    bool isRunning() const { return m_running; }
    void requestShutdown() { m_running = false; }
    
private:
    // Core systems (infrastructure layer)
    std::unique_ptr<RenderSystem> m_renderSystem;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<ThreadManager> m_threadManager;
    
    // Game systems (domain layer)
    std::unique_ptr<Map> m_gameMap;
    std::unique_ptr<ElementsOnMap> m_elementsManager;
    std::unique_ptr<EntitiesManager> m_entitiesManager;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<GameLogic> m_gameLogic;
    
    bool m_running;
    
    // Private methods
    bool initializeCoreSystem();
    bool initializeGameSystems();
    void cleanupSystems();
    void mainLoop();
};

#endif // GAME_ENGINE_H
