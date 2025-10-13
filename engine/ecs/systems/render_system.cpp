// // engine/ecs/systems/render_system.cpp (Implementação)

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

                // 2. Itera sobre os Componentes de Transform (Assumimos que o World retorna todos)
                for (auto const &[entityID, transform] : world.getTransformComponents())
                {

                    // --- CORREÇÃO DE API CRÍTICA ---
                    // OLD: if (world.hasMeshComponent(entityID))
                    if (world.hasComponent<Component::Mesh>(entityID))
                    { // <--- NOVO TEMPLATE HAS COMPONENT

                        // OLD: Component::Mesh& mesh = world.getMeshComponent(entityID);
                        Component::Mesh &mesh = world.getComponent<Component::Mesh>(entityID); // <--- NOVO TEMPLATE GET COMPONENT

                        // 4. Envia os dados para o Renderer
                        m_renderer.submit(mesh.assetID, transform);
                    }
                }

                // 5. Finaliza o frame
                m_renderer.endScene();
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine