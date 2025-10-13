// engine/ECS/systems/camera_targe_system.cpp

#include "./../core/log.h"
#include "camera_target_system.h"
#include "../../ecs/world.h"
#include "../../ecs/components/transform_component.h"
#include <glm/gtx/string_cast.hpp>

namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            void CameraTargetSystem::update(World &world, float dt)
            {
                for (auto const &pair : world.getComponents<Component::CameraTarget>())
                {
                    const EntityID entityID = pair.first;
                    const Component::CameraTarget &target = pair.second;

                    if (world.hasComponent<Component::Transform>(entityID))
                    {
                        Component::Transform &transform = world.getTransform(entityID);

                        glm::vec3 playerPosGLM = transform.position.toGLM();
                        glm::vec3 cameraFocusPoint = playerPosGLM + glm::vec3(0.0f, target.focusHeight, 0.0f);

                        // --- CORREÇÃO DE FRAME 0 (Frame-resilient initialization) ---
                        // Se a câmera ainda estiver no target padrão (0,0,0) E o player estiver em um spawn point válido,
                        // forçamos o Yaw e o Target para resolver o problema de inicialização.
                        if (m_camera.getTarget() == glm::vec3(0.0f) && glm::length(playerPosGLM) > 1.0f)
                        {
                            // Calcula o Yaw do Player
                            glm::quat playerQuat = transform.rotation;
                            float playerYawDegrees = glm::degrees(glm::yaw(playerQuat));

                            // Aplica o Yaw inicial e Target
                            m_camera.setYaw(playerYawDegrees);
                            m_camera.setTarget(cameraFocusPoint);
                            m_camera.resetMouseState();
                        }

                        // APLICA O TRACKING NORMAL (Mesmo que o fix de Frame 0 tenha rodado)
                        m_camera.setTarget(cameraFocusPoint);
                    }
                }
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine