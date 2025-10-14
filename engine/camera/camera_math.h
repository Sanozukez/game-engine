// engine/camera/camera_math.h
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace Engine::Camera::Math {

inline float wrapDegrees(float d) {
    // normaliza para [-180, 180]
    while (d > 180.0f) d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    return d;
}

inline float clampPitchDeg(float p, float minDeg = -89.0f, float maxDeg = +89.0f) {
    return glm::clamp(p, minDeg, maxDeg);
}

inline bool validVec3(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

inline bool nearlyZero(float v, float eps = 1e-6f) { return std::abs(v) < eps; }

inline bool isFinite(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
inline float length2(const glm::vec3& v) { return glm::dot(v,v); }

// Normaliza sem gerar NaN para vetor zero
inline glm::vec3 safeNormalize(const glm::vec3& v, float eps = 1e-6f) {
    const float l2 = length2(v);
    if (l2 <= eps || !isFinite(v)) return glm::vec3(0.0f);
    return v * (1.0f / std::sqrt(l2));
}

} // namespace Engine::Camera::Math
