// // engine/ecs/systems/render_system.cpp (Implementação CORRIGIDA)

#include "render_system.h"
#include "../../ecs/world.h"                     // O RenderSystem precisa da definição completa do World.
#include "../../ecs/components/mesh_component.h" // Para usar Component::Mesh

namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            // Implementação do Construtor
            RenderSystem::RenderSystem(Engine::Render::Renderer &renderer)
                : m_renderer(renderer)
            {
                // Lógica de inicialização do sistema aqui (se necessário)
            }

            // Implementação da Lógica de Update
            void RenderSystem::update(World &world, float dt)
            {
                // 1. Inicia o frame (chamada de baixo nível)
                m_renderer.beginScene();

                // 2. NOVO: Itera sobre a lista de entidades (m_entities) fornecida pelo World.
                // Esta lista JÁ CONTÉM SOMENTE entidades com Transform e Mesh. (DIP)
                for (const EntityID entityID : m_entities)
                {
                    // 3. Obtém os componentes necessários
                    // O World GARANTE que esses componentes existem, então usamos getComponent
                    Component::Transform &transform = world.getComponent<Component::Transform>(entityID);
                    Component::Mesh &mesh = world.getComponent<Component::Mesh>(entityID);

                    // 4. Envia os dados para o Renderer
                    m_renderer.submit(mesh.assetID, transform);
                }

                // 5. Finaliza o frame
                m_renderer.endScene();
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine