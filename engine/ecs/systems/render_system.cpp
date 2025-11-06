// engine/ecs/systems/render_system.cpp
//
// Implementação CORRIGIDA, usando o Model::Skeleton para extrair as matrizes.

#include "render_system.h"
#include "../../ecs/world.h"
#include "../../render/shader_manager.h" // Necessário para buscar o shader de armature
#include "../components/transform_component.h"
#include "../components/animation_component.h"
#include "../components/mesh_component.h" // Necessário para Component::Mesh
#include "../../asset/model.h"            // Necessário para Engine::Asset::Model
#include "../../math/transform_utils.h"
#include "../../asset/skeleton.h" // Necessário para Engine::Asset::Skeleton

#include <glm/glm.hpp>
#include <memory>
#include <format>
#include <vector> // Necessário para std::vector<glm::mat4>

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
    // Obtém as matrizes de View e Projection
    glm::mat4 viewMatrix = m_renderer.getCamera().getViewMatrix();
    glm::mat4 projectionMatrix = m_renderer.getCamera().getProjectionMatrix();
    // NOTA: A matriz ViewProjection é calculada no renderer ou camera, mas passamos view/proj separadas para o armature.

    // 1. Inicia o frame (chamada de baixo nível)
    m_renderer.beginScene();

    // 2. Itera sobre a lista de entidades (m_entities) com Transform e Mesh
    for (const EntityID entityID : m_entities)
    {
        // 3. Obtém os componentes necessários
        Transform &transform = world.getComponent<Transform>(entityID);
        Engine::ECS::Component::Mesh &mesh = world.getComponent<Engine::ECS::Component::Mesh>(entityID);

        // --- PREPARAÇÃO DE ANIMAÇÃO (Correção C2039) ---
        std::vector<glm::mat4> finalBoneTransforms; // Vetor local para as matrizes
        const std::vector<glm::mat4> *boneTransformsPtr = nullptr;

        // Obter o modelo associado ao MeshComponent
        std::shared_ptr<Engine::Asset::Model> model = m_assetManager.getModel(mesh.assetID);

        // Se tiver Animation Component e o Modelo tiver um Skeleton, extrair as transforms.
        if (model && model->hasSkeleton() && world.hasComponent<Animation>(entityID))
        {
            Skeleton *skeleton = model->getSkeleton();
            if (skeleton)
            {
                // Usa o novo método do Skeleton (adicionado na última etapa) para extrair os resultados.
                // Isso funciona porque o AnimationSystem já atualizou o 'finalTransformation' do Skeleton.
                skeleton->getFinalBoneTransforms(finalBoneTransforms);

                if (!finalBoneTransforms.empty())
                {
                    boneTransformsPtr = &finalBoneTransforms;
                }
            }
        }
        // -----------------------------------------------------

        // 4. DESENHA A MALHA
        m_renderer.submit(mesh.assetID, transform, boneTransformsPtr);

        // **********************************************
        // 5. DEBUG: VISUALIZAÇÃO DO ESQUELETO
        // **********************************************
        // Assumindo que a flag de debug deve estar aqui (ex: m_debugArmature, se fosse um membro)
        if (boneTransformsPtr != nullptr && model && model->hasSkeleton())
        {
            // 3.1. Extrai as linhas de debug
            // CORREÇÃO C2660: Função não recebe mais o argumento *boneTransforms.
            Engine::Core::Log::Info("[RENDER_DEBUG] Tentando gerar linhas de debug do esqueleto."); // <-- ADICIONAR ESTE LOG
            std::vector<glm::vec3> debugLines = model->getSkeletonDebugLines();                     // <-- CORRIGIDO

            if (!debugLines.empty())
            {
                Engine::Core::Log::Info(std::format("[RENDER_DEBUG] Geradas {} linhas de debug. Desenhando.", debugLines.size()));
                // 3.2. Obtém o shader de Armature
                Shader &armatureShader = ShaderManager::Get().getShader(m_assetManager.getAssetIDByName("armature"));

                // NOVO: Calcula a Model Matrix do personagem
                glm::mat4 modelMatrix = Engine::Math::getTransformMatrix(transform);

                // 3.3. Desenha o esqueleto
                m_armatureRenderer.draw(armatureShader, debugLines, viewMatrix, projectionMatrix, modelMatrix);
            }
            else
            {
                Engine::Core::Log::Warn("[RENDER_DEBUG] getSkeletonDebugLines retornou vetor vazio."); // <-- ADICIONAR ESTE LOG
            }
        }
    }

    // 6. Finaliza o frame
    m_renderer.endScene();
}