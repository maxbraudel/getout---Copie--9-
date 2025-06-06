#include "camera.h"
#include "globals.h"
#include <iostream>
#include <cmath> // For pow()
#include "enumDefinitions.h"


// Define static constants
const float Camera::MIN_CAMERA_REGION = 5.0f; // Minimum camera region size
const float Camera::MAX_CAMERA_REGION = 200.0f; // Maximum camera region size
const float Camera::DEFAULT_CAMERA_REGION = 14.0f; // Default camera region size

// Global camera instance
Camera gameCamera(GRID_SIZE); // Initialize with GRID_SIZE from globals.h

Camera::Camera(int gridSize) 
    : m_cameraRegion(DEFAULT_CAMERA_REGION), // Use the defined constant
      m_desiredCameraRegion(DEFAULT_CAMERA_REGION), // Initialize desired region to default
      m_left(0.0f),
      m_right(0.0f), 
      m_bottom(0.0f), 
      m_top(0.0f),
      m_gridSize(gridSize),
      m_isTransitioning(false),
      m_transitionStartRegion(0.0f),
      m_transitionTargetRegion(0.0f),
      m_transitionDuration(0.0f),
      m_transitionElapsed(0.0f),
      m_lastKnownPlayerX(gridSize / 2.0f),
      m_lastKnownPlayerY(gridSize / 2.0f),
      m_hasLastKnownPosition(false)
{
    // Constructor body can be empty as we've initialized everything in the initializer list
}

void Camera::increaseCameraRegion(float amount) {
    float oldDesiredRegion = m_desiredCameraRegion;
    
    // Update the desired camera region (user's intention)
    m_desiredCameraRegion = std::min(std::max(m_desiredCameraRegion + amount, MIN_CAMERA_REGION), MAX_CAMERA_REGION);
    
    // Apply window constraints to get the actual camera region
    float maxAllowedRegion = getMaxUsableCameraRegion(windowWidth, windowHeight);
    m_cameraRegion = std::min(m_desiredCameraRegion, maxAllowedRegion);
    
    if (m_desiredCameraRegion == oldDesiredRegion) {
        std::cout << "Maximum zoom out reached (desired region: " << m_desiredCameraRegion 
                  << ", max allowed: " << MAX_CAMERA_REGION << ")" << std::endl;
    } else if (m_cameraRegion < m_desiredCameraRegion) {
        std::cout << "Zoomed out: Desired region " << m_desiredCameraRegion 
                  << " (limited to " << m_cameraRegion << " by window size)" << std::endl;
    } else {
        std::cout << "Zoomed out: Camera region set to: " << m_cameraRegion << std::endl;
    }
}

void Camera::decreaseCameraRegion(float amount) {
    float oldDesiredRegion = m_desiredCameraRegion;
    
    // Update the desired camera region (user's intention)
    m_desiredCameraRegion = std::min(std::max(m_desiredCameraRegion - amount, MIN_CAMERA_REGION), MAX_CAMERA_REGION);
    
    // Apply window constraints to get the actual camera region
    float maxAllowedRegion = getMaxUsableCameraRegion(windowWidth, windowHeight);
    m_cameraRegion = std::min(m_desiredCameraRegion, maxAllowedRegion);
    
    if (m_desiredCameraRegion == oldDesiredRegion) {
        std::cout << "Maximum zoom in reached (desired region: " << m_desiredCameraRegion 
                  << ", min allowed: " << MIN_CAMERA_REGION << ")" << std::endl;
    } else {
        std::cout << "Zoomed in: Camera region set to: " << m_cameraRegion << std::endl;
    }
}

void Camera::setCameraRegion(float value) {
    // Set both desired and actual camera region (used for internal adjustments)
    m_desiredCameraRegion = std::min(std::max(value, MIN_CAMERA_REGION), MAX_CAMERA_REGION);
    m_cameraRegion = m_desiredCameraRegion;
}

void Camera::setCameraRegionWithWindowClamp(float value) {
    // Update the desired camera region (user's intention)
    m_desiredCameraRegion = std::min(std::max(value, MIN_CAMERA_REGION), MAX_CAMERA_REGION);
    
    // Calculate maximum allowed camera region based on current window dimensions
    float maxAllowedRegion = getMaxUsableCameraRegion(windowWidth, windowHeight);
    
    // Apply window constraints to get the actual camera region
    m_cameraRegion = std::min(m_desiredCameraRegion, maxAllowedRegion);
}

