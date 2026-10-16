#pragma once

#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "editor_overlay_utils.h"
#include "editor_viewport_math.h"

namespace Editor::ViewportInteractionUtils {

inline glm::vec3 ComputeOrbitCameraPosition(float yaw, float pitch, float distance, const glm::vec3& target) {
    return target + glm::vec3(
        std::cos(yaw) * std::cos(pitch) * distance,
        std::sin(pitch) * distance,
        std::sin(yaw) * std::cos(pitch) * distance
    );
}

inline glm::vec3 ComputeOrbitViewDirection(float yaw, float pitch, float distance, const glm::vec3& target) {
    const glm::vec3 cameraPosition = ComputeOrbitCameraPosition(yaw, pitch, distance, target);
    return glm::normalize(target - cameraPosition);
}

inline glm::vec3 ComputePanDelta(float yaw, float pitch, float distance, const glm::vec3& target, const glm::vec2& mouseDelta, float panScale) {
    const glm::vec3 viewDir = ComputeOrbitViewDirection(yaw, pitch, distance, target);
    const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, viewDir));
    const glm::vec3 cameraUp = glm::normalize(glm::cross(viewDir, cameraRight));
    return cameraRight * mouseDelta.x * panScale + cameraUp * mouseDelta.y * panScale;
}

inline void ApplyOrbitDrag(
    float& yaw,
    float& pitch,
    const glm::vec2& mouseDelta,
    float orbitScale,
    float minPitch,
    float maxPitch) {
    yaw += mouseDelta.x * orbitScale;
    pitch = ViewportMath::SafeClamp(pitch + mouseDelta.y * orbitScale, minPitch, maxPitch);
}

inline void ApplyZoomDelta(float& distance, float delta, float zoomScale, float minDistance, float maxDistance) {
    distance = ViewportMath::SafeClamp(distance - delta * zoomScale, minDistance, maxDistance);
}

inline glm::mat4 BuildSceneView(float yaw, float pitch, float distance, const glm::vec3& target) {
    const glm::vec3 cameraPosition = ComputeOrbitCameraPosition(yaw, pitch, distance, target);
    const glm::vec3 cameraForward = glm::normalize(target - cameraPosition);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(cameraForward, worldUp)) > 0.98f) {
        worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    return glm::lookAt(cameraPosition, target, worldUp);
}

inline bool RayIntersectsAABB(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDirection,
    const glm::vec3& minBounds,
    const glm::vec3& maxBounds,
    float& tHit) {
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::infinity();

    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(rayDirection[axis]) < 1e-6f) {
            if (rayOrigin[axis] < minBounds[axis] || rayOrigin[axis] > maxBounds[axis]) {
                return false;
            }
        } else {
            const float inv = 1.0f / rayDirection[axis];
            float t1 = (minBounds[axis] - rayOrigin[axis]) * inv;
            float t2 = (maxBounds[axis] - rayOrigin[axis]) * inv;

            if (t1 > t2) {
                std::swap(t1, t2);
            }

            tMin = ViewportMath::SafeMax(tMin, t1);
            tMax = ViewportMath::SafeMin(tMax, t2);

            if (tMax < tMin) {
                return false;
            }
        }
    }

    tHit = tMin;
    return true;
}

inline std::string PickObjectByRay(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDirection,
    const std::unordered_map<std::string, glm::vec3>& objectPositions,
    const std::unordered_map<std::string, glm::vec3>& objectScales) {
    std::string bestNonGroundHit = "";
    float bestNonGroundDistance = std::numeric_limits<float>::infinity();

    for (const auto& [name, position] : objectPositions) {
        if (name == "Ground") {
            continue;
        }

        constexpr float kPickPadding = 0.15f;
        const auto scaleIt = objectScales.find(name);
        const glm::vec3 visualScale = scaleIt != objectScales.end() ? scaleIt->second : glm::vec3(1.0f);
        glm::vec3 half = OverlayUtils::GetVisualHalfExtent(visualScale);
        half += glm::vec3(kPickPadding);
        const glm::vec3 minBounds = position - half;
        const glm::vec3 maxBounds = position + half;
        float distance = 0.0f;

        if (RayIntersectsAABB(rayOrigin, rayDirection, minBounds, maxBounds, distance)) {
            if (distance < bestNonGroundDistance) {
                bestNonGroundDistance = distance;
                bestNonGroundHit = name;
            }
        }
    }

    return bestNonGroundHit;
}

} // namespace Editor::ViewportInteractionUtils
