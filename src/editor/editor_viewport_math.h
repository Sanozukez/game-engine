#pragma once

#include <algorithm>

#include <glm/glm.hpp>
#include <imgui.h>

namespace Editor::ViewportMath {

inline float SafeMax(float a, float b) {
    return (a > b) ? a : b;
}

inline float SafeMin(float a, float b) {
    return (a < b) ? a : b;
}

inline float SafeClamp(float value, float lo, float hi) {
    if (hi < lo) {
        std::swap(lo, hi);
    }
    return std::clamp(value, lo, hi);
}

inline ImVec2 ProjectToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& projection, const ImVec2& viewportSize) {
    const glm::vec4 clip = projection * view * glm::vec4(worldPos, 1.0f);
    if (std::abs(clip.w) < 1e-6f) {
        return ImVec2(0.0f, 0.0f);
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    const float x = (ndc.x * 0.5f + 0.5f) * viewportSize.x;
    const float y = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y;
    return ImVec2(x, y);
}

inline float DistanceToSegmentSquared(const ImVec2& p, const ImVec2& a, const ImVec2& b) {
    const ImVec2 ab = ImVec2(b.x - a.x, b.y - a.y);
    const float denom = ab.x * ab.x + ab.y * ab.y;
    if (denom < 1e-6f) {
        const float dx = p.x - a.x;
        const float dy = p.y - a.y;
        return dx * dx + dy * dy;
    }

    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / denom, 0.0f, 1.0f);
    const ImVec2 closest = ImVec2(a.x + ab.x * t, a.y + ab.y * t);
    const float dx = p.x - closest.x;
    const float dy = p.y - closest.y;
    return dx * dx + dy * dy;
}

inline bool BuildMouseRayInImage(
    const ImVec2& mousePos,
    const ImVec2& imageMin,
    const ImVec2& imageMax,
    const glm::mat4& view,
    const glm::mat4& projection,
    glm::vec3& outRayOrigin,
    glm::vec3& outRayDirection) {
    const float imageWidth = imageMax.x - imageMin.x;
    const float imageHeight = imageMax.y - imageMin.y;
    if (imageWidth <= 1.0f || imageHeight <= 1.0f) {
        return false;
    }

    const float uvX = SafeClamp((mousePos.x - imageMin.x) / imageWidth, 0.0f, 1.0f);
    const float uvY = SafeClamp((mousePos.y - imageMin.y) / imageHeight, 0.0f, 1.0f);
    const float normX = uvX * 2.0f - 1.0f;
    const float normY = 1.0f - uvY * 2.0f;

    const glm::vec4 nearPoint = glm::vec4(normX, normY, -1.0f, 1.0f);
    const glm::vec4 farPoint = glm::vec4(normX, normY, 1.0f, 1.0f);
    const glm::mat4 invVP = glm::inverse(projection * view);
    const glm::vec4 nearWorld = invVP * nearPoint;
    const glm::vec4 farWorld = invVP * farPoint;

    const glm::vec3 nearPos = glm::vec3(nearWorld / nearWorld.w);
    const glm::vec3 farPos = glm::vec3(farWorld / farWorld.w);
    const glm::vec3 rayDir = farPos - nearPos;
    const float len = glm::length(rayDir);
    if (len < 1e-6f) {
        return false;
    }

    outRayOrigin = nearPos;
    outRayDirection = rayDir / len;
    return true;
}

inline bool IntersectRayWithPlane(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDir,
    const glm::vec3& planePoint,
    const glm::vec3& planeNormal,
    glm::vec3& outHitPoint) {
    const float denom = glm::dot(rayDir, planeNormal);
    if (std::abs(denom) < 1e-6f) {
        return false;
    }

    const float t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    if (t < 0.0f) {
        return false;
    }

    outHitPoint = rayOrigin + rayDir * t;
    return true;
}

} // namespace Editor::ViewportMath
