#ifndef PLAYER_MOVEMENT_MANAGER_H
#define PLAYER_MOVEMENT_MANAGER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <queue>
#include "enumDefinitions.h"

// Forward declarations
class Map;
class ElementsOnMap;
class EntitiesManager;
class Camera;

/**
 * PlayerMovementManager handles player movement in a separate thread
 * to ensure responsive controls independent of entity processing lag
 */
class PlayerMovementManager {
public:
    // Player input state
    struct PlayerInput {
        float moveX = 0.0f;
        float moveY = 0.0f;
        bool sprint = false;
        std::chrono::high_resolution_clock::time_point timestamp;
        bool valid = false;
    };

    // Player movement state
    struct PlayerState {
        float x = 0.0f;
        float y = 0.0f;
        bool isMoving = false;
        bool needsSync = false;
        std::chrono::high_resolution_clock::time_point lastUpdate;
    };

    PlayerMovementManager();
    ~PlayerMovementManager();    // Initialize the player movement system
    bool initialize(Map* gameMap, ElementsOnMap* elementsManager, EntitiesManager* entitiesManager, Camera* camera);

    // Start the player movement thread
    void startThread();

    // Stop the player movement thread
    void stopThread();

    // Pause/Resume functionality
    void pauseMovement();
    void resumeMovement();
    bool isMovementPaused() const { return m_paused.load(); }

    // Set player input (called from input thread)
    void setPlayerInput(float moveX, float moveY, bool sprint);

    // Get current player state (thread-safe)
    PlayerState getPlayerState() const;

    // Sync player position with main game state (called from game logic thread)
    void syncWithGameState();

    // Check if the system is running
    bool isRunning() const { return m_running.load(); }

private:
    // Player movement thread function
    void playerMovementThread();    // Process player movement with collision detection
    void processPlayerMovement(const PlayerInput& input, double deltaTime);

    // Update camera based on player position
    void updateCamera(double deltaTime);

    // Update player position and animations
    void updatePlayerPosition(float deltaX, float deltaY);

    // Handle collision detection for player
    bool checkPlayerCollision(float newX, float newY, float& actualDeltaX, float& actualDeltaY) const;    // Check for nearby coconuts and collect them
    void checkAndCollectCoconuts();
    
    // Process win condition timing
    void processWinCondition(double deltaTime);

    // Threading objects
    std::thread m_movementThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_threadStarted;
    std::atomic<bool> m_paused;

    // Synchronization
    mutable std::mutex m_inputMutex;
    mutable std::mutex m_stateMutex;
    std::condition_variable m_inputAvailable;
    std::condition_variable m_pauseCondition;

    // Game objects (not owned)
    Map* m_gameMap;
    ElementsOnMap* m_elementsManager;
    EntitiesManager* m_entitiesManager;
    Camera* m_camera;

    // Player state
    PlayerInput m_currentInput;
    PlayerState m_playerState;
    std::queue<PlayerInput> m_inputQueue;

    // Timing constants
    static constexpr double PLAYER_UPDATE_FPS = 120.0; // Higher frequency for responsive movement
    static constexpr double PLAYER_UPDATE_TIMESTEP = 1.0 / PLAYER_UPDATE_FPS;
    static constexpr int MAX_INPUT_QUEUE_SIZE = 10; // Prevent input lag accumulation    // Performance tracking
    std::atomic<uint64_t> m_movementUpdatesProcessed{0};
    std::atomic<uint64_t> m_collisionChecksPerformed{0};
    std::atomic<double> m_averageUpdateTime{0.0};
    
    // Win condition tracking
    bool m_winConditionTriggered{false};
    double m_winDelayTimer{0.0};
    mutable std::mutex m_winStateMutex;
};

// Global instance
extern PlayerMovementManager* g_playerMovementManager;

// Convenience functions
bool initializePlayerMovement(Map* gameMap, ElementsOnMap* elementsManager, EntitiesManager* entitiesManager, Camera* camera);
void startPlayerMovementThread();
void stopPlayerMovementThread();
void cleanupPlayerMovement();

#endif // PLAYER_MOVEMENT_MANAGER_H
