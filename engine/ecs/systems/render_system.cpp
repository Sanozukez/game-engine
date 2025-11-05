// // engine/ecs/systems/render_system.cpp (Implementação CORRIGIDA)

#include "render_system.h"
#include "../../ecs/world.h"
#include "../../render/shader_manager.h" // Necessário para buscar o shader de armature
#include "../components/transform_component.h"
#include "../components/animation_component.h"
#include "../components/mesh_component.h" // Necessário para Component::Mesh
#include "../../asset/model.h"            // Necessário para Engine::Asset::Model
#include "../../math/transform_utils.h"

#include <glm/glm.hpp>
#include <memory>
#include <format>
// NOTA: Os 'using namespace' foram evitados para resolver o conflito 'Mesh'

using namespace Engine::ECS;
using namespace Engine::ECS::Component;
using namespace Engine::ECS::System;
using namespace Engine::Render;
using namespace Engine::Asset;

// Implementação do Construtor
RenderSystem::RenderSystem(Engine::Render::Renderer &renderer)
    : m_renderer(renderer),
      m_assetManager(AssetManager::Get()) // Inicializa AssetManager (Singleton)
{
    // A inicialização do ArmatureRenderer é implícita (default constructor)
}

// Implementação da Lógica de Update
void RenderSystem::update(World &world, float dt)
{
    // Obtém as matrizes de View e Projection (CORREÇÃO C2039: usando getCamera())
    glm::mat4 viewMatrix = m_renderer.getCamera().getViewMatrix();
    glm::mat4 projectionMatrix = m_renderer.getCamera().getProjectionMatrix();

    // 1. Inicia o frame (chamada de baixo nível)
    m_renderer.beginScene();

    // 2. Itera sobre a lista de entidades (m_entities) com Transform e Mesh
    for (const EntityID entityID : m_entities)
    {
        // 3. Obtém os componentes necessários
        Transform &transform = world.getComponent<Transform>(entityID);

        // CORREÇÃO C2872: Usar o namespace completo, apesar do 'using namespace Component' acima.
        // O compilador ainda pode se confundir com 'Asset::Mesh'.
        Engine::ECS::Component::Mesh &mesh = world.getComponent<Engine::ECS::Component::Mesh>(entityID);

        // --- VERIFICAÇÃO DE ANIMAÇÃO (Lógica opcional) ---
        const std::vector<glm::mat4> *boneTransforms = nullptr;

        if (world.hasComponent<Animation>(entityID))
        {
            auto &anim = world.getComponent<Animation>(entityID);

            if (!anim.finalBoneTransforms.empty())
            {
                boneTransforms = &anim.finalBoneTransforms;
            }
        }
        // -----------------------------------------------------

        // 4. DESENHA A MALHA
        m_renderer.submit(mesh.assetID, transform, boneTransforms);

        // **********************************************
        // 5. DEBUG: VISUALIZAÇÃO DO ESQUELETO
        // **********************************************
       if (boneTransforms != nullptr)
        {
            // Pega o Modelo do Cache
            std::shared_ptr<Engine::Asset::Model> model = m_assetManager.getModel(mesh.assetID);

            if (model)
            {
                // 3.1. Extrai as linhas de debug
                std::vector<glm::vec3> debugLines = model->getSkeletonDebugLines(*boneTransforms);

                if (!debugLines.empty())
                {
                    // 3.2. Obtém o shader de Armature 
                    Shader& armatureShader = ShaderManager::Get().getShader(m_assetManager.getAssetIDByName("armature")); 
                    
                    // NOVO: Calcula a Model Matrix do personagem
                    glm::mat4 modelMatrix = Engine::Math::getTransformMatrix(transform); // Reutiliza a função do renderer

                    // 3.3. Desenha o esqueleto
                    // A matriz do modelo da entidade é passada para posicionar o esqueleto no mundo.
                    m_armatureRenderer.draw(armatureShader, debugLines, viewMatrix, projectionMatrix, modelMatrix); // <-- NOVO ARGUMENTO
                }
            }
        }
    }

    // 6. Finaliza o frame
    m_renderer.endScene();
}