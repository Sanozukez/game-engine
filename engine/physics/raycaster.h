// engine/physics/raycaster.h
#pragma once

#include <glm/glm.hpp>

// Forward declarations
namespace Engine {
    namespace Camera {
        class ICamera;
    }
}

namespace Engine {
namespace Physics {

class Raycaster {
public:
    Raycaster() = default;

    // Calcula um raio no espaço do mundo a partir das coordenadas da tela
    void update(double mouseX, double mouseY, int screenWidth, int screenHeight, const Engine::Camera::ICamera& camera);

    // Retorna o ponto de interseção do raio com um plano horizontal (ex: y=0)
    bool getIntersectionWithPlane(float planeHeight, glm::vec3& outIntersectionPoint) const;
    
    const glm::vec3& getRayOrigin() const { return m_rayOrigin; }
    const glm::vec3& getRayDirection() const { return m_rayDirection; }

private:
    glm::vec3 m_rayOrigin{0.0f};
    glm::vec3 m_rayDirection{0.0f, 0.0f, -1.0f};
};

} // namespace Physics
} // namespace Engine