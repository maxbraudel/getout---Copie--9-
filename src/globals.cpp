#include "globals.h"
#include "GLFW/glfw3.h" // For GLFW_KEY_LAST

// Constants

// Framerate
const double FRAMERATE_IN_SECONDS = 1. / 60.; // 60 FPS

// Grid properties
const int GRID_SIZE = 300;
const float PLAYER_BASE_SPEED = 4.0f;   // Base speed of player movement (grid units per second)
const float PLAYER_SPRINT_SPEED = 6.0f; // Sprint speed when shift is held (grid units per second)

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

// Input handling
bool keyPressedStates[GLFW_KEY_LAST + 1] = { false };
