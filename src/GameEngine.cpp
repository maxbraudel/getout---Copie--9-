#include "GameEngine.h"
#include "RenderSystem.h"
#include "InputManager.h"
#include "ThreadManager.h"
#include "GameLogic.h"
#include "map.h"
#include "elementsOnMap.h"
#include "entities.h"
#include "camera.h"
#include "crashDebug.h"
#include "globals.h"
#include <iostream>
#include <chrono>
#include <thread>

GameEngine::GameEngine() : m_running(false) {
}

GameEngine::~GameEngine() {
    shutdown();
}

bool GameEngine::initialize(int width, int height, const std::string& title) {
    std::cout << "=== INITIALIZING GAME ENGINE ===" << std::endl;
    
    try {
        // Initialize core systems first
        if (!initializeCoreSystem()) {
            std::cerr << "Failed to initialize core systems" << std::endl;
            return false;
        }
        
        // Initialize game systems
        if (!initializeGameSystems()) {
            std::cerr << "Failed to initialize game systems" << std::endl;
            return false;
        }
        
        // Initialize render system
        m_renderSystem = std::make_unique<RenderSystem>();
        if (!m_renderSystem->initialize(width, height, title)) {
            std::cerr << "Failed to initialize render system" << std::endl;
            return false;
        }
        
        // Initialize input system
        m_inputManager = std::make_unique<InputManager>();
        if (!m_inputManager->initialize(m_renderSystem->getWindow())) {
            std::cerr << "Failed to initialize input manager" << std::endl;
            return false;
        }
        
        // Set up window close callback
        m_inputManager->setWindowCloseCallback([this]() { 
            std::cout << "Window close requested" << std::endl;
            requestShutdown(); 
        });
        
        // Initialize thread manager
        m_threadManager = std::make_unique<ThreadManager>();
        if (!m_threadManager->initialize()) {
            std::cerr << "Failed to initialize thread manager" << std::endl;
            return false;
        }
        
        // Set up the game logic update function
        m_threadManager->setGameLogicFunction([this](double deltaTime) {
            // Process input
            auto input = m_inputManager->getCurrentInput();
            m_gameLogic->processInput(input);
            
            // Update game logic
            m_gameLogic->update(deltaTime);
        });
        
        m_running = true;
        std::cout << "=== GAME ENGINE INITIALIZED SUCCESSFULLY ===" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during initialization: " << e.what() << std::endl;
        logCrashEvent("Engine Init Exception", e.what());
        return false;
    } catch (...) {
        std::cerr << "Unknown exception during initialization" << std::endl;
        logCrashEvent("Engine Init Unknown Exception", "Unknown error during initialization");
        return false;
    }
}

void GameEngine::run() {
    std::cout << "=== STARTING GAME ENGINE ===" << std::endl;
    
    try {
        // Start threading system
        m_threadManager->start();
        
        // Run main loop
        mainLoop();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in game loop: " << e.what() << std::endl;
        logCrashEvent("Game Loop Exception", e.what());
    } catch (...) {
        std::cerr << "Unknown exception in game loop" << std::endl;
        logCrashEvent("Game Loop Unknown Exception", "Unknown error in game loop");
    }
    
    std::cout << "=== GAME ENGINE STOPPED ===" << std::endl;
}

void GameEngine::shutdown() {
    if (!m_running) return;
    
    std::cout << "=== SHUTTING DOWN GAME ENGINE ===" << std::endl;
    
    m_running = false;
    
    try {
        // Stop threading system first
        if (m_threadManager) {
            m_threadManager->stop();
        }
        
        // Clean up systems
        cleanupSystems();
        
        std::cout << "=== GAME ENGINE SHUTDOWN COMPLETE ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during shutdown: " << e.what() << std::endl;
        logCrashEvent("Engine Shutdown Exception", e.what());
    } catch (...) {
        std::cerr << "Unknown exception during shutdown" << std::endl;
        logCrashEvent("Engine Shutdown Unknown Exception", "Unknown error during shutdown");
    }
}

bool GameEngine::initializeCoreSystem() {
    // Initialize crash debugging
    DEBUG_LOG_MEMORY("engine_core_init_start");
    
    // Core systems don't need special initialization here
    // They are initialized when created
    
    DEBUG_LOG_MEMORY("engine_core_init_complete");
    return true;
}

bool GameEngine::initializeGameSystems() {
    std::cout << "Initializing game systems..." << std::endl;
    
    // Create game systems
    m_gameMap = std::make_unique<Map>();
    m_elementsManager = std::make_unique<ElementsOnMap>();
    m_entitiesManager = std::make_unique<EntitiesManager>();
    m_camera = std::make_unique<Camera>();
    
    // Initialize them with your existing initialization code
    // TODO: Move your existing map loading, entity setup, etc. here
    // For now, using basic initialization
    
    // Initialize game logic system
    m_gameLogic = std::make_unique<GameLogic>();
    if (!m_gameLogic->initialize(m_gameMap.get(), m_elementsManager.get(), 
                                m_entitiesManager.get(), m_camera.get())) {
        return false;
    }
    
    std::cout << "Game systems initialized successfully" << std::endl;
    return true;
}

void GameEngine::cleanupSystems() {
    std::cout << "Cleaning up game systems..." << std::endl;
    
    try {
        // Clean up in reverse order of initialization
        m_gameLogic.reset();
        m_threadManager.reset();
        m_inputManager.reset();
        m_renderSystem.reset();
        
        // Game systems
        m_camera.reset();
        m_entitiesManager.reset();
        m_elementsManager.reset();
        m_gameMap.reset();
        
        DEBUG_LOG_MEMORY("engine_cleanup_complete");
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during system cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception during system cleanup" << std::endl;
    }
}

void GameEngine::mainLoop() {
    std::cout << "Starting main game loop..." << std::endl;
    
    int frameCount = 0;
    
    while (m_running && !m_renderSystem->shouldClose()) {
        try {
            frameCount++;
            
            // Poll input events
            m_inputManager->pollEvents();
            
            // Get current game state from game logic
            auto gameState = m_gameLogic->getGameState();
            
            // Convert to render state
            RenderState renderState;
            renderState.playerX = gameState.playerX;
            renderState.playerY = gameState.playerY;
            renderState.currentTime = gameState.currentTime;
            renderState.playerMoving = gameState.playerMoving;
            
            // Render the game
            m_renderSystem->render(renderState, m_gameLogic->getGameMap(), 
                                 m_gameLogic->getElementsManager(), m_gameLogic->getCamera());
            
            // Periodic memory monitoring
            if (frameCount % 3600 == 0) { // Every ~60 seconds at 60fps
                DEBUG_LOG_MEMORY("main_loop_frame_" + std::to_string(frameCount));
            }
            
            // Brief sleep for frame rate control (~60fps)
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            
        } catch (const std::exception& e) {
            std::cerr << "Exception in main loop frame " << frameCount << ": " << e.what() << std::endl;
            logCrashEvent("Main Loop Frame Exception", 
                         "Frame " + std::to_string(frameCount) + ": " + e.what());
            // Continue to prevent complete crash
        } catch (...) {
            std::cerr << "Unknown exception in main loop frame " << frameCount << std::endl;
            logCrashEvent("Main Loop Frame Unknown Exception", 
                         "Frame " + std::to_string(frameCount) + ": Unknown error");
            // Continue to prevent complete crash
        }
    }
    
    std::cout << "Main game loop ended after " << frameCount << " frames" << std::endl;
}
