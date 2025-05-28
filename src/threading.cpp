#include "threading.h"
#include "map.h"
#include "elementsOnMap.h"
#include "entities.h"
#include "camera.h"
#include "player.h"
#include "inputs.h"
#include "debug.h"
#include <iostream>

// Global thread manager instance
GameThreadManager* g_threadManager = nullptr;

GameThreadManager::GameThreadManager()
    : m_running(false)
    , m_threadsStarted(false)
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
    if (!gameMap || !elementsManager || !entitiesManager || !camera) {
        std::cerr << "GameThreadManager::initialize - Invalid parameters" << std::endl;
        return false;
    }
    
    m_gameMap = gameMap;
    m_elementsManager = elementsManager;
    m_entitiesManager = entitiesManager;
    m_camera = camera;
    
    // Initialize timing
    m_lastGameUpdate = std::chrono::high_resolution_clock::now();
    m_lastRenderUpdate = m_lastGameUpdate;
    
    std::cout << "GameThreadManager initialized successfully" << std::endl;
    return true;
}

void GameThreadManager::startThreads()
{
    if (m_threadsStarted.load()) {
        std::cout << "Threads already started" << std::endl;
        return;
    }
    
    m_running.store(true);
    
    // Start game logic thread
    m_gameThread = std::thread(&GameThreadManager::gameLogicThread, this);
    
    // Start render thread
    m_renderThread = std::thread(&GameThreadManager::renderThread, this);
    
    m_threadsStarted.store(true);
    std::cout << "Game threads started successfully" << std::endl;
}

void GameThreadManager::stopThreads()
{
    if (!m_threadsStarted.load()) {
        return;
    }
    
    std::cout << "Stopping game threads..." << std::endl;
    
    // Signal threads to stop
    m_running.store(false);
    
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
    std::lock_guard<std::mutex> lock(m_inputStateMutex);
    
    m_inputState.moveX = moveX;
    m_inputState.moveY = moveY;
    
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

void GameThreadManager::gameLogicThread()
{
    std::cout << "Game logic thread started" << std::endl;
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    double accumulatedTime = 0.0;
    
    while (m_running.load()) {
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
    // CRASH FIX: Add null pointer validation
    if (!m_gameMap || !m_elementsManager || !m_entitiesManager || !m_camera) {
        std::cerr << "CRITICAL: GameThreadManager has null pointers!" << std::endl;
        return;
    }
    
    // Get current input state
    InputState currentInput;
    {
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
    
    // Process input if updated
    if (currentInput.stateUpdated) {
        // Track if player is moving
        static bool wasMoving = false;
        bool isMoving = (currentInput.moveX != 0.0f || currentInput.moveY != 0.0f);
        
        // Move the player if any movement key is pressed
        if (isMoving) {
            // If player just started moving, print debug message
            if (!wasMoving) {
                std::cout << "Player started moving" << std::endl;
            }
            movePlayer(currentInput.moveX, currentInput.moveY);
        }
        // If player just stopped moving, disable animation and set to standing frame
        else if (wasMoving) {
            std::cout << "Player stopped moving - disabling animation" << std::endl;
            m_elementsManager->changeElementAnimationStatus("player1", false);
            m_elementsManager->changeElementSpriteFrame("player1", 0); // Set to standing frame
        }
        
        // Update movement state
        wasMoving = isMoving;
        
        // Process debug keys
        processDebugKeys(*m_elementsManager);
        
        // Process camera controls
        processCameraControls();
    }
    
    // Update entities (handle movement and animations)
    m_entitiesManager->update(deltaTime);
    
    // Periodically move antagonists
    if (gameTime - m_lastAntagonistMoveTime >= ANTAGONIST_MOVE_INTERVAL) {
        std::cout << "Moving antagonists at game time: " << gameTime << std::endl;
        m_entitiesManager->walkEntityWithPathfinding("antagonist1", 10.0f, 46.0f, WalkType::NORMAL);
        m_entitiesManager->walkEntityWithPathfinding("antagonist2", 20.0f, 45.0f, WalkType::NORMAL);
        m_entitiesManager->walkEntityWithPathfinding("antagonist3", 30.0f, 44.0f, WalkType::NORMAL);
        m_lastAntagonistMoveTime = gameTime;
    }
    
    // Update game state for rendering
    {
        std::lock_guard<std::mutex> lock(m_gameStateMutex);
        
        // Get player position
        getPlayerPosition(m_currentGameState.playerX, m_currentGameState.playerY);
        m_currentGameState.currentTime = gameTime;
        m_currentGameState.deltaTime = deltaTime;
        m_currentGameState.playerMoving = (currentInput.moveX != 0.0f || currentInput.moveY != 0.0f);
        
        // Update camera position based on player position
        extern int windowWidth, windowHeight; // From globals.h
        m_camera->updateCameraPosition(m_currentGameState.playerX, m_currentGameState.playerY, windowWidth, windowHeight);
    }
    
    // Notify render thread that game state has been updated
    m_gameStateChanged.notify_one();
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
    if (g_threadManager != nullptr) {
        delete g_threadManager;
        g_threadManager = nullptr;
    }
}
