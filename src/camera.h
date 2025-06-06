#pragma once

#include <algorithm> // For std::min and std::max
#include "enumDefinitions.h"


class Camera {
public:
    // Constructor
    Camera(int gridSize);    // Camera region management
    void increaseCameraRegion(float amount);
    void decreaseCameraRegion(float amount);
    void setCameraRegion(float value);
    void setCameraRegionWithWindowClamp(float value); // New method for user input with window clamping
    
    // Smooth camera region transitions
    void decreaseCameraRegionSmoothly(float amount, float timeSeconds);
    void increaseCameraRegionSmoothly(float amount, float timeSeconds);
    void setCameraRegionSmoothly(float targetRegion, float timeSeconds);
    
    // Update smooth transitions (call this each frame with deltaTime)
    void updateSmoothTransitions(float deltaTime);
    
    float getCameraRegion() const;
    float getMaxCameraRegion() const;
    float getMaxUsableCameraRegion(int windowWidth, int windowHeight) const;
    
    // Calculate camera view based on player position
    void updateCameraPosition(float playerX, float playerY, int windowWidth, int windowHeight);
    
    // Get camera bounds
    float getLeft() const;
    float getRight() const;
    float getBottom() const;
    float getTop() const;
    float getWidth() const;    float getHeight() const;
    
    // Last known player position management
    void getLastKnownPlayerPosition(float& x, float& y) const;
    bool hasLastKnownPlayerPosition() const;
    
private:// Camera properties
    float m_cameraRegion; // Current applied camera region (may be adjusted for window constraints)
    float m_desiredCameraRegion; // User's desired camera region (preserved during window resize)
    static const float MIN_CAMERA_REGION;
    static const float MAX_CAMERA_REGION;
    static const float DEFAULT_CAMERA_REGION;
    
    // Current camera boundaries
    float m_left;
    float m_right;
    float m_bottom;
    float m_top;
    
    // Last known player position
    float m_lastKnownPlayerX;
    float m_lastKnownPlayerY;
    bool m_hasLastKnownPosition;
    
    // Grid size
    int m_gridSize;
    
    // Smooth transition members
    bool m_isTransitioning;
    float m_transitionStartRegion;
    float m_transitionTargetRegion;
    float m_transitionDuration;
    float m_transitionElapsed;
};

// Global camera instance
extern Camera gameCamera;
