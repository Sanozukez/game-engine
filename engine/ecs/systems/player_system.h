// // engine/ecs/systems/player_system.h

#pragma once

#include "base_system.h"
#include "../../camera/icamera.h" // Necessário para a dependência
#include "../../input/input_manager.h"
#include "../../core/config_manager.h" 
#include <optional>
#include <glm/glm.hpp> // Necessário para a assinatura da função rayTriangleIntersect

// Forward declarations dos Componentes necessários
namespace Engine::ECS::Component
{
    struct Movement;
    struct Player;
    struct Transform;
    struct AnimationComponent;
    struct Mesh;
}
// Forward declarations de outras dependências
namespace Engine::Asset
{
    class Model;
    class AssetManager;
}
namespace Engine::ECS::System
{
    class AnimationSystem;
}
namespace Engine::Physics
{
    // Assumimos que as funções de Raycasting/Interseção estão disponíveis aqui.
    std::optional<float> rayTriangleIntersect(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2);
}

namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            class PlayerSystem : public BaseSystem
            {
            private:
                // O sistema precisa de acesso direto ao InputManager
                Engine::Input::InputManager &m_inputManager;
                
                // REINTRODUZIDO: Referência à Câmera principal (para Forward/Right e setYaw)
                Engine::Camera::ICamera &m_camera;
                
                // NOVO v101: Ponteiro para AnimationSystem (resolvido em runtime)
                AnimationSystem* m_animationSystem; // Ponteiro, será resolvido no update

            public:
                // O construtor injeta a Câmera como dependência.
                PlayerSystem(Engine::Camera::ICamera &camera) 
                    : m_inputManager(Engine::Input::InputManager::Get()),
                      m_camera(camera), // Inicializa a referência da câmera
                      m_animationSystem(nullptr)
                {
                }
                
                // O update executa a lógica do Player Character.
                void update(World &world, float dt) override;
            };

        } // namespace System
    } // namespace ECS
} // namespace Engine