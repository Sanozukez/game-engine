// // engine/math/vec3.h

#pragma once

#include <glm/glm.hpp>
#include <cmath> // Para funções matemáticas como sqrt, sin, cos

namespace Engine {
namespace Math {

// Vec3: Um vetor simples de 3 componentes
struct Vec3 {
    float x, y, z;

    // Construtor
    constexpr Vec3(float all = 0.0f) : x(all), y(all), z(all) {}
    constexpr Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // Sobrecarga de Operadores (Mínimo Essencial)
    
    // Vec3 + Vec3
    inline Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    // Vec3 * float (Escalar)
    inline Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    // NOVO: Construtor de Conversão Explícito
    explicit Vec3(const glm::vec3& g) : x(g.x), y(g.y), z(g.z) {}

    // NOVO: OPERADORES DE ATRIBUIÇÃO COMPOSTOS (Para transform.position += ...)
    inline Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    inline Vec3& operator-=(const Vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    // NOVO MÉTODO DE CONVERSÃO
    glm::vec3 toGLM() const {
        return glm::vec3(x, y, z);
    }
    
    // NOVO MÉTODO DE CONVERSÃO (Opcional, mas útil)
    static Vec3 fromGLM(const glm::vec3& g) {
        return Vec3(g.x, g.y, g.z);
    }
    
    // Outras funções úteis (Futuro: Dot, Cross, Normalize)
    // Manteremos no mínimo essencial por enquanto para não violar o foco (SRP).
};

} // namespace Math
} // namespace Engine