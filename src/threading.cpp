#include "threading.h"
#include "PlayerMovementManager.h"
#include "map.h"
#include "elementsOnMap.h"
#include "entities.h"
#include "entityBehaviors.h"
#include "camera.h"
#include "player.h"
#include "inputs.h"
#include "debug.h"
#include "crashDebug.h"
#include "performanceProfiler.h"
#include "globals.h"
#include <iostream>
#include "enumDefinitions.h"


// Global thread manager instance
GameThreadManager* g_threadManager = nullptr;

GameThreadManager::GameThreadManager()
    : m_running(false)
    , m_threadsStarted(false)
    , m_paused(false)
    , m_gameMap(nullptr)
    , m_elementsManager(nullptr)
    , m_entitiesManager(nullptr)
    , m_camera(nullptr)
    , m_lastAntagonistMoveTime(0.0)
{
    // Initialize game state
    m_currentGameState = {};
    
    // Initialize input state
    m_inputState = {};
    for (int i = 0; i < 10; i++) {
        m_inputState.debugKeys[i] = false;
    }
    for (int i = 0; i < 5; i++) {
        m_inputState.cameraControls[i] = false;
    }
    m_inputState.stateUpdated = false;
}

GameThreadManager::~GameThreadManager()
{
    stopThreads();
}

bool GameThreadManager::initialize(Map* gameMap, ElementsOnMap* elementsManager, EntitiesManager* entitiesManager, Camera* camera)
{
    DEBUG_VALIDATE_PTR(gameMap);
    DEBUG_VALIDATE_PTR(elementsManager);
    DEBUG_VALIDATE_PTR(entitiesManager);
    DEBUG_VALIDATE_PTR(camera);
    
    if (!gameMap || !elementsManager || !entitiesManager || !camera) {
        std::cerr << "CRASH FIX: GameThreadManager::initialize - Invalid parameters" << std::endl;
        DEBUG_LOG_MEMORY("thread_manager_init_failed");
        return false;
    }
    
    m_gameMap = gameMap;
    m_elementsManager = elementsManager;
    m_entitiesManager = entitiesManager;
    m_camera = camera;
    
    // Initialize timing
    m_lastGameUpdate = std::chrono::high_resolution_clock::now();
    m_lastRenderUpdate = m_lastGameUpdate;
      // Initialize player movement manager with camera
    if (!initializePlayerMovement(gameMap, elementsManager, entitiesManager, camera)) {
        std::cerr << "Failed to initialize player movement manager" << std::endl;
        DEBUG_LOG_MEMORY("player_movement_init_failed");
        return false;
    }
    
    DEBUG_LOG_MEMORY("thread_manager_initialized");
    std::cout << "GameThreadManager initialized successfully with async player movement" << std::endl;
    return true;
}

void GameThreadManager::startThreads()
{
    if (m_threadsStarted.load()) {
        std::cout << "Threads already started" << std::endl;
        return;
    }
    
    m_running.store(true);
    
    // Start player movement thread first
    startPlayerMovementThread();
    
    // Start game logic thread
    m_gameThread = std::thread(&GameThreadManager::gameLogicThread, this);
    
    // Start render thread
    m_renderThread = std::thread(&GameThreadManager::renderThread, this);
    
    m_threadsStarted.store(true);
    std::cout << "Game threads started successfully with async player movement" << std::endl;
}

void GameThreadManager::stopThreads()
{
    if (!m_threadsStarted.load()) {
        return;
    }
    
    std::cout << "Stopping game threads..." << std::endl;
    
    // Signal threads to stop
    m_running.store(false);
    
    // Stop player movement thread first
    stopPlayerMovementThread();
    
    // Wake up any waiting threads
    m_gameStateChanged.notify_all();
    
    // Wait for threads to finish
    if (m_gameThread.joinable()) {
        m_gameThread.join();
    }
    
    if (m_renderThread.joinable()) {
        m_renderThread.join();
    }
    
    m_threadsStarted.store(false);
    std::cout << "Game threads stopped" << std::endl;
}

GameThreadManager::GameState GameThreadManager::getGameState()
{
    std::lock_guard<std::mutex> lock(m_gameStateMutex);
    return m_currentGameState;
}

void GameThreadManager::setInputState(float moveX, float moveY, bool debugKeys[], bool cameraControls[])
{
    // Route player movement to the dedicated player movement manager
    // Note: Sprint state is handled in the main thread and passed directly to setPlayerMovementInput
    
    // Handle debug keys and camera controls in the main game logic thread
    std::lock_guard<std::mutex> lock(m_inputStateMutex);
    
    // We no longer store player movement in the main input state
    m_inputState.moveX = 0.0f;
    m_inputState.moveY = 0.0f;
    
    // Copy debug keys
    for (int i = 0; i < 10; i++) {
        m_inputState.debugKeys[i] = debugKeys[i];
    }
    
    // Copy camera control keys
    for (int i = 0; i < 5; i++) {
        m_inputState.cameraControls[i] = cameraControls[i];
    }
    
    m_inputState.stateUpdated = true;
}

