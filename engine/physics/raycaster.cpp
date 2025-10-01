// engine/physics/raycaster.cpp
#define GLFW_INCLUDE_NONE // Evitar reinclusão de headers do OpenGL
#include "raycaster.h"
#include "./../../engine/render/camera/icamera.h"
#include <glm/gtc/matrix_inverse.hpp>

namespace Engine {
namespace Physics {

void Raycaster::update(double mouseX, double mouseY, int screenWidth, int screenHeight, const Engine::Camera::ICamera& camera) {
    // 1. Coordenadas Normalizadas do Dispositivo (NDC) [-1, 1]
    float x = (2.0f * static_cast<float>(mouseX)) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * static_cast<float>(mouseY)) / screenHeight; // Y é invertido

    // 2. Coordenadas de Recorte (Clip Space) [-1, 1]
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f); // -1.0f em Z para apontar "para dentro"

    // 3. Coordenadas da Câmera (Eye Space)
    glm::mat4 projectionMatrix = camera.getProjectionMatrix(); // Supondo que a ICamera tenha este método
    glm::mat4 viewMatrix = camera.getViewMatrix();
    
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f); // Apontar para frente, W=0 indica uma direção

    // 4. Coordenadas do Mundo (World Space)
    glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix) * rayEye);
    
    m_rayDirection = glm::normalize(rayWorld);
    m_rayOrigin = camera.getPosition();
}

bool Raycaster::getIntersectionWithPlane(float planeHeight, glm::vec3& outIntersectionPoint) const {
    glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);
    glm::vec3 planeOrigin(0.0f, planeHeight, 0.0f);

    float denominator = glm::dot(m_rayDirection, planeNormal);

    // Se o raio for paralelo ao plano, não há interseção
    if (glm::abs(denominator) > 1e-6) {
        float t = glm::dot(planeOrigin - m_rayOrigin, planeNormal) / denominator;
        if (t >= 0) { // Garante que a interseção está na frente da câmera
            outIntersectionPoint = m_rayOrigin + t * m_rayDirection;
            return true;
        }
    }
    return false;
}

} // namespace Physics
} // namespace Engine