#include "PlayerMovementManager.h"
#include "map.h"
#include "elementsOnMap.h"
#include "entities.h"
#include "player.h"
#include "collision.h"
#include "inputs.h"
#include "debug.h"
#include "crashDebug.h"
#include "camera.h"
#include "globals.h"
#include "entitiesStatus.h"
#include "gameMenus.h"
#include "threading.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// Global player movement manager instance
PlayerMovementManager* g_playerMovementManager = nullptr;

PlayerMovementManager::PlayerMovementManager()
    : m_running(false)
    , m_threadStarted(false)
    , m_paused(false)
    , m_gameMap(nullptr)
    , m_elementsManager(nullptr)
    , m_entitiesManager(nullptr)
    , m_camera(nullptr)
    , m_lastKnownPlayerX(0.0f)
    , m_lastKnownPlayerY(0.0f)
{
    // Initialize player state
    m_playerState = {};
    m_currentInput = {};
    
    // Initialize timing
    m_playerState.lastUpdate = std::chrono::high_resolution_clock::now();
}

PlayerMovementManager::~PlayerMovementManager()
{
    stopThread();
}

bool PlayerMovementManager::initialize(Map* gameMap, ElementsOnMap* elementsManager, EntitiesManager* entitiesManager, Camera* camera)
{
    DEBUG_VALIDATE_PTR(gameMap);
    DEBUG_VALIDATE_PTR(elementsManager);
    DEBUG_VALIDATE_PTR(entitiesManager);
    DEBUG_VALIDATE_PTR(camera);
    
    if (!gameMap || !elementsManager || !entitiesManager || !camera) {
        std::cerr << "CRASH FIX: PlayerMovementManager::initialize - Invalid parameters" << std::endl;
        DEBUG_LOG_MEMORY("player_movement_init_failed");
        return false;
    }
    
    m_gameMap = gameMap;
    m_elementsManager = elementsManager;
    m_entitiesManager = entitiesManager;
    m_camera = camera;
      // Get initial player position
    if (!getPlayerPosition(m_playerState.x, m_playerState.y)) {
        std::cerr << "Warning: Could not get initial player position" << std::endl;
        // Use default player position coordinates from the entity configuration
        const EntityConfiguration* playerConfig = m_entitiesManager->getConfiguration(EntityName::PLAYER);
        if (playerConfig) {
            m_playerState.x = 5.0f; // These values should match the player entity placement
            m_playerState.y = 45.0f;
            std::cout << "Using player initial position from placement coordinates: (" 
                     << m_playerState.x << ", " << m_playerState.y << ")" << std::endl;
        } else {
            m_playerState.x = GRID_SIZE / 2.0f;
            m_playerState.y = GRID_SIZE / 2.0f;
            std::cout << "Using map center as initial player position: (" 
                     << m_playerState.x << ", " << m_playerState.y << ")" << std::endl;
        }
    }
    
    // Pre-position the camera based on player's initial position to prevent flicker
    // This needs to happen BEFORE threads start to ensure camera is in the correct position
    extern int windowWidth, windowHeight; // From globals.h
    m_camera->updateCameraPosition(m_playerState.x, m_playerState.y, windowWidth, windowHeight);
    
    DEBUG_LOG_MEMORY("player_movement_initialized");
    std::cout << "PlayerMovementManager initialized successfully at (" << m_playerState.x << ", " << m_playerState.y << ")" << std::endl;
    return true;
}

void PlayerMovementManager::startThread()
{
    if (m_threadStarted.load()) {
        std::cout << "Player movement thread already started" << std::endl;
        return;
    }
    
    m_running.store(true);
    
    // Start player movement thread
    m_movementThread = std::thread(&PlayerMovementManager::playerMovementThread, this);
    
    m_threadStarted.store(true);
    std::cout << "Player movement thread started successfully" << std::endl;
}

