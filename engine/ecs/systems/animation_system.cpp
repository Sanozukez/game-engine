// // engine/ecs/systems/animation_system.cpp

#include "animation_system.h"
#include "../world.h"
#include "../components/transform_component.h"
#include "../components/movement_component.h"
#include "../components/animation_component.h"
#include "../../asset/animation_utils.h"
#include "../../core/log.h"
#include <glm/gtx/norm.hpp>
#include <format>
// #include <functional>

using namespace Engine::ECS::System;
using namespace Engine::ECS::Component;
using namespace Engine::Asset;

// Definimos o hash usando o AnimationUtils (que usa std::hash)
// Isso é necessário porque o AnimationSystem::update não é static.
const uint32_t ANIM_IDLE = AnimationUtils::getAnimationHashID("IDLE"); // CORRIGIDO
const uint32_t ANIM_RUN = AnimationUtils::getAnimationHashID("RUN");   // CORRIGIDO


AnimationSystem::AnimationSystem(AssetManager& assetManager) 
    : m_assetManager(assetManager)
{
    // O construtor é simples, apenas armazena a dependência.
}

// Implementação simples da função utilitária (para uso futuro)
uint32_t AnimationSystem::getAnimationID(const std::string& animationName) const
{
    // Esta função deve usar o AssetManager::getAssetIDByName(animationName)
    // para garantir o hash consistente do sistema, usando a instância injetada.
    return AnimationUtils::getAnimationHashID(animationName);
}

void AnimationSystem::update(World &world, float dt)
{
    // Itera sobre todas as Entidades com a assinatura: Transform, Movement, Animation
    for (const auto& entity : m_entities)
    {
        // auto& tr = world.getComponent<Transform>(entity); // Transform é necessário para o cálculo final
        auto& mv = world.getComponent<Movement>(entity);
        auto& anim = world.getComponent<Animation>(entity);
        
        // ----------------------------------------------------------------------
        // 1. DETERMINAÇÃO DO ESTADO (SRP: Traduz Movimento -> Estado Visual)
        // ----------------------------------------------------------------------

        uint32_t desiredAnimID;
        // Se a velocidade for maior que um pequeno epsilon ou estiver se movendo por CTM
        bool isMoving = (mv.currentVelocity > 0.01f) || mv.isMovingToDestination;
        
        if (isMoving) {
            desiredAnimID = ANIM_RUN;
        } else {
            desiredAnimID = ANIM_IDLE;
        }

        // ----------------------------------------------------------------------
        // 2. LÓGICA DE TRANSIÇÃO E BLEND (SRP: Suavização)
        // ----------------------------------------------------------------------

        if (desiredAnimID != anim.currentAnimationID)
        {
            // Se já não estivermos em transição, configuramos a nova transição:
            if (anim.blendFactor >= 1.0f)
            {
                anim.previousAnimationID = anim.currentAnimationID;
                anim.currentAnimationID = desiredAnimID;
                anim.blendFactor = 0.0f; // Inicia a transição
                // Log::Debug(std::format("[AnimSystem] Transicionando para ID: {}", desiredAnimID));
            }
        }

        // Atualiza o fator de blend
        if (anim.blendFactor < 1.0f)
        {
            anim.blendFactor += anim.blendSpeed * dt;
            anim.blendFactor = glm::min(anim.blendFactor, 1.0f);
        }

        // ----------------------------------------------------------------------
        // 3. CÁLCULO DE FRAMES (Placeholder para o Core de Animação)
        // ----------------------------------------------------------------------
        
        // Atualiza o tempo da animação (simplesmente avança o contador)
        anim.currentTime += dt; 
        
        // Busca o modelo
        std::shared_ptr<Model> model = m_assetManager.getModel(anim.animationAssetID);

        if (model)
        {
            // O Utilitário faz o cálculo complexo
            AnimationUtils::calculateBoneTransforms(
                model, 
                anim.currentAnimationID, 
                anim.previousAnimationID, 
                anim.currentTime, 
                anim.blendFactor, 
                anim.finalBoneTransforms
            );
        }       
        
        // Por enquanto, garantimos que o vetor de transforms finais está pronto para o Renderer
        if (anim.finalBoneTransforms.empty())
        {
             anim.finalBoneTransforms.resize(Animation::MAX_BONES, glm::mat4(1.0f));
        }
    }
}