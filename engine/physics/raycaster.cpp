// engine/physics/raycaster.cpp
#define GLFW_INCLUDE_NONE // Evitar reinclusão de headers do OpenGL
#include "raycaster.h"
#include "./../../client/camera/icamera.h"
#include <glm/gtc/matrix_inverse.hpp>
#include "../../client/camera/camera_math.h"
#include <glm/gtx/norm.hpp>

namespace Engine
{
    namespace Physics
    {

        // Implementação da função de interseção de raios(para uso no ajuste de altura)
        std::optional<float> rayTriangleIntersect(
            const glm::vec3 &rayOrigin,
            const glm::vec3 &rayDirection,
            const glm::vec3 &v0,
            const glm::vec3 &v1,
            const glm::vec3 &v2)
        {
            // CÓDIGO DE INTERSEÇÃO DE RAIO/TRIÂNGULO
            // Este é um algoritmo complexo (Möller–Trumbore) que estava em seu código original.
            // Vamos usar a lógica essencial:

            const float EPSILON = 0.0000001f;
            glm::vec3 edge1, edge2, h, s, q;
            float a, f, u, v;

            edge1 = v1 - v0;
            edge2 = v2 - v0;

            h = glm::cross(rayDirection, edge2);
            a = glm::dot(edge1, h);

            if (a > -EPSILON && a < EPSILON)
                return std::nullopt; // Raio é paralelo ao triângulo

            f = 1.0f / a;
            s = rayOrigin - v0;
            u = f * glm::dot(s, h);

            if (u < 0.0f || u > 1.0f)
                return std::nullopt;

            q = glm::cross(s, edge1);
            v = f * glm::dot(rayDirection, q);

            if (v < 0.0f || u + v > 1.0f)
                return std::nullopt;

            // Calcula t para saber a distância
            float t = f * glm::dot(edge2, q);

            if (t > EPSILON)
            {
                return t; // Retorna a distância
            }
            else
            {
                return std::nullopt; // Linha não intersecta o triângulo (ou está atrás)
            }
        }

        void Raycaster::update(double mouseX, double mouseY, int screenWidth, int screenHeight, const Engine::Camera::ICamera &camera)
        {
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

            m_rayDirection = Engine::Camera::Math::safeNormalize(rayWorld);
            if (glm::length2(m_rayDirection) < 1e-12f)
            {
                // fallback estável: use o forward atual da câmera
                m_rayDirection = Engine::Camera::Math::safeNormalize(camera.getForwardVector());
            }
            m_rayOrigin = camera.getPosition();
        }

        bool Raycaster::getIntersectionWithPlane(float planeHeight, glm::vec3 &outIntersectionPoint) const
        {
            glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);
            glm::vec3 planeOrigin(0.0f, planeHeight, 0.0f);

            float denominator = glm::dot(m_rayDirection, planeNormal);

            // Se o raio for paralelo ao plano, não há interseção
            if (glm::abs(denominator) > 1e-6)
            {
                float t = glm::dot(planeOrigin - m_rayOrigin, planeNormal) / denominator;
                if (t >= 0)
                { // Garante que a interseção está na frente da câmera
                    outIntersectionPoint = m_rayOrigin + t * m_rayDirection;
                    return true;
                }
            }
            return false;
        }

    } // namespace Physics
} // namespace Engine