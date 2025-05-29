#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <memory>
#include <mutex>
#include "InputManager.h"
#include "enumDefinitions.h"


// Forward declarations
class Map;
class ElementsOnMap;
class EntitiesManager;
class Camera;

/**
 * Game state structure containing all game-related state information
 */
struct GameState {
    float playerX = 0.0f;
    float playerY = 0.0f;
    double currentTime = 0.0;
    double deltaTime = 0.0;
    bool playerMoving = false;
};

/**
 * Game Logic - handles all game rules, state management, and entity updates
 * Pure game logic without any rendering or input handling concerns
 */
class GameLogic {
public:
    GameLogic();
    ~GameLogic();
    
    bool initialize(Map* gameMap, ElementsOnMap* elementsManager, 
                   EntitiesManager* entitiesManager, Camera* camera);
    
    void update(double deltaTime);
    void processInput(const InputState& input);
    
    GameState getGameState() const;
    
    // Access to game systems for rendering
    Map* getGameMap() const { return m_gameMap; }
    ElementsOnMap* getElementsManager() const { return m_elementsManager; }
    Camera* getCamera() const { return m_camera; }
    
private:
    // Game systems references (not owned)
    Map* m_gameMap;
    ElementsOnMap* m_elementsManager;
    EntitiesManager* m_entitiesManager;
    Camera* m_camera;
    
    // Game state
    GameState m_gameState;
    mutable std::mutex m_stateMutex;
    
    // Timing and state
    double m_gameTime;
    double m_lastAntagonistMoveTime;
    bool m_wasPlayerMoving;
      // Internal methods
    void updatePlayer(const InputState& input);
    void updateEntities(double deltaTime);
    void updateAntagonists();
    void updateCamera(double deltaTime);
    void updateGameState(double deltaTime, bool playerMoving);
    
    static constexpr double ANTAGONIST_MOVE_INTERVAL = 5.0;
};

#endif // GAME_LOGIC_H
