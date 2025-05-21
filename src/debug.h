#ifndef DEBUG_H
#define DEBUG_H

// Forward declarations
class ElementsOnMap;

// Function to draw anchor point visualization for an element
void drawAnchorPoint(float anchorX, float anchorY);

// Function to draw collision box visualization
void drawCollisionBox(float x, float y, float radius);

// Function to toggle collision box visualization
void toggleCollisionBoxVisualization();

// Check if collision boxes should be visualized
bool isShowingCollisionBoxes();

// Handle debug key presses, including anchor point and collision box visualizations
void handleDebugKeys(ElementsOnMap& elementsManager, bool* keyPressedStates);

#endif // DEBUG_H
