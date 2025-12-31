// // engine/ecs/systems/camera_system.cpp

#include "camera_system.h"
#include "core/config_manager.h"
#include "core/log.h"
#include "camera/free_camera.h"
#include "camera/orbit_camera.h"
#include "input/input_manager.h"
#include "ecs/world.h"
#include <GLFW/glfw3.h>

namespace Engine
{
    namespace ECS
    {
        namespace System
        {
            CameraSystem::CameraSystem(Engine::Camera::ICamera &camera)
                : m_camera(camera),
                  m_inputManager(Engine::Input::InputManager::Get())
            {
                auto &config = Engine::Core::ConfigManager::Get();

                if (auto *orbitCam = dynamic_cast<Engine::Camera::OrbitCamera *>(&m_camera))
                {
                    orbitCam->setTarget(glm::vec3(0.0f, 1.0f, 0.0f));
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
            }

            void CameraSystem::update(World &world, float dt)
            {
                const float rotationAmountDegrees = 75.0f * dt;

                // RMB pressionado => modo "controle livre": A/D e Q/E = STRAFE lateral
                if (m_inputManager.IsRightMouseButtonPressed())
                {
                    if (m_inputManager.IsKeyPressed(GLFW_KEY_A) || m_inputManager.IsKeyPressed(GLFW_KEY_Q))
                        m_camera.processKeyboard(Camera::CameraMovement::LEFT, dt);

                    if (m_inputManager.IsKeyPressed(GLFW_KEY_D) || m_inputManager.IsKeyPressed(GLFW_KEY_E))
                        m_camera.processKeyboard(Camera::CameraMovement::RIGHT, dt);

                    return; // não aplicar yaw nesses frames
                }

                // RMB solto => A/D giram a câmera (o Player acompanha se estiver acoplado)
                if (m_inputManager.IsKeyPressed(GLFW_KEY_A))
                    m_camera.setYaw(m_camera.getYaw() + rotationAmountDegrees);
                if (m_inputManager.IsKeyPressed(GLFW_KEY_D))
                    m_camera.setYaw(m_camera.getYaw() - rotationAmountDegrees);

                // (opcional) Q/E strafe também sem RMB
                if (m_inputManager.IsKeyPressed(GLFW_KEY_Q))
                    m_camera.processKeyboard(Camera::CameraMovement::LEFT, dt);
                if (m_inputManager.IsKeyPressed(GLFW_KEY_E))
                    m_camera.processKeyboard(Camera::CameraMovement::RIGHT, dt);
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine
