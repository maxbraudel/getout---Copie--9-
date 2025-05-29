#include "GameLogic.h"
#include "map.h"
#include "elementsOnMap.h"
#include "entities.h"
#include "entityBehaviors.h"
#include "camera.h"
#include "player.h"
#include "inputs.h"
#include "debug.h"
#include "crashDebug.h"
#include <iostream>

GameLogic::GameLogic() 
    : m_gameMap(nullptr), m_elementsManager(nullptr), m_entitiesManager(nullptr), m_camera(nullptr),
      m_gameTime(0.0), m_lastAntagonistMoveTime(0.0), m_wasPlayerMoving(false) {
}

GameLogic::~GameLogic() {
    // GameLogic doesn't own the game systems, so no cleanup needed
}

bool GameLogic::initialize(Map* gameMap, ElementsOnMap* elementsManager, 
                          EntitiesManager* entitiesManager, Camera* camera) {
    DEBUG_VALIDATE_PTR(gameMap);
    DEBUG_VALIDATE_PTR(elementsManager);
    DEBUG_VALIDATE_PTR(entitiesManager);
    DEBUG_VALIDATE_PTR(camera);
    
    if (!gameMap || !elementsManager || !entitiesManager || !camera) {
        std::cerr << "GameLogic::initialize - Invalid parameters" << std::endl;
        DEBUG_LOG_MEMORY("game_logic_init_failed");
        return false;
    }
    
    m_gameMap = gameMap;
    m_elementsManager = elementsManager;
    m_entitiesManager = entitiesManager;
    m_camera = camera;
    
    DEBUG_LOG_MEMORY("game_logic_initialized");
    std::cout << "GameLogic initialized successfully" << std::endl;
    return true;
}

void GameLogic::update(double deltaTime) {
    // Validate pointers
    DEBUG_VALIDATE_PTR(m_gameMap);
    DEBUG_VALIDATE_PTR(m_elementsManager);
    DEBUG_VALIDATE_PTR(m_entitiesManager);
    DEBUG_VALIDATE_PTR(m_camera);
    
    if (!m_gameMap || !m_elementsManager || !m_entitiesManager || !m_camera) {
        std::cerr << "CRITICAL: GameLogic has null pointers!" << std::endl;
        DEBUG_LOG_MEMORY("game_logic_null_ptrs");
        return;
    }
    
    // Update game time
    m_gameTime += deltaTime;
    
    // Update entities
    updateEntities(deltaTime);
    
    // Periodically move antagonists
    updateAntagonists();
      // Update camera
    updateCamera(deltaTime);
    
    // Update game state
    updateGameState(deltaTime, m_wasPlayerMoving);
}

void GameLogic::processInput(const InputState& input) {
    if (!input.stateUpdated) {
        return;
    }
    
    // Process player movement
    updatePlayer(input);
    
    // Process debug keys
    if (m_elementsManager) {
        processDebugKeys(*m_elementsManager);
    }
    
    // Process camera controls
    processCameraControls();
}

GameState GameLogic::getGameState() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_gameState;
}

void GameLogic::updatePlayer(const InputState& input) {
    // Track if player is moving
    bool isMoving = (input.moveX != 0.0f || input.moveY != 0.0f);
    
    // Move the player if any movement key is pressed
    if (isMoving) {
        // If player just started moving, print debug message
        if (!m_wasPlayerMoving) {
            std::cout << "Player started moving" << std::endl;
        }
        movePlayer(input.moveX, input.moveY);
    }
    // If player just stopped moving, disable animation and set to standing frame
    else if (m_wasPlayerMoving) {
        std::cout << "Player stopped moving - disabling animation" << std::endl;
        if (m_elementsManager) {
            m_elementsManager->changeElementAnimationStatus("player1", false);
            m_elementsManager->changeElementSpriteFrame("player1", 0);
        }
    }
    
    // Update movement state
    m_wasPlayerMoving = isMoving;
}

void GameLogic::updateEntities(double deltaTime) {
    if (!m_entitiesManager) return;
    
    try {
        // Update entity movement and animations
        m_entitiesManager->update(deltaTime);
        
        // Update entity behaviors (automatic behaviors like passive random walking)
        entityBehaviorManager.update(deltaTime, *m_entitiesManager);
        
        // Update player slip functionality
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception in entities update: " << e.what() << std::endl;
        // Continue execution to prevent complete crash
    } catch (...) {
        std::cerr << "CRITICAL: Unknown exception in entities update!" << std::endl;
        // Continue execution to prevent complete crash
    }
}

void GameLogic::updateAntagonists() {
    if (!m_entitiesManager) return;
    
    if (m_gameTime - m_lastAntagonistMoveTime >= ANTAGONIST_MOVE_INTERVAL) {
        std::cout << "Moving antagonists at game time: " << m_gameTime << std::endl;
        m_entitiesManager->walkEntityWithPathfinding("antagonist1", 10.0f, 46.0f, WalkType::NORMAL);
        m_entitiesManager->walkEntityWithPathfinding("antagonist2", 20.0f, 45.0f, WalkType::NORMAL);
        m_entitiesManager->walkEntityWithPathfinding("antagonist3", 30.0f, 44.0f, WalkType::NORMAL);
        m_lastAntagonistMoveTime = m_gameTime;
    }
}

void GameLogic::updateCamera(double deltaTime) {
    if (!m_camera) return;
    
    // Update smooth camera transitions
    m_camera->updateSmoothTransitions(static_cast<float>(deltaTime));
    
    // Camera position is updated in the render system with current player position
    // This method handles smooth camera region transitions
}

void GameLogic::updateGameState(double deltaTime, bool playerMoving) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    // Get player position
    getPlayerPosition(m_gameState.playerX, m_gameState.playerY);
    
    // Update timing
    m_gameState.currentTime = m_gameTime;
    m_gameState.deltaTime = deltaTime;
    m_gameState.playerMoving = playerMoving;
}
