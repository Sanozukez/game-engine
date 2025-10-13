// // engine/ecs/systems/camera_system.cpp

#include "camera_system.h"
#include "../../core/config_manager.h"
#include "../../core/log.h"
#include "../../math/vec3.h"
#include "../../camera/free_camera.h"
#include "../../camera/orbit_camera.h"
#include "../../input/input_manager.h" // Garante InputManager
#include "../../ecs/world.h"           // Garante World

#include <GLFW/glfw3.h>
#include <glm/gtx/string_cast.hpp> // Para o Log

namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            // ------------------------------------------------------------------------
            // CONSTRUTOR: Setup de Inicialização e JSON (LSP)
            // ------------------------------------------------------------------------
            CameraSystem::CameraSystem(Engine::Camera::ICamera &camera)
                : m_camera(camera),
                  m_inputManager(Engine::Input::InputManager::Get())
            {
                Engine::Log::Info("CameraSystem: Inicializado. Aplicando setup JSON.");

                auto &config = Engine::ConfigManager::Get();

                // --- LÓGICA DE SETUP MIGRADA DO SCENE::INITIALIZE ---
                if (auto *orbitCam = dynamic_cast<Engine::Camera::OrbitCamera *>(&m_camera))
                {
                    glm::vec3 initialTarget(0.0f, 1.0f, 0.0f);
                    orbitCam->setTarget(initialTarget);

                    orbitCam->setDistanceLimits(
                        config.getValue<float>("camera.orbit.min_distance", 2.0f),
                        config.getValue<float>("camera.orbit.max_distance", 25.0f));
                    orbitCam->setPitchLimits(
                        config.getValue<float>("camera.orbit.min_pitch_degrees", -20.0f),
                        config.getValue<float>("camera.orbit.max_pitch_degrees", 85.0f));
                }
                else if (auto *freeCam = dynamic_cast<Engine::Camera::FreeCamera *>(&m_camera))
                {
                    freeCam->setPosition(glm::vec3(25.0f, 15.0f, 25.0f));
                }

                // NENHUM CALLBACK DEVE FICAR AQUI! (Eles estão no App::run)
            }

            // ------------------------------------------------------------------------
            // UPDATE: Lógica Contínua (WASD, Rotação A/D)
            // ------------------------------------------------------------------------

            void CameraSystem::update(World &world, float dt)
            {
                // DEFINIÇÃO DA VELOCIDADE ANGULAR (Suficiente, a velocidade linear é para o PlayerSystem)
                float rotationAmountDegrees = 75.0f * dt;

                // 2. LÓGICA DE ROTAÇÃO E STRAFE
                if (m_inputManager.IsRightMouseButtonPressed())
                {
                    // ESTADO 1: MOUSE ROTAÇÃO (APENAS STRAFE LATERAL)
                    // O MouseMoved callback está cuidando da rotação Pitch/Yaw. Aqui, tratamos APENAS o movimento lateral.

                    // Side-Strafe (Movimento Lateral) - Usa A/D ou Q/E como strafe
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_A) || m_inputManager.IsKeyPressed(GLFW_KEY_Q))
                        m_camera.processKeyboard(Camera::CameraMovement::LEFT, dt);

                    if (m_inputManager.IsKeyPressed(GLFW_KEY_D) || m_inputManager.IsKeyPressed(GLFW_KEY_E))
                        m_camera.processKeyboard(Camera::CameraMovement::RIGHT, dt);
                }
                else // Botão Direito Solto (ESTADO 2: CONTROLE DE TECLADO PURO)
                {
                    // // CORREÇÃO: RESTAURAÇÃO DO GIRO A/D (Ação principal da ICamera)
                    // if (m_inputManager.IsKeyPressed(GLFW_KEY_A))
                    //     m_camera.setYaw(m_camera.getYaw() + rotationAmountDegrees);

                    // if (m_inputManager.IsKeyPressed(GLFW_KEY_D))
                    //     m_camera.setYaw(m_camera.getYaw() - rotationAmountDegrees);

                    // Side-Strafe (Q/E) - Tratamento secundário
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_Q))
                        m_camera.processKeyboard(Camera::CameraMovement::LEFT, dt);

                    if (m_inputManager.IsKeyPressed(GLFW_KEY_E))
                        m_camera.processKeyboard(Camera::CameraMovement::RIGHT, dt);
                }

                const EntityID playerID = world.getSingleEntityWith<Component::Player>();

                // if (playerID != INVALID_ENTITY_ID)
                // {
                //     // A câmera deve ler a posição FINAL do Player (que já foi atualizada)
                //     Component::Transform &transform = world.getTransform(playerID);
                //     Component::Movement &movementComponent = world.getComponent<Component::Movement>(playerID);

                //     glm::vec3 playerPosGLM = transform.position.toGLM();

                //     // Calcula o ponto de foco (posição do Player + offset de altura)
                //     glm::vec3 cameraFocusPoint = playerPosGLM + glm::vec3(0.0f, movementComponent.cameraFocusHeight, 0.0f);

                //     // ATUALIZA O ALVO DA CÂMERA
                //     m_camera.setTarget(cameraFocusPoint);
                // }
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine