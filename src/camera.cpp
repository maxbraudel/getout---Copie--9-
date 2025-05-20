#include "camera.h"
#include "globals.h"
#include <iostream>

// Define static constants
const float Camera::MIN_CAMERA_REGION = 5.0f; // Minimum camera region size
const float Camera::MAX_CAMERA_REGION = 200.0f; // Maximum camera region size
const float Camera::DEFAULT_CAMERA_REGION = 15.0f; // Default camera region size

// Global camera instance
Camera gameCamera(GRID_SIZE); // Initialize with GRID_SIZE from globals.h

Camera::Camera(int gridSize) 
    : m_cameraRegion(DEFAULT_CAMERA_REGION), // Use the defined constant
      m_left(0.0f),
      m_right(0.0f), 
      m_bottom(0.0f), 
      m_top(0.0f),
      m_gridSize(gridSize) 
{
    // Constructor body can be empty as we've initialized everything in the initializer list
}

void Camera::increaseCameraRegion(float amount) {
    setCameraRegion(m_cameraRegion + amount);
    std::cout << "Zoomed out: Camera region set to: " << m_cameraRegion << std::endl;
}

void Camera::decreaseCameraRegion(float amount) {
    setCameraRegion(m_cameraRegion - amount);
    std::cout << "Zoomed in: Camera region set to: " << m_cameraRegion << std::endl;
}

void Camera::setCameraRegion(float value) {
    // Clamp the value to the valid range
    m_cameraRegion = std::min(std::max(value, MIN_CAMERA_REGION), MAX_CAMERA_REGION);
}

float Camera::getCameraRegion() const {
    return m_cameraRegion;
}

void Camera::updateCameraPosition(float playerX, float playerY, int windowWidth, int windowHeight) {
    // Calculate adaptive camera view that matches window aspect ratio
    float windowAspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    
    // Calculate half-width and half-height of camera view based on aspect ratio
    float cameraHalfWidth, cameraHalfHeight;
    
    if (windowAspectRatio >= 1.0f) {
        // Window is wider than tall (or square) - expand width
        cameraHalfHeight = m_cameraRegion;
        cameraHalfWidth = cameraHalfHeight * windowAspectRatio;
    } else {
        // Window is taller than wide - expand height
        cameraHalfWidth = m_cameraRegion;
        cameraHalfHeight = cameraHalfWidth / windowAspectRatio;
    }
    
    // Center camera on player
    m_left = playerX - cameraHalfWidth;
    m_right = playerX + cameraHalfWidth;
    m_bottom = playerY - cameraHalfHeight;
    m_top = playerY + cameraHalfHeight;
    
    // Calculate camera width and height
    float cameraWidth = cameraHalfWidth * 2.0f;
    float cameraHeight = cameraHalfHeight * 2.0f;
    
    // Ensure the camera doesn't go out of bounds
    if (m_left < 0) {
        m_left = 0;
        m_right = cameraWidth;
    }
    if (m_right > m_gridSize) {
        m_right = m_gridSize;
        m_left = m_gridSize - cameraWidth;
        if (m_left < 0) m_left = 0; // Additional safety check
    }
    if (m_bottom < 0) {
        m_bottom = 0;
        m_top = cameraHeight;
    }
    if (m_top > m_gridSize) {
        m_top = m_gridSize;
        m_bottom = m_gridSize - cameraHeight;
        if (m_bottom < 0) m_bottom = 0; // Additional safety check
    }
    
    // Further adjust if the grid is smaller than the camera view
    if (m_gridSize < cameraWidth) {
        // Grid is narrower than camera view - center horizontally
        m_left = 0;
        m_right = m_gridSize;
    }
    if (m_gridSize < cameraHeight) {
        // Grid is shorter than camera view - center vertically
        m_bottom = 0;
        m_top = m_gridSize;
    }
}

float Camera::getLeft() const { return m_left; }
float Camera::getRight() const { return m_right; }
float Camera::getBottom() const { return m_bottom; }
float Camera::getTop() const { return m_top; }
float Camera::getWidth() const { return m_right - m_left; }
float Camera::getHeight() const { return m_top - m_bottom; }
