#pragma once

// Forward declarations
struct GLFWwindow;

// Constants
extern const double FRAMERATE_IN_SECONDS;
extern const int GRID_SIZE;
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

// Input handling
extern bool keyPressedStates[]; // GLFW_KEY_LAST + 1