void PlayerMovementManager::stopThread()
{
    m_running.store(false);
    
    // Wake up the thread if it's waiting
    m_inputAvailable.notify_all();
    m_pauseCondition.notify_all();
    
    if (m_movementThread.joinable()) {
        m_movementThread.join();
    }
    
    m_threadStarted.store(false);
    std::cout << "Player movement thread stopped" << std::endl;
}

void PlayerMovementManager::setPlayerInput(float moveX, float moveY, bool sprint)
{
    std::lock_guard<std::mutex> lock(m_inputMutex);
    
    // Don't accept movement input when paused - this prevents input accumulation
    if (m_paused.load()) {
        // Clear current input when paused to ensure no movement
        PlayerInput clearInput;
        clearInput.moveX = 0.0f;
        clearInput.moveY = 0.0f;
        clearInput.sprint = false;
        clearInput.timestamp = std::chrono::high_resolution_clock::now();
        clearInput.valid = false;
        m_currentInput = clearInput;
        return;
    }
    
    // Create new input
    PlayerInput newInput;
    newInput.moveX = moveX;
    newInput.moveY = moveY;
    newInput.sprint = sprint;
    newInput.timestamp = std::chrono::high_resolution_clock::now();
    newInput.valid = true;
    
    // Update current input
    m_currentInput = newInput;
    
    // Add to queue (with size limit to prevent lag accumulation)
    if (m_inputQueue.size() < MAX_INPUT_QUEUE_SIZE) {
        m_inputQueue.push(newInput);
    } else {
        // Remove oldest input if queue is full
        m_inputQueue.pop();
        m_inputQueue.push(newInput);
    }
    
    // Notify the movement thread
    m_inputAvailable.notify_one();
}

PlayerMovementManager::PlayerState PlayerMovementManager::getPlayerState() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_playerState;
}

void PlayerMovementManager::syncWithGameState()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    if (m_playerState.needsSync) {
        // Get the actual position from the game elements
        float actualX, actualY;
        if (getPlayerPosition(actualX, actualY)) {
            // Check if there's a significant discrepancy
            float deltaX = std::abs(actualX - m_playerState.x);
            float deltaY = std::abs(actualY - m_playerState.y);
            
            if (deltaX > 0.1f || deltaY > 0.1f) {
                std::cout << "Syncing player position: (" << m_playerState.x << ", " << m_playerState.y 
                          << ") -> (" << actualX << ", " << actualY << ")" << std::endl;
                m_playerState.x = actualX;
                m_playerState.y = actualY;
            }
        } else {
            // Player entity was destroyed - disable movement state
            if (m_playerState.isMoving) {
                std::cout << "Player entity destroyed during sync - disabling movement state" << std::endl;
                m_playerState.isMoving = false;
            }
        }
        
        m_playerState.needsSync = false;
    }
}

void PlayerMovementManager::playerMovementThread()
{
    std::cout << "Player movement thread started with " << PLAYER_UPDATE_FPS << " FPS target" << std::endl;
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    double accumulatedTime = 0.0;
    
    while (m_running.load()) {        // Check if paused and wait if necessary
        std::unique_lock<std::mutex> pauseLock(m_stateMutex);
        m_pauseCondition.wait(pauseLock, [this] { return !m_paused.load() || !m_running.load(); });
        pauseLock.unlock();
        
        // If we're no longer running after waiting, exit
        if (!m_running.load()) {
            break;
        }
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Accumulate time for fixed timestep
        accumulatedTime += elapsed;
        
        // Process movement at fixed timestep
        while (accumulatedTime >= PLAYER_UPDATE_TIMESTEP && m_running.load()) {
            auto updateStart = std::chrono::high_resolution_clock::now();
            
            // Get current input
            PlayerInput currentInput;
            {
                std::lock_guard<std::mutex> lock(m_inputMutex);
                if (!m_inputQueue.empty()) {
                    currentInput = m_inputQueue.front();
                    m_inputQueue.pop();
                } else {
                    currentInput = m_currentInput;
                }
            }
              // Process player movement if input is valid
            if (currentInput.valid) {
                processPlayerMovement(currentInput, PLAYER_UPDATE_TIMESTEP);
            }
              // Update camera at high frequency (120Hz) synchronized with player movement
            updateCamera(PLAYER_UPDATE_TIMESTEP);
            
            // Process win condition timing
            processWinCondition(PLAYER_UPDATE_TIMESTEP);
            
            // Process defeat condition timing
            processDefeatCondition(PLAYER_UPDATE_TIMESTEP);
            
            accumulatedTime -= PLAYER_UPDATE_TIMESTEP;
            m_movementUpdatesProcessed++;
            
            // Track performance
            auto updateEnd = std::chrono::high_resolution_clock::now();
            double updateTime = std::chrono::duration<double>(updateEnd - updateStart).count();
            m_averageUpdateTime.store((m_averageUpdateTime.load() * 0.9) + (updateTime * 0.1)); // Rolling average
        }
        
        // Sleep for a short time to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::microseconds(100));
          // Respect pause state
        {
            std::unique_lock<std::mutex> lock(m_stateMutex);
            m_pauseCondition.wait(lock, [this]() { return !m_paused.load() || !m_running.load(); });
        }
    }
    
    std::cout << "Player movement thread ended. Processed " << m_movementUpdatesProcessed.load() << " updates" << std::endl;
}

void PlayerMovementManager::processPlayerMovement(const PlayerInput& input, double deltaTime)
{
    // CRITICAL FIX: Check if player entity exists before processing any movement
    float playerX, playerY;
    if (!getPlayerPosition(playerX, playerY)) {
        // Player entity doesn't exist - disable all movement and camera updates
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_playerState.isMoving) {
            m_playerState.isMoving = false;
            std::cout << "Player entity destroyed - disabling movement controls" << std::endl;
            
            // Trigger defeat condition if not already triggered
            if (!m_defeatConditionTriggered) {
                std::lock_guard<std::mutex> defeatLock(m_defeatStateMutex);
                if (!m_defeatConditionTriggered) {
                    m_defeatConditionTriggered = true;
                    m_defeatDelayTimer = 0.0;
                    std::cout << "DEFEAT CONDITION TRIGGERED! Player entity no longer exists. Starting " << WAIT_BEFORE_WINNING_OR_LOSING << " second countdown..." << std::endl;
                }
            }
        }
        return;
    }
    
    // Skip if no movement
    if (input.moveX == 0.0f && input.moveY == 0.0f) {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_playerState.isMoving) {
            m_playerState.isMoving = false;
            
            // Disable animation in the main thread
            if (m_elementsManager) {
                m_elementsManager->changeElementAnimationStatus("player1", false);
                m_elementsManager->changeElementSpriteFrame("player1", 0);
            }
        }
        return;
    }
      // Get player configuration
    const EntityConfiguration* config = m_entitiesManager->getConfiguration(EntityName::PLAYER);
    if (!config) {
        std::cerr << "Player configuration not found in player movement thread" << std::endl;
        return;
    }
    
    // Change the player's facing direction based on input direction FIRST
    // Always use the original input moveX/Y for direction changes, regardless of whether movement will succeed
    // This ensures the player always faces the direction they're trying to move, even if blocked by collision
    if (input.moveX != 0.0f || input.moveY != 0.0f) {
        if (input.moveX > 0.0f && std::abs(input.moveX) > std::abs(input.moveY)) {
            // Trying to move right (and right movement is dominant)
            m_elementsManager->changeElementSpritePhase("player1", config->spritePhaseWalkRight);
        } else if (input.moveX < 0.0f && std::abs(input.moveX) > std::abs(input.moveY)) {
            // Trying to move left (and left movement is dominant)
            m_elementsManager->changeElementSpritePhase("player1", config->spritePhaseWalkLeft);
        } else if (input.moveY > 0.0f) {
            // Trying to move up (or up movement is dominant)
            m_elementsManager->changeElementSpritePhase("player1", config->spritePhaseWalkUp);
        } else if (input.moveY < 0.0f) {
            // Trying to move down (or down movement is dominant)
            m_elementsManager->changeElementSpritePhase("player1", config->spritePhaseWalkDown);
        }
    }
    
    // Calculate movement speed based on sprint state
    float speed = input.sprint ? config->sprintWalkingSpeed : config->normalWalkingSpeed;
    
    // Calculate movement delta for this frame
    float deltaX = input.moveX * speed * static_cast<float>(deltaTime);
    float deltaY = input.moveY * speed * static_cast<float>(deltaTime);
    
    // Normalize diagonal movement
    if (deltaX != 0.0f && deltaY != 0.0f) {
        float normalizeFactor = 0.7071f; // 1/sqrt(2)
        deltaX *= normalizeFactor;
        deltaY *= normalizeFactor;
    }
    
    // Check collision and get actual movement
    float actualDeltaX, actualDeltaY;
    bool canMove = false;
    
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        float newX = m_playerState.x + deltaX;
        float newY = m_playerState.y + deltaY;
        canMove = checkPlayerCollision(newX, newY, actualDeltaX, actualDeltaY);
        m_collisionChecksPerformed++;
    }    // Update player position if movement is possible
    if (canMove) {
        updatePlayerPosition(actualDeltaX, actualDeltaY);        // Check for water damage after player movement
        if (m_entitiesManager) {
            checkAndApplyDamageBlocksToEntity("player1", *m_entitiesManager);
            
            // Check for defeat condition after damage application
            Entity* player = m_entitiesManager->getEntity("player1");
            if (player && player->lifePoints <= 0 && !m_defeatConditionTriggered) {
                std::lock_guard<std::mutex> defeatLock(m_defeatStateMutex);
                m_defeatConditionTriggered = true;
                m_defeatDelayTimer = 0.0;
                std::cout << "DEFEAT CONDITION TRIGGERED! Player has " << player->lifePoints 
                          << " life points. Starting " << WAIT_BEFORE_WINNING_OR_LOSING << " second countdown..." << std::endl;
            }
        }
        
        // Check for coconuts to collect after movement
        checkAndCollectCoconuts();
    }
    
    // Update movement state
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        bool wasMoving = m_playerState.isMoving;
        m_playerState.isMoving = canMove;
        m_playerState.lastUpdate = std::chrono::high_resolution_clock::now();
        
        // Handle animation state changes
        if (canMove && !wasMoving) {
            // Started moving - enable animation
            if (m_elementsManager) {
                m_elementsManager->changeElementAnimationStatus("player1", true);
                float animSpeed = input.sprint ? config->sprintWalkingAnimationSpeed : config->normalWalkingAnimationSpeed;
                m_elementsManager->changeElementAnimationSpeed("player1", animSpeed);
            }
        } else if (!canMove && wasMoving) {
            // Stopped moving - disable animation
            if (m_elementsManager) {
                m_elementsManager->changeElementAnimationStatus("player1", false);
                m_elementsManager->changeElementSpriteFrame("player1", 0);
            }
        }
    }
}

