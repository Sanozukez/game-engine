// engine/ecs/systems/render_system.cpp
//
// Implementação CORRIGIDA, usando o Model::Skeleton para extrair as matrizes.

#include "render_system.h"
#include "../../ecs/world.h"
#include "../../render/shader_manager.h" 
#include "../components/transform_component.h"
#include "../components/animation_component.h" // <-- Inclui AnimationComponent
#include "../components/mesh_component.h" 
#include "../../asset/model.h"
#include "../../math/transform_utils.h"
#include "../../asset/skeleton.h" 
#include "../../render/armature_renderer.h" // (Necessário para m_armatureRenderer)

#include <glm/glm.hpp>
#include <memory>
#include <format>
#include <vector>

using namespace Engine::ECS;
using namespace Engine::ECS::System;
using namespace Engine::Render;
using namespace Engine::Asset;

// Implementação do Construtor
RenderSystem::RenderSystem(Engine::Render::Renderer &renderer)
    : m_renderer(renderer),
      m_assetManager(AssetManager::Get())
{
    // A inicialização do ArmatureRenderer é implícita
}

// Implementação da Lógica de Update
void RenderSystem::update(World &world, float dt)
{
    glm::mat4 viewMatrix = m_renderer.getCamera().getViewMatrix();
    glm::mat4 projectionMatrix = m_renderer.getCamera().getProjectionMatrix();

    m_renderer.beginScene();

    for (const EntityID entityID : m_entities)
    {
        Engine::ECS::Component::Transform &transform = world.getComponent<Engine::ECS::Component::Transform>(entityID);
        Engine::ECS::Component::Mesh &mesh = world.getComponent<Engine::ECS::Component::Mesh>(entityID);
        
        std::shared_ptr<Engine::Asset::Model> model = m_assetManager.getModel(mesh.assetID);

        // (Log [DEBUG_PTR] do RenderSystem, corrigido para C7595)
        // Engine::Core::Log::Error(std::format("[DEBUG_PTR] RenderSystem: Entidade {} usa Model@0x{:X}",
        //                                      static_cast<uint32_t>(entityID),
        //                                      reinterpret_cast<uintptr_t>(model.get())));

        // --- PREPARAÇÃO DE ANIMAÇÃO (A CORREÇÃO DE ARQUITETURA e C2039) ---
        const std::vector<glm::mat4> *boneTransformsPtr = nullptr;

        // A "Fonte da Verdade" é o Componente, não o Model.
        // --- CORREÇÃO: Usa o nome completo 'AnimationComponent' ---
        if (world.hasComponent<Engine::ECS::Component::AnimationComponent>(entityID)) 
        {
            // Se a entidade tem o componente, nós lemos dele.
            // --- CORREÇÃO: Usa o nome completo 'AnimationComponent' ---
            Engine::ECS::Component::AnimationComponent &animComp = world.getComponent<Engine::ECS::Component::AnimationComponent>(entityID);

            if (!animComp.finalBoneTransforms.empty())
            {
                // Aponta o ponteiro diretamente para os dados no componente.
                boneTransformsPtr = &animComp.finalBoneTransforms;
            }
            else
            {
                // O AnimSystem correu mas não preencheu o vetor
                Engine::Core::Log::Warn(std::format("[DEBUG_RENDER] Entidade {} tem AnimComp, mas 'finalBoneTransforms' está VAZIO.", static_cast<uint32_t>(entityID)));
            }
        }
        // --- FIM DA CORREÇÃO ---

        // 4. DESENHA A MALHA
        // (O submit recebe o ponteiro, que é nulo para estáticos ou preenchido para animados)
        m_renderer.submit(mesh.assetID, transform, boneTransformsPtr);
        
        // 5. DEBUG: VISUALIZAÇÃO DO ESQUELETO
        
        // A lógica de 'getSkeletonDebugLines' (lida do Model) ainda é necessária 
        // para as linhas verdes.
        // Verificamos 'boneTransformsPtr' (para saber se a animação correu)
        // e 'hasSkeleton' (para saber se o Model pode gerar linhas).
        if (boneTransformsPtr != nullptr && model && model->hasSkeleton())
        {
            // Engine::Core::Log::Info("[RENDER_DEBUG] Tentando gerar linhas de debug do esqueleto."); 
            std::vector<glm::vec3> debugLines = model->getSkeletonDebugLines(); 

            if (!debugLines.empty())
            {
                // (Corrigido o log para mostrar o número de LINHAS, não de vértices)
                // Engine::Core::Log::Info(std::format("[RENDER_DEBUG] Geradas {} linhas de debug. Desenhando.", debugLines.size() / 2));
                Shader &armatureShader = ShaderManager::Get().getShader(m_assetManager.getAssetIDByName("armature"));
                glm::mat4 modelMatrix = Engine::Math::getTransformMatrix(transform);
                
                m_armatureRenderer.draw(armatureShader, debugLines, viewMatrix, projectionMatrix, modelMatrix);
            }
            else
            {
                 Engine::Core::Log::Warn("[RENDER_DEBUG] getSkeletonDebugLines retornou vetor vazio.");
            }
        }
        // --- CORREÇÃO: Usa o nome completo 'AnimationComponent' ---
        else if (world.hasComponent<Engine::ECS::Component::AnimationComponent>(entityID)) 
        {
             // (Este log agora substitui o antigo "ERRO CRÍTICO")
             Engine::Core::Log::Warn(std::format("[DEBUG_RENDER] Entidade {} tem AnimComp, mas o debug de ossos falhou (boneTransformsPtr={}, hasSkeleton={}).", 
                static_cast<uint32_t>(entityID),
                (boneTransformsPtr != nullptr),
                (model ? model->hasSkeleton() : false)
             ));
        }
    }

    m_renderer.endScene();
}