float Camera::getCameraRegion() const {
    return m_cameraRegion;
}

float Camera::getMaxCameraRegion() const {
    // Calculate maximum allowed camera region based on grid size
    // Use a more conservative limit to ensure we never exceed boundaries
    return std::min(static_cast<float>(m_gridSize) / 2.5f, MAX_CAMERA_REGION);
}

float Camera::getMaxUsableCameraRegion(int windowWidth, int windowHeight) const {
    // Calculate the maximum camera region based on current window aspect ratio
    float windowAspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    
    // Calculate what the maximum camera region should be to fit within the map
    float maxRegionByWidth, maxRegionByHeight;
    
    if (windowAspectRatio >= 1.0f) {
        // Window is wider than tall - width is expanded, height is the base
        maxRegionByHeight = static_cast<float>(m_gridSize) / 2.0f;
        maxRegionByWidth = maxRegionByHeight / windowAspectRatio;
    } else {
        // Window is taller than wide - height is expanded, width is the base  
        maxRegionByWidth = static_cast<float>(m_gridSize) / 2.0f;
        maxRegionByHeight = maxRegionByWidth * windowAspectRatio;
    }
    
    // Use the more restrictive limit and ensure it's not smaller than minimum
    float maxUsableRegion = std::min(maxRegionByWidth, maxRegionByHeight);
    maxUsableRegion = std::max(maxUsableRegion, MIN_CAMERA_REGION);
    
    // Also respect the absolute maximum
    return std::min(maxUsableRegion, MAX_CAMERA_REGION);
}

void Camera::updateCameraPosition(float playerX, float playerY, int windowWidth, int windowHeight) {
    // Store this as the last known player position
    m_lastKnownPlayerX = playerX;
    m_lastKnownPlayerY = playerY;
    m_hasLastKnownPosition = true;
    
    // Calculate adaptive camera view that matches window aspect ratio
    float windowAspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    
    // Always try to use the user's desired camera region first
    float targetCameraRegion = m_desiredCameraRegion;
    
    // Calculate maximum allowed camera region for current window
    float maxAllowedRegion = getMaxUsableCameraRegion(windowWidth, windowHeight);
    
    // If the desired region fits within current window constraints, use it
    // Otherwise, clamp it to the maximum allowed
    bool wasAdjusted = false;
    if (targetCameraRegion > maxAllowedRegion) {
        targetCameraRegion = maxAllowedRegion;
        wasAdjusted = true;
        
        // Only show message if this is a significant change (not just minor floating point differences)
        if (std::abs(m_cameraRegion - targetCameraRegion) > 0.1f) {
            std::cout << "Zoom temporarily limited to " << targetCameraRegion 
                      << " due to window size (desired: " << m_desiredCameraRegion << ")" << std::endl;
        }
    } else if (m_cameraRegion < m_desiredCameraRegion && !wasAdjusted) {
        // The desired region now fits - restore it
        if (std::abs(m_cameraRegion - targetCameraRegion) > 0.1f) {
            std::cout << "Zoom restored to desired level: " << targetCameraRegion << std::endl;
        }
    }
    
    // Update the actual camera region
    m_cameraRegion = targetCameraRegion;
    
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
    
    // Calculate camera width and height
    float cameraWidth = cameraHalfWidth * 2.0f;
    float cameraHeight = cameraHalfHeight * 2.0f;
    
    // Center camera on player
    m_left = playerX - cameraHalfWidth;
    m_right = playerX + cameraHalfWidth;
    m_bottom = playerY - cameraHalfHeight;
    m_top = playerY + cameraHalfHeight;
    
    // Ensure the camera doesn't go out of bounds by clamping to map boundaries
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
}

float Camera::getLeft() const { return m_left; }
float Camera::getRight() const { return m_right; }
float Camera::getBottom() const { return m_bottom; }
float Camera::getTop() const { return m_top; }
float Camera::getWidth() const { return m_right - m_left; }
float Camera::getHeight() const { return m_top - m_bottom; }

