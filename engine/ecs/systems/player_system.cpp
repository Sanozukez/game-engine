// engine/ecs/systems/player_system.cpp

#include "player_system.h"
#include "../../core/log.h"
#include "../../ecs/world.h"
#include "../../ecs/components/movement_component.h"
#include "../../ecs/components/player_component.h"
#include "../../ecs/components/transform_component.h"
#include "../../ecs/components/camera_input_component.h"
#include "../../math/quat.h"
#include <glm/gtx/quaternion.hpp>
#include <GLFW/glfw3.h>
#include <cmath>
#include "../../camera/camera_math.h"

namespace Engine
{
    namespace ECS
    {
        namespace System
        {
            void PlayerSystem::update(World &world, float dt)
            {
                if (m_entities.empty())
                    return;

                const EntityID playerID = *m_entities.begin();
                auto &mv = world.getComponent<Component::Movement>(playerID);
                auto &tr = world.getComponent<Component::Transform>(playerID);

                const float vel = mv.movementSpeed * dt;
                const float yawSpeed = mv.rotationSpeed * dt;

                // --- ESTADO 1: CTM (desacoplado) ---
                if (mv.isMovingToDestination)
                {
                    // Se o jogador pressionar W/S/Q/E, interrompe CTM e passa a mover manualmente
                    bool w = m_inputManager.IsKeyPressed(GLFW_KEY_W);
                    bool s = m_inputManager.IsKeyPressed(GLFW_KEY_S);
                    bool q = m_inputManager.IsKeyPressed(GLFW_KEY_Q);
                    bool e = m_inputManager.IsKeyPressed(GLFW_KEY_E);

                    if (w || s || q || e)
                    {
                        // Direções baseadas na câmera (niveladas)
                        glm::vec3 camF = m_camera.getForwardVector();
                        glm::vec3 camR = m_camera.getRightVector();

                        glm::vec3 dir(0.0f);
                        if (w)
                            dir += camF;
                        if (s)
                            dir -= camF;
                        if (q)
                            dir -= camR;
                        if (e)
                            dir += camR;

                        const float len2 = glm::dot(dir, dir);
                        if (len2 > 1e-8f)
                        {
                            glm::vec3 dirN = dir * (1.0f / std::sqrt(len2));
                            tr.position += Engine::Math::Vec3(dirN * vel);
                        }

                        // Interrompe CTM e entra no estado "acoplado"
                        mv.isMovingToDestination = false;
                        mv.isCameraAttachedToPlayer = true;
                        mv.isCameraOrbitModeActive = false;

                        // Alinha o player ao yaw da câmera (como no estado acoplado)
                        auto &camInput = world.getComponent<Component::CameraInput>(playerID);
                        tr.rotation = Engine::Math::Quat(
                            glm::angleAxis(glm::radians(camInput.yaw_degrees), glm::vec3(0.0f, 1.0f, 0.0f)));

                        // sai do bloco CTM neste frame (sem executar o follow CTM abaixo)
                    }
                    else
                    {
                        // CTM normal (segue para o destino)
                        glm::vec3 to = mv.targetDestination.toGLM() - tr.position.toGLM();
                        if (glm::length(to) < 0.1f)
                        {
                            mv.isMovingToDestination = false;
                        }
                        else
                        {
                            tr.position += Engine::Math::Vec3(glm::normalize(to) * vel);

                            // personagem olha para o caminho (independente da câmera)
                            glm::vec3 horiz(to.x, 0.0f, to.z);
                            if (glm::length(horiz) > 1e-3f)
                            {
                                float targetYaw = atan2(horiz.x, horiz.z);
                                tr.rotation = Engine::Math::Quat(glm::angleAxis(targetYaw, glm::vec3(0, 1, 0)));
                            }
                        }
                    }
                }
                else
                {
                    // --- ESTADO 2: ACOPLADO (ativado por A/D/Q/E/W/S) ---

                    // Direções baseadas na câmera (forward/right já são horizontais na OrbitCamera)
                    glm::vec3 camF = m_camera.getForwardVector();
                    glm::vec3 camR = m_camera.getRightVector();

                    glm::vec3 dir(0.0f);
                    bool moving = false;

                    const float eps2 = 1e-8f;
                    float len2 = glm::dot(dir, dir);

                    if (len2 > eps2)
                    {
                        glm::vec3 dirN = dir * (1.0f / std::sqrt(len2));
                        tr.position += Engine::Math::Vec3(dirN * vel);
                        mv.isCameraAttachedToPlayer = true; // reforça acoplado enquanto move
                    }
                    else
                    {
                        // --- ESTADO 2: ACOPLADO (ativado por A/D/Q/E/W/S) ---

                        // Direções baseadas na câmera (niveladas no plano XZ)
                        glm::vec3 camF = m_camera.getForwardVector(); // orbit já é XZ, mas garantimos
                        glm::vec3 camR = m_camera.getRightVector();
                        camF.y = 0.0f;
                        camR.y = 0.0f;
                        camF = Engine::Camera::Math::safeNormalize(camF);
                        camR = Engine::Camera::Math::safeNormalize(camR);

                        glm::vec3 dir(0.0f);
                        const bool isRMB = m_inputManager.IsRightMouseButtonPressed();

                        // W/S = frente/trás
                        if (m_inputManager.IsKeyPressed(GLFW_KEY_W))
                            dir += camF;
                        if (m_inputManager.IsKeyPressed(GLFW_KEY_S))
                            dir -= camF;

                        // Q/E = strafe
                        if (m_inputManager.IsKeyPressed(GLFW_KEY_Q))
                            dir -= camR;
                        if (m_inputManager.IsKeyPressed(GLFW_KEY_E))
                            dir += camR;

                        // Com RMB, A/D viram strafe (como Q/E). Sem RMB, A/D rodam a câmera (tratado no CameraInputSystem).
                        if (isRMB)
                        {
                            if (m_inputManager.IsKeyPressed(GLFW_KEY_A))
                                dir -= camR;
                            if (m_inputManager.IsKeyPressed(GLFW_KEY_D))
                                dir += camR;
                        }

                        // Normaliza com proteção — evita NaN quando W+S / A+D / Q+E cancelam
                        glm::vec3 dirN = Engine::Camera::Math::safeNormalize(dir);
                        if (dirN.x != 0.0f || dirN.y != 0.0f || dirN.z != 0.0f)
                        {
                            tr.position += Engine::Math::Vec3(dirN * vel);
                            // enquanto há input, mantém acoplado
                            mv.isCameraAttachedToPlayer = true;
                        }

                        // Se ACOPLADO → player acompanha yaw da câmera (via componente compartilhado)
                        if (mv.isCameraAttachedToPlayer)
                        {
                            auto &camInput = world.getComponent<Component::CameraInput>(playerID);
                            tr.rotation = Engine::Math::Quat(
                                glm::angleAxis(glm::radians(camInput.yaw_degrees), glm::vec3(0.0f, 1.0f, 0.0f)));
                        }
                    }
                }

                // A câmera SEMPRE segue o target do player (inclusive CTM + RMB)
                glm::vec3 playerPos = tr.position.toGLM();
                glm::vec3 focus = playerPos + glm::vec3(0.0f, mv.cameraFocusHeight, 0.0f);
                m_camera.setTarget(focus);
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine
