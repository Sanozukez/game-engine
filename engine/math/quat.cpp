// // engine/math/quat.cpp
#include "quat.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace Engine::Math;

// Implementação do toMat4()
glm::mat4 Quat::toMat4() const
{
    // Assume que a struct Quat armazena os valores no formato glm::quat.
    // Usamos o construtor explícito da sua struct.
    glm::quat gQuat(w, x, y, z); 
    return glm::mat4_cast(gQuat);
}

// Implementação do slerp()
Quat Quat::slerp(const Quat& q1, const Quat& q2, float t)
{
    glm::quat gq1(q1.w, q1.x, q1.y, q1.z);
    glm::quat gq2(q2.w, q2.x, q2.y, q2.z);

    glm::quat result = glm::slerp(gq1, gq2, t);

    // Converte de volta para a sua struct Quat
    return Quat(result.x, result.y, result.z, result.w); 
}