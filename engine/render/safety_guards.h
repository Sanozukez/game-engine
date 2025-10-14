// // engine/render/safety_guards.h

/**
 * @brief Guard para prevenir tela preta ao atingir NaN na parte de render.
 * *
 */

#pragma once

#include <glm/glm.hpp>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp> // lookAt
#include <glm/gtx/norm.hpp>             // length2

namespace Engine::Render::Safety
{

    inline glm::mat4 safePerspective(float fovDeg, float aspect, float nearZ, float farZ)
    {
        if (!std::isfinite(fovDeg))
            fovDeg = 60.0f;
        if (!std::isfinite(aspect) || aspect <= 0.0001f)
            aspect = 16.0f / 9.0f;
        fovDeg = glm::clamp(fovDeg, 20.0f, 100.0f);
        nearZ = std::max(nearZ, 0.0001f);
        farZ = std::max(farZ, nearZ + 1.0f);
        return glm::perspective(glm::radians(fovDeg), aspect, nearZ, farZ);
    }

    inline bool finiteMat4(const glm::mat4 &M)
    {
        const float *p = reinterpret_cast<const float *>(&M);
        for (int i = 0; i < 16; ++i)
            if (!std::isfinite(p[i]))
                return false;
        return true;
    }

    inline bool finiteVec3(const glm::vec3 &v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    inline glm::mat4 safeLookAt(const glm::vec3 &eye,
                                const glm::vec3 &center,
                                const glm::vec3 &up)
    {
        glm::vec3 e = finiteVec3(eye) ? eye : glm::vec3(0, 3, 6);
        glm::vec3 c = finiteVec3(center) ? center : glm::vec3(0, 0, 0);
        glm::vec3 u = finiteVec3(up) ? up : glm::vec3(0, 1, 0);
        if (glm::length2(e - c) < 1e-8f)
            e += glm::vec3(0, 0, 0.001f);
        return glm::lookAt(e, c, u);
    }

} // namespace Engine::Render::Safety
