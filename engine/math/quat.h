// // engine/math/quat.h

#pragma once

#include "vec3.h" // Necessário para o construtor de Euler
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // Incluir extensões glm

namespace Engine
{
    namespace Math
    {

        struct Quat : public glm::quat
        {

            // Construtor Default
            Quat() : glm::quat(1.0f, 0.0f, 0.0f, 0.0f) {}

            // Construtor de Cópia (Para que o C++ possa mover/copiar)
            using glm::quat::quat;

            // NOVO: Construtor de CONVERSÃO de glm::quat (resolve o C2440)
            Quat(const glm::quat &q) : glm::quat(q) {}

            // Construtor de Vec3 para Euler (Roll, Pitch, Yaw)
            Quat(const Vec3 &euler)
                : glm::quat(glm::vec3(glm::radians(euler.x), glm::radians(euler.y), glm::radians(euler.z))) {}
        };

    } // namespace Math
} // namespace Engine