void GameThreadManager::setPlayerMovementInput(float moveX, float moveY, bool sprint)
{
    if (g_playerMovementManager != nullptr) {
        g_playerMovementManager->setPlayerInput(moveX, moveY, sprint);
    }
}

void GameThreadManager::gameLogicThread()
{
    std::cout << "Game logic thread started" << std::endl;
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    double accumulatedTime = 0.0;
    
    while (m_running.load()) {
        // Check if paused - if so, wait for resume
        if (m_paused.load()) {
            std::unique_lock<std::mutex> lock(m_gameStateMutex);
            m_pauseCondition.wait(lock, [this] { return !m_paused.load() || !m_running.load(); });
            
            // Reset timing when resuming to avoid catching up on missed frames
            lastTime = std::chrono::high_resolution_clock::now();
            accumulatedTime = 0.0;
            
            if (!m_running.load()) break;
        }
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Accumulate time for fixed timestep
        accumulatedTime += elapsed;
        
        // Run game logic at fixed timestep
        while (accumulatedTime >= GAME_LOGIC_TIMESTEP) {
            updateGameLogic(GAME_LOGIC_TIMESTEP);
            accumulatedTime -= GAME_LOGIC_TIMESTEP;
        }
        
        // Sleep for a short time to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
    
    std::cout << "Game logic thread ended" << std::endl;
}

void GameThreadManager::renderThread()
{
    std::cout << "Render thread started" << std::endl;
    
    while (m_running.load()) {
        // Wait for game state update or timeout
        std::unique_lock<std::mutex> lock(m_gameStateMutex);
        m_gameStateChanged.wait_for(lock, std::chrono::milliseconds(16)); // ~60fps max
        
        // The actual rendering will be done in the main thread
        // This thread just manages the timing and synchronization
        
        // Sleep briefly to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
    
    std::cout << "Render thread ended" << std::endl;
}

void GameThreadManager::updateGameLogic(double deltaTime)
{
    PROFILE_SCOPE("GameLogic_Total");
    
    // CRASH FIX: Add null pointer validation and memory monitoring
    DEBUG_VALIDATE_PTR(m_gameMap);
    DEBUG_VALIDATE_PTR(m_elementsManager);
    DEBUG_VALIDATE_PTR(m_entitiesManager);
    DEBUG_VALIDATE_PTR(m_camera);
    
    if (!m_gameMap || !m_elementsManager || !m_entitiesManager || !m_camera) {
        std::cerr << "CRITICAL: GameThreadManager has null pointers!" << std::endl;
        DEBUG_LOG_MEMORY("game_logic_null_ptrs");
        return;
    }
    
    // CRASH FIX: Static frame counter for periodic memory monitoring
    static int logicFrameCount = 0;
    logicFrameCount++;
    
    if (logicFrameCount % 1800 == 0) { // Every ~30 seconds at 60 FPS
        DEBUG_LOG_MEMORY("game_logic_frame_" + std::to_string(logicFrameCount));
    }
    
    // Get current input state
    InputState currentInput;
    {
        PROFILE_SCOPE("InputState_Copy");
        std::lock_guard<std::mutex> lock(m_inputStateMutex);
        currentInput = m_inputState;
        m_inputState.stateUpdated = false;
    }

    // CRASH FIX: Validate input state before using
    if (!m_running.load()) {
        return; // Exit early if shutting down
    }
    
    // Update game time
    static double gameTime = 0.0;
    gameTime += deltaTime;
      
    // Process input if updated (debug keys and camera controls only - player movement handled separately)
    if (currentInput.stateUpdated) {
        PROFILE_SCOPE("Input_Processing");
        // Process debug keys
        processDebugKeys(*m_elementsManager);
        
        // Process camera controls
        processCameraControls();
    }
    
    // Sync player position from the dedicated player movement thread
    {
        PROFILE_SCOPE("PlayerMovement_Sync");
        if (g_playerMovementManager != nullptr) {
            g_playerMovementManager->syncWithGameState();
        }
    }
      
    // Update entities (handle movement and animations) - this is now the main focus of the game logic thread
    // CRASH FIX: Add try-catch around entities update to prevent crashes
    try {
        // Get camera bounds for view frustum culling
        float cameraLeft, cameraRight, cameraBottom, cameraTop;
        {
            PROFILE_SCOPE("Camera_Bounds_Get");
            cameraLeft = gameCamera.getLeft();
            cameraRight = gameCamera.getRight();
            cameraBottom = gameCamera.getBottom();
            cameraTop = gameCamera.getTop();
        }
        
        // Update entities with view frustum culling
        {
            PROFILE_SCOPE("Entities_Update");
            m_entitiesManager->update(deltaTime, cameraLeft, cameraRight, cameraBottom, cameraTop);
        }
        
        // Update entity behaviors with view frustum culling (automatic behaviors like passive random walking)
        {
            PROFILE_SCOPE("EntityBehaviors_Update");
            entityBehaviorManager.update(deltaTime, *m_entitiesManager, cameraLeft, cameraRight, cameraBottom, cameraTop);
        }
        
        // Update player slip functionality
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception in entities update: " << e.what() << std::endl;
        // Continue execution to prevent complete crash
    } catch (...) {
        std::cerr << "CRITICAL: Unknown exception in entities update!" << std::endl;
        // Continue execution to prevent complete crash
    }
    
    /* // Periodically move antagonists
    if (gameTime - m_lastAntagonistMoveTime >= ANTAGONIST_MOVE_INTERVAL) {
        std::cout << "Moving antagonists at game time: " << gameTime << std::endl;
        m_entitiesManager->walkEntityWithPathFindingToRandomRadiusTarget("antagonist1", 10.0f, WalkType::NORMAL);
        m_entitiesManager->walkEntityWithPathFindingToRandomRadiusTarget("antagonist2", 20.0f, WalkType::NORMAL);
        m_entitiesManager->walkEntityWithPathFindingToRandomRadiusTarget("antagonist3", 30.0f, WalkType::NORMAL);
        m_lastAntagonistMoveTime = gameTime;
    } */
      
    // Update game state for rendering
    {
        PROFILE_SCOPE("GameState_Update");
        std::lock_guard<std::mutex> lock(m_gameStateMutex);
        
        // Get player position and movement state from the dedicated player movement manager
        if (g_playerMovementManager != nullptr) {
            auto playerState = g_playerMovementManager->getPlayerState();
            m_currentGameState.playerX = playerState.x;
            m_currentGameState.playerY = playerState.y;
            m_currentGameState.playerMoving = playerState.isMoving;        } else {
            // Fallback to direct position query if player movement manager is not available
            if (!getPlayerPosition(m_currentGameState.playerX, m_currentGameState.playerY)) {
                // Player entity doesn't exist - keep previous position and disable movement
                m_currentGameState.playerMoving = false;
            } else {
                m_currentGameState.playerMoving = false;
            }
        }
          
        m_currentGameState.currentTime = gameTime;
        m_currentGameState.deltaTime = deltaTime;
        
        // Camera updates are now handled in the player movement thread at 120Hz
        // for smoother camera following synchronized with player movement
    }
    
    // Print performance report every 5 seconds (300 frames at 60Hz)
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter >= 300) {
        PerformanceProfiler::getInstance().printReport();
        frameCounter = 0;
    }
    
    // Notify render thread that game state has been updated
    m_gameStateChanged.notify_one();
}

void GameThreadManager::pauseGame()
{
    std::cout << "Pausing game..." << std::endl;
    m_paused.store(true);
    
    // Set game state to PAUSE
    GAME_STATE = ::GameState::PAUSE;
    std::cout << "Game state set to: " << gameStateToString(GAME_STATE) << std::endl;
    
    // Also pause player movement
    if (g_playerMovementManager != nullptr) {
        g_playerMovementManager->pauseMovement();
    }
}

void GameThreadManager::resumeGame()
{
    // Check if the game is in WIN or DEFEAT state - if so, don't allow resuming
    if (GAME_STATE == ::GameState::WIN) {
        std::cout << "Cannot resume game - player has won! Game remains paused." << std::endl;
        return;
    }
    if (GAME_STATE == ::GameState::DEFEAT) {
        std::cout << "Cannot resume game - player has been defeated! Game remains paused." << std::endl;
        return;
    }
    
    std::cout << "Resuming game..." << std::endl;
    m_paused.store(false);
    
    // Set game state back to GAMEPLAY
    GAME_STATE = ::GameState::GAMEPLAY;
    std::cout << "Game state set to: " << gameStateToString(GAME_STATE) << std::endl;
    
    // Also resume player movement
    if (g_playerMovementManager != nullptr) {
        g_playerMovementManager->resumeMovement();
    }
    
    // Wake up any waiting threads
    m_pauseCondition.notify_all();
}

// Convenience functions implementation
bool initializeThreading(Map* gameMap, ElementsOnMap* elementsManager, EntitiesManager* entitiesManager, Camera* camera)
{
    if (g_threadManager != nullptr) {
        std::cout << "Thread manager already initialized" << std::endl;
        return true;
    }
    
    g_threadManager = new GameThreadManager();
    return g_threadManager->initialize(gameMap, elementsManager, entitiesManager, camera);
}

void startGameThreads()
{
    if (g_threadManager != nullptr) {
        g_threadManager->startThreads();
    }
}

void stopGameThreads()
{
    if (g_threadManager != nullptr) {
        g_threadManager->stopThreads();
    }
}

void cleanupThreading()
{
    // Clean up player movement manager first
    cleanupPlayerMovement();
    
    if (g_threadManager != nullptr) {
        delete g_threadManager;
        g_threadManager = nullptr;
    }
}