void PlayerMovementManager::updatePlayerPosition(float deltaX, float deltaY)
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    // Update internal state
    m_playerState.x += deltaX;
    m_playerState.y += deltaY;
    m_playerState.needsSync = true;
      // Update actual game element position
    if (m_elementsManager) {
        m_elementsManager->moveElement("player1", deltaX, deltaY);
    }
}

bool PlayerMovementManager::checkPlayerCollision(float newX, float newY, float& actualDeltaX, float& actualDeltaY) const
{
    const EntityConfiguration* config = m_entitiesManager->getConfiguration(EntityName::PLAYER);
    if (!config) {
        actualDeltaX = 0.0f;
        actualDeltaY = 0.0f;
        return false;
    }
    
    float deltaX = newX - m_playerState.x;
    float deltaY = newY - m_playerState.y;
    
    // Check if the combined movement would collide
    bool collisionWithElement = wouldEntityCollideWithElementsGranular(*config, newX, newY, false);
    bool collisionWithBlock = wouldEntityCollideWithBlocksGranular(*config, newX, newY, false);
    bool collisionWithEntity = wouldEntityCollideWithEntitiesGranular(*config, newX, newY, false, "player1");
    bool canMove = !(collisionWithElement || collisionWithBlock || collisionWithEntity);
    
    if (canMove) {
        // Can move diagonally
        actualDeltaX = deltaX;
        actualDeltaY = deltaY;
        return true;
    }
    
    // Try axis-separated movement if diagonal fails
    if (deltaX != 0 && deltaY != 0) {
        // Try horizontal only
        float testX = m_playerState.x + deltaX;
        float testY = m_playerState.y;
        bool horizontalCollision = wouldEntityCollideWithElementsGranular(*config, testX, testY, false) ||
                                  wouldEntityCollideWithBlocksGranular(*config, testX, testY, false) ||
                                  wouldEntityCollideWithEntitiesGranular(*config, testX, testY, false, "player1");
        
        // Try vertical only
        float testX2 = m_playerState.x;
        float testY2 = m_playerState.y + deltaY;
        bool verticalCollision = wouldEntityCollideWithElementsGranular(*config, testX2, testY2, false) ||
                                wouldEntityCollideWithBlocksGranular(*config, testX2, testY2, false) ||
                                wouldEntityCollideWithEntitiesGranular(*config, testX2, testY2, false, "player1");
        
        // Set actual movement based on what's possible
        actualDeltaX = horizontalCollision ? 0.0f : deltaX;
        actualDeltaY = verticalCollision ? 0.0f : deltaY;
        
        return (!horizontalCollision || !verticalCollision);
    }
    
    // Single-axis movement that's blocked
    actualDeltaX = 0.0f;
    actualDeltaY = 0.0f;    return false;
}

void PlayerMovementManager::updateCamera(double deltaTime)
{
    if (!m_camera) {
        return;
    }
    
    // CRITICAL FIX: Check if player entity exists before updating camera
    float playerX, playerY;
    bool playerExists = getPlayerPosition(playerX, playerY);
    
    // Always update smooth transitions to maintain visual consistency
    m_camera->updateSmoothTransitions(static_cast<float>(deltaTime));
    
    if (!playerExists) {
        // Player entity doesn't exist - don't update camera position
        // Only use the last known position stored in the camera
        return;
    }
    
    // Get current player position (thread-safe)
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        playerX = m_playerState.x;
        playerY = m_playerState.y;
        
        // Update the last known position in thread state
        m_lastKnownPlayerX = playerX;
        m_lastKnownPlayerY = playerY;
    }
    
    // Update camera position based on current player position at 120Hz for smooth following
    extern int windowWidth, windowHeight; // From globals.h
    m_camera->updateCameraPosition(playerX, playerY, windowWidth, windowHeight);
}

void PlayerMovementManager::checkAndCollectCoconuts() {
    // Get player position
    float playerX, playerY;
    if (!getPlayerPosition(playerX, playerY)) {
        return; // Can't get player position
    }
    
    // Check for coconuts within 1 block radius
    const float COCONUT_PICKUP_RADIUS = 1.0f;
    std::vector<std::string> coconutsToRemove;
    
    // Get all elements and check for nearby coconuts
    const auto& elements = m_elementsManager->getElements();
    
    for (const auto& element : elements) {
        // Check if this element is a coconut
        if (element.elementName == ElementName::COCONUT) {
            // Calculate distance between player and coconut
            float dx = playerX - element.x;
            float dy = playerY - element.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            // If coconut is within pickup radius, mark it for removal
            if (distance <= COCONUT_PICKUP_RADIUS) {
                coconutsToRemove.push_back(element.instanceName);
            }
        }
    }
      // Remove the coconuts and increment counter
    for (const std::string& coconutName : coconutsToRemove) {        if (m_elementsManager->removeElement(coconutName)) {
            COCONUT_COUNTER++;
            std::cout << "Collected coconut! Total coconuts: " << COCONUT_COUNTER << std::endl;
            
            // Check for win condition
            if (COCONUT_COUNTER >= 3 && !m_winConditionTriggered) {
                std::lock_guard<std::mutex> winLock(m_winStateMutex);
                m_winConditionTriggered = true;
                m_winDelayTimer = 0.0;
                std::cout << "WIN CONDITION TRIGGERED! Starting " << WAIT_BEFORE_WINNING_OR_LOSING << " second countdown..." << std::endl;
            }
            
            // Update the COCONUTS UI element sprite phase based on counter value
            int targetPhase;
            if (COCONUT_COUNTER <= 0) {
                targetPhase = 3;  // 0 or below = phase 3
            } else if (COCONUT_COUNTER == 1) {
                targetPhase = 2;  // 1 = phase 2
            } else if (COCONUT_COUNTER == 2) {
                targetPhase = 1;  // 2 = phase 1
            } else { // 3 or more
                targetPhase = 0;  // 3+ = phase 0
            }
            
            // Update the UI element
            extern GameMenus gameMenus;
            bool success = gameMenus.changeUIElementSpriteSheetPhase(UIElementName::COCONUTS, targetPhase);
            if (success) {
                std::cout << "Updated coconuts UI: counter=" << COCONUT_COUNTER 
                          << " -> phase=" << targetPhase << std::endl;
            } else {
                std::cout << "Failed to update coconuts UI for counter value: " << COCONUT_COUNTER << std::endl;
            }        }
    }
}

