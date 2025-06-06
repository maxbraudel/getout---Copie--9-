#pragma once
#include "enumDefinitions.h"


// Forward declarations
struct GLFWwindow;

// Constants
extern const double FRAMERATE_IN_SECONDS;
extern const int GRID_SIZE;
// DEPRECATED: Use entity configuration instead (playerConfig->normalWalkingSpeed and playerConfig->sprintWalkingSpeed)
extern const float PLAYER_BASE_SPEED;
extern const float PLAYER_SPRINT_SPEED;

// Terrain generation parameters
extern float islandFeatureSize;
extern float seaFeatureSize;

// Rendering parameters
extern float gridLineWidth;
extern int windowWidth;
extern int windowHeight;
extern float aspectRatio;

// Grid rendering parameters
extern float g_startX;
extern float g_endX;
extern float g_startY;
extern float g_endY;

// Boolean flags for visibility features
extern bool showGridLines;
extern bool hideOutsideGrid;

// Debug flags
extern bool DEBUG_MAP; // When true, uses a simplified debug map instead of Perlin noise
extern bool DEBUG_SHOW_PATHS; // When true, displays entity paths
// Debug logging control
extern bool DEBUG_LOGS;

// Game counters
extern int COCONUT_COUNTER; // Tracks the number of coconuts collected by the player

// Game state tracking
extern GameState GAME_STATE; // Tracks the current game state (START, DEFEAT, PAUSE, WIN)

// Win condition flags
extern bool SHOULD_SHOW_WIN_MENU; // Flag to indicate WIN menu should be displayed

// Game timing for win/loss conditions
extern const float WAIT_BEFORE_WINNING_OR_LOSING; // Time to wait before showing win/loss screen

// Input handling
extern bool keyPressedStates[]; // GLFW_KEY_LAST + 1
