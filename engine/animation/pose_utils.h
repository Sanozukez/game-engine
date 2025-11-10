// engine/animation/pose_utils.h


#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace Engine::Animation {

struct PoseConfig {
    static constexpr float SCALE_EPSILON = 1e-4f;
    static constexpr float MATRIX_IDENTITY_EPSILON = 1e-5f;
    static constexpr float QUAT_EPSILON = 1e-6f;
};

// Decompõe uma matriz 4x4 em componentes Translation, Rotation e Scale
void decomposeTRS(const glm::mat4& m, glm::vec3& T, glm::quat& R, glm::vec3& S);

// Compõe uma matriz 4x4 a partir de componentes Translation, Rotation e Scale
glm::mat4 composeTRS(const glm::vec3& T, const glm::quat& R, const glm::vec3& S);

// Verifica se uma matriz está próxima da identidade
bool isNearlyIdentity(const glm::mat4& M, float eps = PoseConfig::MATRIX_IDENTITY_EPSILON);

// Sanitiza valores de escala para evitar colapsos
glm::vec3 sanitizeScale(const glm::vec3& scale, float eps = PoseConfig::SCALE_EPSILON);

// Verifica se um vetor tem todos os componentes válidos
bool isValidVector3(const glm::vec3& v);

// Verifica se um quaternion é válido
bool isValidQuaternion(const glm::quat& q);

// Sanitiza um vetor3, retornando um valor padrão se inválido
glm::vec3 sanitizeVector3(const glm::vec3& v, const glm::vec3& defaultValue = glm::vec3(0.0f));

// Sanitiza um quaternion, retornando um valor padrão se inválido
glm::quat sanitizeQuaternion(const glm::quat& q, const glm::quat& defaultValue = glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

} // namespace Engine::Animation