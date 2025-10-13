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
                // NOVO: Iteração sobre o pool m_entities
                if (m_entities.empty()) {
                    return; // Não há alvo, não faz nada.
                }
                
                // Assumimos que o primeiro (e único) elemento é o alvo da câmera (o Player)
                const EntityID entityID = *m_entities.begin();
                
                // Obtém os Componentes necessários (garantidos pela Signature)
                Component::Transform &transform = world.getComponent<Component::Transform>(entityID);
                const Component::CameraTarget &target = world.getComponent<Component::CameraTarget>(entityID);
                
                glm::vec3 playerPosGLM = transform.position.toGLM();
                glm::vec3 cameraFocusPoint = playerPosGLM + glm::vec3(0.0f, target.focusHeight, 0.0f);

                // --- CORREÇÃO DE FRAME 0 (CRÍTICO para a inicialização) ---
                if (m_camera.getTarget() == glm::vec3(0.0f) && glm::length(playerPosGLM) > 1.0f)
                {
                    // Lógica de inicialização (yaw, target, reset mouse state)
                    glm::quat playerQuat = transform.rotation;
                    float playerYawDegrees = glm::degrees(glm::yaw(playerQuat));

                    m_camera.setYaw(playerYawDegrees);
                    m_camera.setTarget(cameraFocusPoint);
                    m_camera.resetMouseState();
                }

                // APLICA O TRACKING NORMAL
                m_camera.setTarget(cameraFocusPoint);
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine