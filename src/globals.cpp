#include "globals.h"
#include "GLFW/glfw3.h" // For GLFW_KEY_LAST
#include "enumDefinitions.h"


// Constants

// Framerate
const double FRAMERATE_IN_SECONDS = 1. / 60.; // 60 FPS

// Grid properties
const int GRID_SIZE = 170;
// Player speeds are defined in entity configuration in entities.cpp
const float PLAYER_BASE_SPEED = 3.0f;   // DEPRECATED: Use playerConfig->normalWalkingSpeed instead
const float PLAYER_SPRINT_SPEED = 6.0f; // DEPRECATED: Use playerConfig->sprintWalkingSpeed instead

// Terrain generation parameters
/* float islandFeatureSize = 0.1f; // Controls the size of islands. Smaller values = smaller islands
float seaFeatureSize = 0.016f;  // Controls the size of sea areas */

// Terrain generation parameters
float islandFeatureSize = 1.0f; // Controls the size of islands. Smaller values = smaller islands
float seaFeatureSize = 0.1f;  // Controls the size of sea areas

// Rendering parameters
float gridLineWidth = 1.0f;
int windowWidth = 1920;
int windowHeight = 1080;
float aspectRatio = 1.0f;

// Grid rendering parameters for coordinate conversion
float g_startX = -1.0f;
float g_endX = 1.0f;
float g_startY = -1.0f;
float g_endY = 1.0f;

// Boolean flags for visibility features
bool showGridLines = false;
bool hideOutsideGrid = false; // Controls whether to hide pixels outside the map grid

// Debug flags
bool DEBUG_MAP = false; // Set to true to use simplified debug map
bool DEBUG_SHOW_PATHS = false; // Set to true to display entity paths

// Debug logging control
bool DEBUG_LOGS = false;

// Game counters
int COCONUT_COUNTER = 0; // Initialize coconut counter to 0

// Game state tracking
GameState GAME_STATE = GameState::START;

// Win condition flags
bool SHOULD_SHOW_WIN_MENU = false;

// Defeat condition flags
bool SHOULD_SHOW_GAME_OVER = false;// Initialize game state to START

// Game timing for win/loss conditions
const float WAIT_BEFORE_WINNING_OR_LOSING = 2.0f; // Wait 2 seconds before showing win/loss screen

// Input handling
bool keyPressedStates[GLFW_KEY_LAST + 1] = { false };
