// engine/animation/pose_utils.cpp

#include "pose_utils.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Engine::Animation {

void decomposeTRS(const glm::mat4& m, glm::vec3& T, glm::quat& R, glm::vec3& S)
{
    glm::vec3 skew;
    glm::vec4 persp;
    glm::decompose(m, S, R, T, skew, persp);
    R = glm::normalize(R);
}

glm::mat4 composeTRS(const glm::vec3& T, const glm::quat& R, const glm::vec3& S)
{
    glm::mat4 M(1.0f);
    M = glm::translate(M, T);
    M *= glm::mat4_cast(R);
    M = glm::scale(M, S);
    return M;
}

bool isNearlyIdentity(const glm::mat4& M, float eps)
{
    const glm::mat4 I(1.0f);
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (std::fabs(M[c][r] - I[c][r]) > eps) {
                return false;
            }
        }
    }
    return true;
}

bool isValidVector3(const glm::vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool isValidQuaternion(const glm::quat& q)
{
    return std::isfinite(q.x) && std::isfinite(q.y) && 
           std::isfinite(q.z) && std::isfinite(q.w) &&
           std::abs(1.0f - glm::length(q)) < PoseConfig::QUAT_EPSILON;
}

glm::vec3 sanitizeVector3(const glm::vec3& v, const glm::vec3& defaultValue)
{
    return isValidVector3(v) ? v : defaultValue;
}

glm::quat sanitizeQuaternion(const glm::quat& q, const glm::quat& defaultValue)
{
    if (!isValidQuaternion(q)) {
        return defaultValue;
    }
    return glm::normalize(q);
}

glm::vec3 sanitizeScale(const glm::vec3& scale, float eps)
{
    auto fixComponent = [eps](float v) {
        if (!std::isfinite(v)) return 1.0f;
        if (std::abs(v) < eps) return (v >= 0.f ? eps : -eps);
        return v;
    };

    return glm::vec3(
        fixComponent(scale.x),
        fixComponent(scale.y),
        fixComponent(scale.z)
    );
}

} // namespace Engine::Animation