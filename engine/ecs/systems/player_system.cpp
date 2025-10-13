// // engine/ecs/systems/player_system.cpp
#include "player_system.h"
#include "../../core/log.h"
#include "../../ecs/world.h"
#include "../../ecs/components/movement_component.h"
#include "../../ecs/components/player_component.h"
#include "../../ecs/components/transform_component.h"
#include "../../ecs/components/terrain_component.h"
#include "../../ecs/components/mesh_component.h"
#include "../../asset/asset_manager.h"
#include "../../asset/model.h"
#include "../../physics/raycaster.h"
#include "../../math/quat.h"

#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <limits>
#include <format>
#include <GLFW/glfw3.h>
#include <cmath> // For atan2

namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            void PlayerSystem::update(World &world, float dt)
            {
                // === 1. ENCONTRAR O PLAYER E COMPONENTES ===
                // NOVO: Usamos o pool m_entities do próprio sistema (DIP)
                if (m_entities.empty())
                {
                    return;
                }

                // Assumimos que o PlayerSystem só tem UMA Entity (o Player)
                const EntityID playerID = *m_entities.begin(); // Pega a primeira (e única) entidade no pool.

                // O World garante a presença dos componentes (Transform, Movement, Player)
                Component::Movement &movement = world.getComponent<Component::Movement>(playerID);
                Component::Transform &transform = world.getComponent<Component::Transform>(playerID); // AGORA CORRETO!

                // VARIÁVEIS DE CÁLCULO
                float velocity = movement.movementSpeed * dt; // Velocidade 1.0x (uniforme)
                float rotationAmountDegrees = movement.rotationSpeed * dt;

                // VARIÁVEL ÚNICA QUE DETECTA SE HÁ INTENÇÃO DE MOVIMENTO (Apenas para cancelamento)
                // bool isMoving = false; // Não é mais usada para a lógica de input, pois o vetor 'rawInputDirection' é suficiente.

                // === 3. BLOCO 1: CANCELAMENTO DE ESTADO (Prioridade de Teclado) ===
                if (m_inputManager.IsKeyPressed(GLFW_KEY_W) || m_inputManager.IsKeyPressed(GLFW_KEY_S) ||
                    m_inputManager.IsKeyPressed(GLFW_KEY_Q) || m_inputManager.IsKeyPressed(GLFW_KEY_E))
                {
                    if (movement.isMovingToDestination || movement.isCameraOrbitModeActive)
                    {
                        movement.isMovingToDestination = false;
                        movement.isCameraOrbitModeActive = false;
                    }
                }

                // // engine/ecs/systems/player_system.cpp
                // === 4. BLOCO 2: LÓGICA DE MOVIMENTO FÍSICO (Click-to-Move) ===
                if (movement.isMovingToDestination)
                {
                    // --- LÓGICA DE ROTAÇÃO DA CÂMERA (A/D) É INCLUÍDA AQUI ---
                    // Mesmo em CTM, a câmera está desacoplada e pode ser girada por A/D.
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_A))
                    {
                        m_camera.setYaw(m_camera.getYaw() + rotationAmountDegrees);
                    }
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_D))
                    {
                        m_camera.setYaw(m_camera.getYaw() - rotationAmountDegrees);
                    }
                    // --- FIM DA LÓGICA DE ROTAÇÃO DA CÂMERA ---

                    glm::vec3 directionToTarget = movement.targetDestination.toGLM() - transform.position.toGLM();
                    if (glm::length(directionToTarget) < 0.1f)
                    {
                        movement.isMovingToDestination = false;
                        // NOTA: isCameraOrbitModeActive JÁ ESTÁ TRUE pela callback LMB.
                    }
                    else
                    {
                        // Move e Rotaciona (igual ao código antigo)
                        transform.position += Engine::Math::Vec3(glm::normalize(directionToTarget) * velocity);

                        glm::vec3 horizontalDirection = glm::vec3(directionToTarget.x, 0.0f, directionToTarget.z);
                        if (glm::length(horizontalDirection) > 0.001f)
                        {
                            // NOVO: Calcula o YAW em RADIANOS a partir da direção do movimento
                            float targetYaw = atan2(horizontalDirection.x, horizontalDirection.z);
                            transform.rotation = Engine::Math::Quat(glm::angleAxis(targetYaw, glm::vec3(0.0f, 1.0f, 0.0f)));
                            // Engine::Core::Log::Info(...) // Removido Log
                        }
                    }
                }

                // [BLOCO 3: INPUT DO TECLADO - WASD/Strafe]
                else if (!movement.isMovingToDestination)
                {
                    glm::vec3 cameraForwardHorizontal = m_camera.getForwardVector();
                    glm::vec3 cameraRightHorizontal = m_camera.getRightVector();
                    bool isRMBPressed = m_inputManager.IsRightMouseButtonPressed();
                    bool hasWASInput = m_inputManager.IsKeyPressed(GLFW_KEY_W) || m_inputManager.IsKeyPressed(GLFW_KEY_S);
                    glm::vec3 rawInputDirection(0.0f);
                    bool isMoving = false;

                    // 1. Acumular o Input Bruto (Forward/Back)
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_W))
                    {
                        rawInputDirection += cameraForwardHorizontal;
                        isMoving = true;
                    }
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_S))
                    {
                        rawInputDirection -= cameraForwardHorizontal;
                        isMoving = true;
                    }

                    // 2. Acumular o Input Bruto (Strafe Q/E e A/D Condicional)
                    bool isADStrafeMode = hasWASInput && isRMBPressed;

                    if (m_inputManager.IsKeyPressed(GLFW_KEY_W))
                    {
                        rawInputDirection += cameraForwardHorizontal;
                        isMoving = true;
                    }
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_S))
                    {
                        rawInputDirection -= cameraForwardHorizontal;
                        isMoving = true;
                    }
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_Q))
                    {
                        rawInputDirection -= cameraRightHorizontal; // Strafe Esquerdo (Q)
                        isMoving = true;
                    }
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_E))
                    {
                        rawInputDirection += cameraRightHorizontal; // Strafe Direito (E)
                        isMoving = true;
                    }

                    // Processamento do A/D Condicional
                    if (isADStrafeMode)
                    {
                        // A/D funciona como STRAFE (Movimento Lateral)
                        if (m_inputManager.IsKeyPressed(GLFW_KEY_A))
                        {
                            rawInputDirection -= cameraRightHorizontal;
                            isMoving = true;
                        }
                        if (m_inputManager.IsKeyPressed(GLFW_KEY_D))
                        {
                            rawInputDirection += cameraRightHorizontal;
                            isMoving = true;
                        }
                    }

                    // 3. Normalizar e Aplicar a Velocidade
                    if (isMoving)
                    {
                        // NORMALIZAÇÃO CRÍTICA: Garante que a velocidade diagonal não seja 1.41x.
                        glm::vec3 normalizedDirection = glm::normalize(rawInputDirection);

                        // Aplica o movimento total ao transform.position (velocidade uniforme 1.0x)
                        transform.position += Engine::Math::Vec3(normalizedDirection * velocity);

                        // ROTAÇÃO E ACOPLAMENTO (Permanece igual)

                        // Rotação da Câmera (A/D)
                        if (!isADStrafeMode) // A/D SÓ GIRA a Câmera se não estiver em modo Strafe
                        {
                            if (m_inputManager.IsKeyPressed(GLFW_KEY_A))
                            {
                                m_camera.setYaw(m_camera.getYaw() + rotationAmountDegrees);
                            }
                            if (m_inputManager.IsKeyPressed(GLFW_KEY_D))
                            {
                                m_camera.setYaw(m_camera.getYaw() - rotationAmountDegrees);
                            }
                        }

                        // APLICAR ROTAÇÃO DO PERSONAGEM (Estado 1: Acoplado)
                        if (isMoving || isRMBPressed)
                        {
                            transform.rotation = Engine::Math::Quat(
                                glm::angleAxis(glm::radians(m_camera.getYaw()), glm::vec3(0.0f, 1.0f, 0.0f)));
                        }
                    }
                    else // O Personagem está parado
                    {
                        // ROTAÇÃO A/D NO ESTADO 2 (Se o movimento for zero, mas a câmera for livre)
                        if (movement.isCameraOrbitModeActive)
                        {
                            if (m_inputManager.IsKeyPressed(GLFW_KEY_A))
                            {
                                m_camera.setYaw(m_camera.getYaw() + rotationAmountDegrees);
                            }
                            if (m_inputManager.IsKeyPressed(GLFW_KEY_D))
                            {
                                m_camera.setYaw(m_camera.getYaw() - rotationAmountDegrees);
                            }
                        }
                    }
                }
                // Tracking DEVE ficar aqui caso contrário buga a camera na movimentação
                glm::vec3 playerPosGLM = transform.position.toGLM();
                Component::Movement &movementComponent = world.getComponent<Component::Movement>(playerID);

                // Calcula o ponto de foco (posição do Player + offset de altura)
                glm::vec3 cameraFocusPoint = playerPosGLM + glm::vec3(0.0f, movementComponent.cameraFocusHeight, 0.0f);

                // ATUALIZA O ALVO DA CÂMERA IMEDIATAMENTE
                m_camera.setTarget(cameraFocusPoint);
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine