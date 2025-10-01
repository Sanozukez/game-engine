#define GLFW_INCLUDE_NONE
#include "player_character.h"
#include "./../core/log.h"
#include "./../asset/model.h"
#include "./../input/input_manager.h"
#include "./../../engine/physics/intersection.h"
#include "./../../src/app/scene.h"
#include "./../../engine/render/camera/icamera.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <format>
#include <GLFW/glfw3.h>
#include <memory>
#include <limits>

namespace Engine
{
    namespace Game
    {
        PlayerCharacter::PlayerCharacter()
            : GameObject(), m_movementSpeed(5.0f), m_rotationSpeed(75.0f)
        {
            this->name = "PlayerCharacter";
        }

        PlayerCharacter::PlayerCharacter(std::unique_ptr<Engine::Asset::Model> model)
            : GameObject(std::move(model)), m_movementSpeed(5.0f), m_rotationSpeed(75.0f)
        {
            this->name = "PlayerCharacter";
        }

        void PlayerCharacter::moveTo(const glm::vec3 &destination)
        {
            m_targetDestination = destination;
            m_isMovingToDestination = true;
            m_isCameraOrbitModeActive = true;
        }

        void PlayerCharacter::update(float deltaTime, const Input::InputManager &inputManager, Scene &scene, Camera::ICamera &camera)
        {
            // --- 1. VERIFICAÇÃO DE SAÍDA DO ESTADO ESPECIAL ---
            // Input de teclado (W/S) ou RMB cancela o modo pós-clique.
            if (m_isCameraOrbitModeActive &&
                (inputManager.IsKeyPressed(GLFW_KEY_W) ||
                 inputManager.IsKeyPressed(GLFW_KEY_S) ||
                 inputManager.IsRightMouseButtonPressed()))
            {
                m_isCameraOrbitModeActive = false;
                m_isMovingToDestination = false;
            }

            // --- 2. LÓGICA DE MOVIMENTO DE CLIQUE ---
            // --- LÓGICA DE MOVIMENTO ---
            if (m_isMovingToDestination)
            {
                glm::vec3 directionToTarget = m_targetDestination - m_position;

                if (glm::length(directionToTarget) < 0.1f)
                {
                    m_isMovingToDestination = false;
                }
                else
                {
                    // O movimento continua usando o vetor de direção completo (3D)
                    m_position += glm::normalize(directionToTarget) * m_movementSpeed * deltaTime;

                    // --- CORREÇÃO DEFINITIVA DO "TOMBO" ---
                    // 1. Para a ROTAÇÃO, usamos apenas a projeção horizontal da direção.
                    glm::vec3 horizontalDirection = glm::vec3(directionToTarget.x, 0.0f, directionToTarget.z);

                    // 2. Só atualizamos a rotação se houver um movimento horizontal a ser feito.
                    if (glm::length(horizontalDirection) > 0.001f)
                    {
                        // 3. Usamos atan2 para calcular o ângulo Yaw de forma estável.
                        float targetYaw = atan2(horizontalDirection.x, horizontalDirection.z);

                        // 4. Criamos um quatérnion apenas com essa rotação em torno do eixo Y.
                        // Isso garante que o personagem NUNCA vai tombar ou se inclinar.
                        m_rotation = glm::quat(glm::vec3(0.0f, targetYaw, 0.0f));
                    }
                }
            }

            // --- 3. LÓGICA DE INPUT DO TECLADO ---
            if (m_isCameraOrbitModeActive)
            {
                // ESTADO PÓS-CLIQUE: A/D sempre giram a câmera, personagem não é afetado.
                // Esta lógica agora está separada do movimento, corrigindo o bug.
                float rotationAmountDegrees = m_rotationSpeed * deltaTime;
                if (inputManager.IsKeyPressed(GLFW_KEY_A))
                    camera.setYaw(camera.getYaw() + rotationAmountDegrees);
                if (inputManager.IsKeyPressed(GLFW_KEY_D))
                    camera.setYaw(camera.getYaw() - rotationAmountDegrees);
            }
            else
            {
                // ESTADO NORMAL: Controle total pelo teclado.
                glm::vec3 cameraForwardHorizontal = camera.getForwardVector();
                glm::vec3 cameraRightHorizontal = camera.getRightVector();
                float velocity = m_movementSpeed * deltaTime;

                if (inputManager.IsKeyPressed(GLFW_KEY_W))
                    m_position += cameraForwardHorizontal * velocity;
                if (inputManager.IsKeyPressed(GLFW_KEY_S))
                    m_position -= cameraForwardHorizontal * velocity;

                if (inputManager.IsRightMouseButtonPressed())
                {
                    if (inputManager.IsKeyPressed(GLFW_KEY_A) || inputManager.IsKeyPressed(GLFW_KEY_Q))
                        m_position -= cameraRightHorizontal * velocity;
                    if (inputManager.IsKeyPressed(GLFW_KEY_D) || inputManager.IsKeyPressed(GLFW_KEY_E))
                        m_position += cameraRightHorizontal * velocity;
                }
                else
                {
                    if (inputManager.IsKeyPressed(GLFW_KEY_Q))
                        m_position -= cameraRightHorizontal * velocity;
                    if (inputManager.IsKeyPressed(GLFW_KEY_E))
                        m_position += cameraRightHorizontal * velocity;

                    float rotationAmountDegrees = m_rotationSpeed * deltaTime;
                    if (inputManager.IsKeyPressed(GLFW_KEY_A))
                        camera.setYaw(camera.getYaw() + rotationAmountDegrees);
                    if (inputManager.IsKeyPressed(GLFW_KEY_D))
                        camera.setYaw(camera.getYaw() - rotationAmountDegrees);
                }

                setRotationEuler(0.0f, camera.getYaw(), 0.0f);
            }

            // --- LÓGICA DE AJUSTE DE ALTURA ---
            auto terrain = scene.getTerrain();
            if (terrain && terrain->getModel())
            {
                glm::vec3 rayOrigin = glm::vec3(m_position.x, 100.0f, m_position.z);
                glm::vec3 rayDirection(0.0f, -1.0f, 0.0f);

                float minDistance = std::numeric_limits<float>::max();
                bool foundIntersection = false;

                for (const auto &mesh_ptr : terrain->getModel()->getMeshes())
                {
                    // --- CORREÇÃO DAS CHAVES AQUI ---
                    // Removidas as chaves extras e desnecessárias que estavam envolvendo este bloco.
                    const auto &vertices = mesh_ptr->getVertices();
                    const auto &indices = mesh_ptr->getIndices();

                    for (size_t i = 0; i < indices.size(); i += 3)
                    {
                        const glm::vec3 &v0 = vertices[indices[i]].Position;
                        const glm::vec3 &v1 = vertices[indices[i + 1]].Position;
                        const glm::vec3 &v2 = vertices[indices[i + 2]].Position;

                        if (auto distance = Engine::Physics::rayTriangleIntersect(rayOrigin, rayDirection, v0, v1, v2))
                        {
                            if (*distance < minDistance)
                            {
                                minDistance = *distance;
                                foundIntersection = true;
                            }
                        }
                    }
                } // Fim do for (const auto& mesh_ptr : ...)

                if (foundIntersection)
                {
                    float groundHeight = rayOrigin.y - minDistance;
                    m_position.y = groundHeight;
                }
            } // --- CORREÇÃO: Fim do if (terrain && terrain->getModel()) ---

            camera.setTarget(m_position);
        } // --- Fim do void PlayerCharacter::update(...) ---

    } // namespace Game
} // namespace Engine