void PlayerMovementManager::processWinCondition(double deltaTime)
{
    std::lock_guard<std::mutex> winLock(m_winStateMutex);
    
    if (m_winConditionTriggered) {
        m_winDelayTimer += deltaTime;
          if (m_winDelayTimer >= WAIT_BEFORE_WINNING_OR_LOSING) {
            // Time's up - trigger the win state
            std::cout << "WIN! Player collected 3 coconuts - game won!" << std::endl;
            
            // Set game state to WIN immediately
            GAME_STATE = GameState::WIN;
            std::cout << "Game state set to: " << gameStateToString(GAME_STATE) << std::endl;
            
            // Signal main thread to show WIN menu (avoid OpenGL context issues)
            SHOULD_SHOW_WIN_MENU = true;
            std::cout << "WIN menu display requested" << std::endl;
            
            // Wait additional time to allow UI to render properly before pausing
            if (m_winDelayTimer >= WAIT_BEFORE_WINNING_OR_LOSING + 0.5) {
                // Force pause the game AFTER giving UI time to render
                if (g_threadManager) {
                    g_threadManager->pauseGame();
                    std::cout << "Game forcibly paused for win condition" << std::endl;
                }
                
                // Reset win condition state (prevent retriggering)
                m_winConditionTriggered = false;
                m_winDelayTimer = 0.0;
            } else {
                // Still waiting for UI to stabilize
                float remainingTime = (WAIT_BEFORE_WINNING_OR_LOSING + 0.5) - m_winDelayTimer;
                std::cout << "WIN menu stabilizing... " << remainingTime << " seconds before pause..." << std::endl;
            }
        } else {
            // Still counting down
            float remainingTime = WAIT_BEFORE_WINNING_OR_LOSING - m_winDelayTimer;
            std::cout << "Win countdown: " << remainingTime << " seconds remaining..." << std::endl;
        }
    }
}