void Camera::decreaseCameraRegionSmoothly(float amount, float timeSeconds) {
    // If already transitioning, cancel current transition and use current region as start
    float currentRegion = m_isTransitioning ? 
        (m_transitionStartRegion + (m_transitionTargetRegion - m_transitionStartRegion) * (m_transitionElapsed / m_transitionDuration)) :
        m_desiredCameraRegion;
    
    float targetRegion = std::min(std::max(currentRegion - amount, MIN_CAMERA_REGION), MAX_CAMERA_REGION);
    
    // Start smooth transition
    m_isTransitioning = true;
    m_transitionStartRegion = currentRegion;
    m_transitionTargetRegion = targetRegion;
    m_transitionDuration = timeSeconds;
    m_transitionElapsed = 0.0f;
    
    std::cout << "Starting smooth zoom in: " << currentRegion << " -> " << targetRegion 
              << " over " << timeSeconds << " seconds" << std::endl;
}

void Camera::increaseCameraRegionSmoothly(float amount, float timeSeconds) {
    // If already transitioning, cancel current transition and use current region as start
    float currentRegion = m_isTransitioning ? 
        (m_transitionStartRegion + (m_transitionTargetRegion - m_transitionStartRegion) * (m_transitionElapsed / m_transitionDuration)) :
        m_desiredCameraRegion;
    
    float targetRegion = std::min(std::max(currentRegion + amount, MIN_CAMERA_REGION), MAX_CAMERA_REGION);
    
    // Start smooth transition
    m_isTransitioning = true;
    m_transitionStartRegion = currentRegion;
    m_transitionTargetRegion = targetRegion;
    m_transitionDuration = timeSeconds;
    m_transitionElapsed = 0.0f;
    
    std::cout << "Starting smooth zoom out: " << currentRegion << " -> " << targetRegion 
              << " over " << timeSeconds << " seconds" << std::endl;
}

void Camera::setCameraRegionSmoothly(float targetRegion, float timeSeconds) {
    // If already transitioning, cancel current transition and use current region as start
    float currentRegion = m_isTransitioning ? 
        (m_transitionStartRegion + (m_transitionTargetRegion - m_transitionStartRegion) * (m_transitionElapsed / m_transitionDuration)) :
        m_desiredCameraRegion;
    
    targetRegion = std::min(std::max(targetRegion, MIN_CAMERA_REGION), MAX_CAMERA_REGION);
    
    // Start smooth transition
    m_isTransitioning = true;
    m_transitionStartRegion = currentRegion;
    m_transitionTargetRegion = targetRegion;
    m_transitionDuration = timeSeconds;
    m_transitionElapsed = 0.0f;
    
    std::cout << "Starting smooth camera transition: " << currentRegion << " -> " << targetRegion 
              << " over " << timeSeconds << " seconds" << std::endl;
}

void Camera::updateSmoothTransitions(float deltaTime) {
    if (!m_isTransitioning) {
        return;
    }
    
    m_transitionElapsed += deltaTime;
    
    if (m_transitionElapsed >= m_transitionDuration) {
        // Transition complete
        m_desiredCameraRegion = m_transitionTargetRegion;
        m_isTransitioning = false;
        
        // Apply window constraints to get the actual camera region
        float maxAllowedRegion = getMaxUsableCameraRegion(windowWidth, windowHeight);
        m_cameraRegion = std::min(m_desiredCameraRegion, maxAllowedRegion);
        
        std::cout << "Smooth camera transition completed. Final region: " << m_cameraRegion << std::endl;
    } else {
        // Interpolate between start and target using smooth easing
        float t = m_transitionElapsed / m_transitionDuration;
        
        // Apply smooth easing (ease-in-out cubic)
        t = t < 0.5f ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
        
        // Calculate current region
        float currentRegion = m_transitionStartRegion + (m_transitionTargetRegion - m_transitionStartRegion) * t;
        m_desiredCameraRegion = currentRegion;
          // Apply window constraints to get the actual camera region
        float maxAllowedRegion = getMaxUsableCameraRegion(windowWidth, windowHeight);
        m_cameraRegion = std::min(m_desiredCameraRegion, maxAllowedRegion);
    }
}

void Camera::getLastKnownPlayerPosition(float& x, float& y) const {
    x = m_lastKnownPlayerX;
    y = m_lastKnownPlayerY;
}

bool Camera::hasLastKnownPlayerPosition() const {
    return m_hasLastKnownPosition;
}
