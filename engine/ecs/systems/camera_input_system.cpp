// engine/ecs/systems/camera_input_system.cpp

#include "camera_input_system.h"
#include "../../ecs/world.h"
#include "../../core/log.h"
#include "../../ecs/components/movement_component.h"
#include "../../ecs/components/player_component.h"
#include "../../ecs/components/camera_input_component.h"
#include "../../camera/orbit_camera.h"
#include "../../camera/free_camera.h"
#include <GLFW/glfw3.h>
#include <format>

namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            CameraInputSystem::CameraInputSystem(ECS::World &world,
                                                 Camera::ICamera &cameraRef,
                                                 Input::InputManager &inputManager,
                                                 Input::InputService &inputService)
                : m_world(world), m_camera(cameraRef), m_inputManager(inputManager), m_inputService(inputService)
            {
                // Inicializa o mouse-look 1x com a janela. Sensibilidade padrão: 0.12f
                m_mouseLook.init(m_inputService.getWindowPtr(), 0.12f);
            }

            // --- Mouse buttons ----------------------------------------------------------
            void CameraInputSystem::handleMouseButtonInput(int button, int action)
            {
                if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
                {
                    ++m_orbitSessionId;
                    Engine::Core::Log::Debug(std::format("[CIS] RMB PRESS session={} -> MouseLook.onRMBDown()", m_orbitSessionId));

                    // Ativa cursor disabled + raw mouse + centraliza + debounce do 1º frame
                    m_mouseLook.onRMBDown();
                }
                else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
                {
                    Engine::Core::Log::Debug(std::format("[CIS] RMB RELEASE session={}", m_orbitSessionId));
                    // Volta cursor normal (libera raw e desbloqueia centro)
                    m_mouseLook.onRMBUp();
                }
                else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
                {
                    Engine::Core::Log::Debug("[CIS] LMB PRESS (CTM) -> set isMovingToDestination=true, detach camera");

                    // CTM → Estado desacoplado
                    const EntityID playerID = m_world.getSingleEntityWith<Component::Player>();
                    if (playerID != INVALID_ENTITY_ID)
                    {
                        double mouseX, mouseY;
                        glfwGetCursorPos(m_inputService.getWindowPtr(), &mouseX, &mouseY);
                        int screenWidth, screenHeight;
                        glfwGetWindowSize(m_inputService.getWindowPtr(), &screenWidth, &screenHeight);

                        auto &raycaster = m_inputService.getRaycaster();
                        raycaster.update(mouseX, mouseY, screenWidth, screenHeight, m_camera);

                        glm::vec3 hit;
                        if (raycaster.getIntersectionWithPlane(0.0f, hit))
                        {
                            auto &mv = m_world.getComponent<Component::Movement>(playerID);
                            mv.targetDestination = Engine::Math::Vec3(hit);
                            mv.isMovingToDestination = true;
                            mv.isCameraOrbitModeActive = true;
                            mv.isCameraAttachedToPlayer = false;
                        }
                    }
                }
            }

            // --- Mouse move -------------------------------------------------------------
            // Com o lock-no-centro ativo (RMB pressionado), ignoramos eventos absolutos.
            // O delta relativo será lido por frame via m_mouseLook.update() no update().
            void CameraInputSystem::handleMouseMovement(double xpos, double ypos)
            {
                if (m_inputManager.IsRightMouseButtonPressed())
                    return;

                // Fora do mouse-look você pode tratar hover/pan se precisar.
                (void)xpos;
                (void)ypos;
            }

            // --- Scroll -----------------------------------------------------------------
            void CameraInputSystem::handleScrollInput(double yOffset)
            {
                m_camera.processScroll(yOffset);
            }

            // --- Teclado ----------------------------------------------------------------
            void CameraInputSystem::handleKeyInput(int key, int action)
            {
                const EntityID playerID = m_world.getSingleEntityWith<Component::Player>();
                if (playerID == INVALID_ENTITY_ID)
                    return;

                auto &mv = m_world.getComponent<Component::Movement>(playerID);

                // Pressionar A/D/Q/E/W/S → entra no estado "acoplado"
                if (action == GLFW_PRESS)
                {
                    const bool inCTM = mv.isMovingToDestination || (mv.isCameraOrbitModeActive && !mv.isCameraAttachedToPlayer);

                    if (key == GLFW_KEY_A || key == GLFW_KEY_D)
                    {
                        // Em CTM: A/D não re-acoplam; só giram a câmera (feito no update()).
                        if (!inCTM)
                        {
                            mv.isCameraAttachedToPlayer = true;
                            mv.isCameraOrbitModeActive = false;
                        }
                    }
                    else if (key == GLFW_KEY_Q || key == GLFW_KEY_E || key == GLFW_KEY_W || key == GLFW_KEY_S)
                    {
                        // Q/E/W/S continuam forçando acoplamento
                        mv.isCameraAttachedToPlayer = true;
                        mv.isCameraOrbitModeActive = false;
                    }
                }

                // Se RMB estiver pressionado, A/D farão strafe no CameraSystem → nada a fazer aqui.
            }

            // --- Update por frame -------------------------------------------------------
            void CameraInputSystem::update(ECS::World &world, float dt)
            {
                const EntityID playerID = world.getSingleEntityWith<Component::Player>();
                if (playerID == INVALID_ENTITY_ID)
                    return;

                auto &movement = world.getComponent<Component::Movement>(playerID);

                // 1) MOUSELOOK ATIVO (RMB pressionado)?
                if (m_inputManager.IsRightMouseButtonPressed())
                {
                    float dx = 0.f, dy = 0.f;
                    if (m_mouseLook.update(dx, dy))
                    {
                        // dx/dy já vêm com sensibilidade aplicada no MouseLook
                        float newYaw = m_camera.getYaw() - dx;
                        float newPitch = m_camera.getPitch() + dy;

                        // Clamp de pitch (deg) no sistema também, por segurança
                        const float pitchMin = -89.0f;
                        const float pitchMax = +89.0f;
                        if (newPitch < pitchMin)
                            newPitch = pitchMin;
                        if (newPitch > pitchMax)
                            newPitch = pitchMax;

                        m_camera.setYaw(newYaw);
                        m_camera.setPitch(newPitch);

                        // Atualiza o componente compartilhado para “colar” o player no mesmo frame
                        auto &camInput = world.getComponent<Component::CameraInput>(playerID);
                        camInput.yaw_degrees = m_camera.getYaw();
                    }

                    // Enquanto RMB está ativo, seu CameraSystem trata strafe → evitamos conflitos
                    return;
                }

                // 2) Sem RMB — rotação por teclado A/D (seu fluxo original)
                const float rotationSpeed = 75.0f * dt; // graus/segundo
                bool yawChanged = false;

                const bool inCTM = movement.isMovingToDestination || (movement.isCameraOrbitModeActive && !movement.isCameraAttachedToPlayer);

                if (m_inputManager.IsKeyPressed(GLFW_KEY_A))
                {
                    m_camera.setYaw(m_camera.getYaw() + rotationSpeed);
                    yawChanged = true;
                    if (!inCTM)
                        movement.isCameraAttachedToPlayer = true;
                }
                if (m_inputManager.IsKeyPressed(GLFW_KEY_D))
                {
                    m_camera.setYaw(m_camera.getYaw() - rotationSpeed);
                    yawChanged = true;
                    if (!inCTM)
                        movement.isCameraAttachedToPlayer = true;
                }

                if (yawChanged)
                {
                    auto &camInput = world.getComponent<Component::CameraInput>(playerID);
                    camInput.yaw_degrees = m_camera.getYaw();
                    // não alteramos pitch aqui (teclado A/D só gira no yaw)
                }
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine
