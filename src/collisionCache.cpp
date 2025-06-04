#include "collisionCache.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CollisionBoxUtils {
    
    void calculateCollisionBox(PreCalculatedCollisionBox& cache,
                             const std::vector<std::pair<float, float>>& localPoints,
                             float x, float y, float rotation, float scale) {
        
        // Clear previous data
        cache.worldPoints.clear();
        cache.worldPoints.reserve(localPoints.size());
        
        // Convert rotation to radians
        const float angleRad = rotation * M_PI / 180.0f;
        const float cosA = cos(angleRad);
        const float sinA = sin(angleRad);
        
        // Transform each local point to world coordinates
        for (const auto& localPoint : localPoints) {
            // Apply scaling first
            float scaledX = localPoint.first * scale;
            float scaledY = localPoint.second * scale;
            
            // Apply rotation
            float rotatedX = scaledX * cosA - scaledY * sinA;
            float rotatedY = scaledX * sinA + scaledY * cosA;
            
            // Apply translation
            float worldX = x + rotatedX;
            float worldY = y + rotatedY;
            
            cache.worldPoints.push_back({worldX, worldY});
        }
        
        // Update cache parameters
        cache.cachedX = x;
        cache.cachedY = y;
        cache.cachedRotation = rotation;
        cache.cachedScale = scale;
        
        // Calculate bounding box
        cache.updateBoundingBox();
    }
    
    const PreCalculatedCollisionBox& getOrUpdateCollisionBox(PreCalculatedCollisionBox& cache,
                                                           const std::vector<std::pair<float, float>>& localPoints,
                                                           float x, float y, float rotation, float scale) {
        
        // Check if cache is still valid
        if (!cache.isCacheValid(x, y, rotation, scale)) {
            // Recalculate collision box
            calculateCollisionBox(cache, localPoints, x, y, rotation, scale);
        }
        
        return cache;
    }
}
