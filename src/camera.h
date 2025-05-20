#pragma once

#include <algorithm> // For std::min and std::max

class Camera {
public:
    // Constructor
    Camera(int gridSize);
    
    // Camera region management
    void increaseCameraRegion(float amount);
    void decreaseCameraRegion(float amount);
    void setCameraRegion(float value);
    float getCameraRegion() const;
    
    // Calculate camera view based on player position
    void updateCameraPosition(float playerX, float playerY, int windowWidth, int windowHeight);
    
    // Get camera bounds
    float getLeft() const;
    float getRight() const;
    float getBottom() const;
    float getTop() const;
    float getWidth() const;
    float getHeight() const;
    
private:    // Camera properties
    float m_cameraRegion; // Size of the visible region around the player (in grid units)
    static const float MIN_CAMERA_REGION;
    static const float MAX_CAMERA_REGION;
    static const float DEFAULT_CAMERA_REGION;
    
    // Current camera boundaries
    float m_left;
    float m_right;
    float m_bottom;
    float m_top;
    
    // Grid size
    int m_gridSize;
};

// Global camera instance
extern Camera gameCamera;
