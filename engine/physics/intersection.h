// engine/physics/intersection.h
#pragma once

#include <glm/glm.hpp>
#include <optional>

namespace Engine {
namespace Physics {

// Implementação do algoritmo de interseção Möller-Trumbore.
// Retorna a distância 't' ao longo do raio se houver uma interseção.
inline std::optional<float> rayTriangleIntersect(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDirection,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    constexpr float epsilon = 1e-8f;

    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(rayDirection, edge2);
    float a = glm::dot(edge1, h);

    if (a > -epsilon && a < epsilon) {
        return std::nullopt; // Raio paralelo ao triângulo
    }

    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f) {
        return std::nullopt;
    }

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(rayDirection, q);

    if (v < 0.0f || u + v > 1.0f) {
        return std::nullopt;
    }

    // Neste ponto, temos uma interseção. Calculamos t para saber onde.
    float t = f * glm::dot(edge2, q);

    if (t > epsilon) { // Interseção na direção do raio
        return t;
    }

    return std::nullopt;
}

} // namespace Physics
} // namespace Engine