void PlayerMovementManager::processDefeatCondition(double deltaTime)
{
    std::lock_guard<std::mutex> defeatLock(m_defeatStateMutex);
    
    if (m_defeatConditionTriggered) {
        m_defeatDelayTimer += deltaTime;
          if (m_defeatDelayTimer >= WAIT_BEFORE_WINNING_OR_LOSING) {
            // Time's up - trigger the defeat state
            std::cout << "DEFEAT! Player has been defeated - game lost!" << std::endl;
            
            // Set game state to DEFEAT immediately
            GAME_STATE = GameState::DEFEAT;
            std::cout << "Game state set to: " << gameStateToString(GAME_STATE) << std::endl;
            
            // Signal main thread to show GAME_OVER menu (avoid OpenGL context issues)
            SHOULD_SHOW_GAME_OVER = true;
            std::cout << "GAME_OVER menu display requested" << std::endl;
            
            // Wait additional time to allow UI to render properly before pausing
            if (m_defeatDelayTimer >= WAIT_BEFORE_WINNING_OR_LOSING + 0.5) {
                // Force pause the game AFTER giving UI time to render
                if (g_threadManager) {
                    g_threadManager->pauseGame();
                    std::cout << "Game forcibly paused for defeat condition" << std::endl;
                }
                
                // Reset defeat condition state (prevent retriggering)
                m_defeatConditionTriggered = false;
                m_defeatDelayTimer = 0.0;
            } else {
                // Still waiting for UI to stabilize
                float remainingTime = (WAIT_BEFORE_WINNING_OR_LOSING + 0.5) - m_defeatDelayTimer;
                std::cout << "GAME_OVER menu stabilizing... " << remainingTime << " seconds before pause..." << std::endl;
            }
        } else {
            // Still counting down
            float remainingTime = WAIT_BEFORE_WINNING_OR_LOSING - m_defeatDelayTimer;
            std::cout << "Defeat countdown: " << remainingTime << " seconds remaining..." << std::endl;
        }    }
}

void PlayerMovementManager::triggerDefeatCondition()
{
    std::lock_guard<std::mutex> defeatLock(m_defeatStateMutex);
    if (!m_defeatConditionTriggered) {
        m_defeatConditionTriggered = true;
        m_defeatDelayTimer = 0.0;
        std::cout << "DEFEAT CONDITION TRIGGERED EXTERNALLY! Starting " << WAIT_BEFORE_WINNING_OR_LOSING << " second countdown..." << std::endl;
    }
}

void PlayerMovementManager::resetGameConditions()
{
    // Reset win condition state
    {
        std::lock_guard<std::mutex> lock(m_winStateMutex);
        m_winConditionTriggered = false;
        m_winDelayTimer = 0.0;
    }
    
    // Reset defeat condition state  
    {
        std::lock_guard<std::mutex> lock(m_defeatStateMutex);
        m_defeatConditionTriggered = false;
        m_defeatDelayTimer = 0.0;
    }
    
    std::cout << "Reset PlayerMovementManager win/defeat conditions for new gameplay session" << std::endl;
}

void PlayerMovementManager::pauseMovement()
{
    m_paused.store(true);
    std::cout << "Player movement paused" << std::endl;
}

void PlayerMovementManager::resumeMovement()
{
    m_paused.store(false);
    
    // Clear any accumulated inputs when resuming to prevent teleportation
    {
        std::lock_guard<std::mutex> lock(m_inputMutex);
        
        // Clear the input queue
        std::queue<PlayerInput> empty;
        std::swap(m_inputQueue, empty);
        
        // Clear current input
        PlayerInput clearInput;
        clearInput.moveX = 0.0f;
        clearInput.moveY = 0.0f;
        clearInput.sprint = false;
        clearInput.timestamp = std::chrono::high_resolution_clock::now();
        clearInput.valid = false;
        m_currentInput = clearInput;
    }
    
    m_pauseCondition.notify_all();
    std::cout << "Player movement resumed" << std::endl;
}

// Convenience functions implementation
bool initializePlayerMovement(Map* gameMap, ElementsOnMap* elementsManager, EntitiesManager* entitiesManager, Camera* camera)
{
    if (g_playerMovementManager != nullptr) {
        std::cout << "Player movement manager already initialized" << std::endl;
        return true;
    }
    
    g_playerMovementManager = new PlayerMovementManager();
    return g_playerMovementManager->initialize(gameMap, elementsManager, entitiesManager, camera);
}

void startPlayerMovementThread()
{
    if (g_playerMovementManager != nullptr) {
        g_playerMovementManager->startThread();
    }
}

void stopPlayerMovementThread()
{
    if (g_playerMovementManager != nullptr) {
        g_playerMovementManager->stopThread();
    }
}

void cleanupPlayerMovement()
{
    if (g_playerMovementManager != nullptr) {
        delete g_playerMovementManager;
        g_playerMovementManager = nullptr;
    }
}
