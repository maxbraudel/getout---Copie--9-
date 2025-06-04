#pragma once

#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>

// Pre-calculated collision box structure for performance optimization
struct PreCalculatedCollisionBox {
    std::vector<std::pair<float, float>> worldPoints;  // Transformed collision points in world coordinates
    float boundingBoxMinX = 0.0f, boundingBoxMaxX = 0.0f;  // Axis-aligned bounding box for fast rejection
    float boundingBoxMinY = 0.0f, boundingBoxMaxY = 0.0f;
    bool isValid = false;  // Whether the cached data is valid
    
    // Parameters used to generate this cache (for invalidation detection)
    float cachedX = 0.0f, cachedY = 0.0f;
    float cachedRotation = 0.0f, cachedScale = 1.0f;
    
    // Calculate bounding box from world points
    void updateBoundingBox() {
        if (worldPoints.empty()) {
            isValid = false;
            return;
        }
        
        boundingBoxMinX = boundingBoxMaxX = worldPoints[0].first;
        boundingBoxMinY = boundingBoxMaxY = worldPoints[0].second;
        
        for (const auto& point : worldPoints) {
            boundingBoxMinX = std::min(boundingBoxMinX, point.first);
            boundingBoxMaxX = std::max(boundingBoxMaxX, point.first);
            boundingBoxMinY = std::min(boundingBoxMinY, point.second);
            boundingBoxMaxY = std::max(boundingBoxMaxY, point.second);
        }
        isValid = true;
    }
    
    // Check if cache is valid for given parameters
    bool isCacheValid(float x, float y, float rotation, float scale) const {
        const float epsilon = 0.001f;
        return isValid && 
               std::abs(cachedX - x) < epsilon &&
               std::abs(cachedY - y) < epsilon &&
               std::abs(cachedRotation - rotation) < epsilon &&
               std::abs(cachedScale - scale) < epsilon;
    }
    
    // Fast bounding box intersection test
    bool boundingBoxIntersects(const PreCalculatedCollisionBox& other) const {
        if (!isValid || !other.isValid) return false;
        
        return !(boundingBoxMaxX < other.boundingBoxMinX ||
                boundingBoxMinX > other.boundingBoxMaxX ||
                boundingBoxMaxY < other.boundingBoxMinY ||
                boundingBoxMinY > other.boundingBoxMaxY);
    }
    
    // Clear the cache
    void invalidate() {
        isValid = false;
        worldPoints.clear();
    }
};

// Utility functions for collision box calculation
namespace CollisionBoxUtils {
    // Calculate collision box from local points and transformation parameters
    void calculateCollisionBox(PreCalculatedCollisionBox& cache,
                             const std::vector<std::pair<float, float>>& localPoints,
                             float x, float y, float rotation, float scale);
    
    // Get or update cached collision box for an entity/element
    const PreCalculatedCollisionBox& getOrUpdateCollisionBox(PreCalculatedCollisionBox& cache,
                                                           const std::vector<std::pair<float, float>>& localPoints,
                                                           float x, float y, float rotation, float scale);
}
