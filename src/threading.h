#ifndef THREADING_H
#define THREADING_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>

// Forward declarations
class Map;
class ElementsOnMap;
class EntitiesManager;
class Camera;

/**
 * GameThreadManager handles the separation of game logic and rendering into separate threads
 * - Game logic runs at fixed 60Hz timestep 
 * - Rendering runs at variable rate (up to display refresh rate)
 * - Thread synchronization prevents race conditions
 */
class GameThreadManager {
public:
    // Constructor - initializes threading system
    GameThreadManager();
    
    // Destructor - ensures clean shutdown
    ~GameThreadManager();
    
    // Initialize the threading system with game objects
    bool initialize(Map* gameMap, ElementsOnMap* elementsManager, EntitiesManager* entitiesManager, Camera* camera);
    
    // Start the game and render threads
    void startThreads();
    
    // Stop all threads and cleanup
    void stopThreads();
    
    // Check if threads should continue running
    bool isRunning() const { return m_running.load(); }
    
    // Set the running state (used to signal shutdown)
    void setRunning(bool running) { m_running.store(running); }
    
    // Get current game state for rendering (thread-safe)
    struct GameState {
        float playerX, playerY;
        double currentTime;
        double deltaTime;
        bool playerMoving;
        // Add more state variables as needed
    };
    
    // Thread-safe getter for current game state
    GameState getGameState();
    
    // Thread-safe setter for input state
    void setInputState(float moveX, float moveY, bool debugKeys[], bool cameraControls[]);
    
private:
    // Thread functions
    void gameLogicThread();
    void renderThread();
    
    // Game logic update function (runs at 60Hz)
    void updateGameLogic(double deltaTime);
    
    // Threading objects
    std::thread m_gameThread;
    std::thread m_renderThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_threadsStarted;
    
    // Synchronization
    mutable std::mutex m_gameStateMutex;
    mutable std::mutex m_inputStateMutex;
    std::condition_variable m_gameStateChanged;
    
    // Game objects (not owned by this class)
    Map* m_gameMap;
    ElementsOnMap* m_elementsManager;
    EntitiesManager* m_entitiesManager;
    Camera* m_camera;
    
    // Game state data (protected by mutex)
    GameState m_currentGameState;
    
    // Input state data (protected by mutex)
    struct InputState {
        float moveX, moveY;
        bool debugKeys[10];  // Assuming max 10 debug keys
        bool cameraControls[5]; // Assuming max 5 camera control keys
        bool stateUpdated;
    } m_inputState;
    
    // Timing constants
    static constexpr double GAME_LOGIC_FPS = 60.0;
    static constexpr double GAME_LOGIC_TIMESTEP = 1.0 / GAME_LOGIC_FPS;
    
    // Timing variables
    std::chrono::high_resolution_clock::time_point m_lastGameUpdate;
    std::chrono::high_resolution_clock::time_point m_lastRenderUpdate;
    
    // Entity movement timing
    double m_lastAntagonistMoveTime;
    static constexpr double ANTAGONIST_MOVE_INTERVAL = 5.0;
};

// Global instance (declared in threading.cpp)
extern GameThreadManager* g_threadManager;

// Convenience functions for main.cpp
bool initializeThreading(Map* gameMap, ElementsOnMap* elementsManager, EntitiesManager* entitiesManager, Camera* camera);
void startGameThreads();
void stopGameThreads();
void cleanupThreading();

#endif // THREADING_H
