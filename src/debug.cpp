#include "debug.h"
#include "elementsOnMap.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "enumDefinitions.h"


// Global state for keyboard handling of debug features
static bool lastFrameAnchorPointKeyState = false;
static bool lastFrameCollisionBoxKeyState = false;
static bool showCollisionBoxes = false;

// Function to draw anchor point visualization for an element
void drawAnchorPoint(float anchorX, float anchorY) {
    // Draw a small crosshair to indicate the anchor point
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);  // Red color
    glLineWidth(3.0f);
    
    // Draw horizontal line at the actual anchor position
    glBegin(GL_LINES);
    glVertex2f(anchorX - 0.02f, anchorY);
    glVertex2f(anchorX + 0.02f, anchorY);
    glEnd();
    
    // Draw vertical line at the actual anchor position
    glBegin(GL_LINES);
    glVertex2f(anchorX, anchorY - 0.02f);
    glVertex2f(anchorX, anchorY + 0.02f);
    glEnd();
    
    glLineWidth(1.0f);  // Reset line width
}

// Function to draw collision box visualization
void drawCollisionBox(float x, float y, float radius) {
    // Draw a blue circle to represent the collision radius
    glColor4f(0.0f, 0.5f, 1.0f, 0.5f);  // Semi-transparent blue
    
    // Draw filled circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);  // Center point
    const int numSegments = 20;
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * 2.0f * 3.14159f / numSegments;
        float dx = radius * cosf(angle);
        float dy = radius * sinf(angle);
        glVertex2f(x + dx, y + dy);
    }
    glEnd();
    
    // Draw circle outline
    glColor4f(0.0f, 0.0f, 1.0f, 0.8f);  // More solid blue for the outline
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < numSegments; i++) {
        float angle = i * 2.0f * 3.14159f / numSegments;
        float dx = radius * cosf(angle);
        float dy = radius * sinf(angle);
        glVertex2f(x + dx, y + dy);
    }
    glEnd();
    glLineWidth(1.0f);  // Reset line width
}

// Function to toggle collision box visualization
void toggleCollisionBoxVisualization() {
    showCollisionBoxes = !showCollisionBoxes;
    std::cout << "Collision box visualization " << (showCollisionBoxes ? "enabled" : "disabled") << std::endl;
}

// Check if collision boxes should be visualized
bool isShowingCollisionBoxes() {
    return showCollisionBoxes;
}

// Handle debug key presses, including anchor point and collision box visualizations
void handleDebugKeys(ElementsOnMap& elementsManager, bool* keyPressedStates) {
    // Toggle anchor point visualization with F5
    if (keyPressedStates[GLFW_KEY_F5] && !lastFrameAnchorPointKeyState) {
        elementsManager.toggleAnchorPointVisualization();
    }
    lastFrameAnchorPointKeyState = keyPressedStates[GLFW_KEY_F5];
    
    // Toggle collision box visualization with F7
    if (keyPressedStates[GLFW_KEY_F7] && !lastFrameCollisionBoxKeyState) {
        toggleCollisionBoxVisualization();
    }
    lastFrameCollisionBoxKeyState = keyPressedStates[GLFW_KEY_F7];
}
