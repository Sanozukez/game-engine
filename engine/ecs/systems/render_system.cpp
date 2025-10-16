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

                // 2. Itera sobre a lista de entidades (m_entities) com Transform e Mesh
                for (const EntityID entityID : m_entities)
                {
                    // 3. Obtém os componentes necessários
                    Component::Transform &transform = world.getComponent<Component::Transform>(entityID);
                    Component::Mesh &mesh = world.getComponent<Component::Mesh>(entityID);

                    // --- NOVO: VERIFICAÇÃO DE ANIMAÇÃO (Lógica opcional) ---
                    // O vetor de bones é opcional (nullptr para objetos estáticos)
                    const std::vector<glm::mat4> *boneTransforms = nullptr;

                    // Verifica se a Entidade tem um Componente de Animação (OPTIONAL)
                    if (world.hasComponent<Component::Animation>(entityID))
                    {
                        // O World garante que a Entidade tem o componente.
                        auto &anim = world.getComponent<Component::Animation>(entityID);

                        // Aponta para as transformações calculadas pelo AnimationSystem
                        boneTransforms = &anim.finalBoneTransforms;
                    }
                    // -----------------------------------------------------

                    // 4. Envia os dados para o Renderer
                    // O Renderer trata se boneTransforms é nullptr (objeto estático) ou não.
                    m_renderer.submit(mesh.assetID, transform, boneTransforms);
                }

                // 5. Finaliza o frame
                m_renderer.endScene();
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine