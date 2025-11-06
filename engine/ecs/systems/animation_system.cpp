// engine/ecs/systems/animation_system.cpp
//
// CORREÇÃO: Usando o mapa 'boneFinalLocalTransforms' (Pose de Descanso + Override de Animação)
// e passando-o para o SkeletonHierarchy.

#include "animation_system.h"
#include "../world.h"
#include "../components/transform_component.h"
#include "../components/movement_component.h"
#include "../components/animation_component.h"
#include "../components/mesh_component.h" // Necessário para a assinatura
#include "../../asset/skeleton.h"
#include "../../asset/animation.h"
#include "../../animation/keyframe_sampler.h"
#include "../../animation/skeleton_hierarchy.h"
#include "../../core/log.h"
#include "../../asset/model.h" // Necessário para getNodeLocalTransform

#include <glm/gtx/norm.hpp>
#include <format>
#include <map>
#include <stdexcept>
#include <memory>

using namespace Engine::ECS;
using namespace Engine::ECS::System;
using namespace Engine::ECS::Component;
using namespace Engine::Asset;

// Construtor
AnimationSystem::AnimationSystem(AssetManager &assetManager)
    : m_assetManager(assetManager)
{
    // A assinatura (Signature) é definida no app_setup.cpp
}

void AnimationSystem::update(World &world, float dt)
{
    // LOG 1: (Mantido)
    if (m_entities.empty())
    {
        Engine::Core::Log::Warn("[ANIM_SYSTEM] Nenhuma entidade para processar neste frame. (m_entities vazio)");
        return;
    }

    // LOG 2: (Mantido)
    Engine::Core::Log::Info(std::format("[ANIM_SYSTEM] Processando {} entidades.", m_entities.size()));

    for (const EntityID entityID : m_entities)
    {
        Engine::ECS::Component::Animation &animComp = world.getComponent<Engine::ECS::Component::Animation>(entityID);
        Engine::ECS::Component::Mesh &meshComp = world.getComponent<Engine::ECS::Component::Mesh>(entityID);

        std::shared_ptr<Model> model = m_assetManager.getModel(meshComp.assetID);

        if (!model || !model->hasSkeleton())
        {
            continue;
        }

        Skeleton *skeleton = model->getSkeleton();
        const Animation *currentAnim = model->getAnimation(animComp.currentAnimationID);

        if (!skeleton || !currentAnim)
        {
            continue;
        }

        // 1. Atualizar o tempo da animação (Mantido)
        animComp.currentTime += dt * currentAnim->ticksPerSecond;
        float duration = currentAnim->duration;
        if (duration > 0.0f)
        {
            animComp.currentTime = std::fmod(animComp.currentTime, duration);
        }

        // --- CORREÇÃO (Passo 1): Preparar o mapa da Pose Final ---

        // Este é o único mapa que precisamos.
        std::map<int, glm::mat4> boneFinalLocalTransforms;

        // Primeiro, preenchemos o mapa com a POSE DE DESCANSO (Rest Pose / T-Pose)
        // que está armazenada na hierarquia de Nós (Nodes) do Modelo.
        for (const auto &bone : skeleton->bones)
        {
            // Obtém a transformação de descanso
            glm::mat4 restPoseTransform = model->getNodeLocalTransform(bone.name);

            // LOG DE DEPURAÇÃO: Checar o que estamos recebendo
            if (bone.id == skeleton->rootNodeId)
            { // Logar apenas o Root para evitar spam
                Engine::Core::Log::Info(std::format("[ANIM_DEBUG] Rest Pose T (Root): ({:.2f}, {:.2f}, {:.2f})",
                                                    restPoseTransform[3].x, restPoseTransform[3].y, restPoseTransform[3].z));
            }

            boneFinalLocalTransforms[bone.id] = restPoseTransform;
        }

        // --- CORREÇÃO (Passo 2): Aplicar a Animação (Override) ---
        // Agora, sobrescrevemos a pose de descanso com a pose animada (ex: "idle")
        for (const auto &pair : currentAnim->channels)
        {
            const BoneChannel &channel = pair.second;

            if (channel.boneId == -1 ||
                channel.positionKeys.empty() ||
                channel.rotationKeys.empty() ||
                channel.scaleKeys.empty())
            {
                continue;
            }

            Engine::Core::Log::Debug(std::format("[ANIM_DBG] Sampling Bone: {}", channel.boneName));

            size_t indexA, indexB;
            float progress = KeyframeSampler::findKeyframePairAndGetProgress(
                channel,
                animComp.currentTime,
                indexA,
                indexB);

            // 2a. Interpolação T, R, S
            glm::vec3 T = KeyframeSampler::interpolateTranslation(channel, progress, indexA, indexB);
            glm::quat R = KeyframeSampler::interpolateRotation(channel, progress, indexA, indexB);
            glm::vec3 S = KeyframeSampler::interpolateScale(channel, progress, indexA, indexB);

            // 2b. Calcular a Matriz Local: T * R * S
            glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), T);
            localTransform *= glm::mat4_cast(R);
            localTransform = glm::scale(localTransform, S);

            // ** A MUDANÇA: Sobrescreve a pose de descanso pela pose animada **
            boneFinalLocalTransforms[channel.boneId] = localTransform;
        }

        // 3. Aplicar Cinemática Forward (Usando o mapa correto)
        SkeletonHierarchy::traverseAndCalculateFinalTransforms(
            *skeleton,
            boneFinalLocalTransforms); // <--- PASSANDO O MAPA CORRETO

        // 4. Debug (Logs mantidos)
        if (skeleton->rootNodeId != -1)
        {
            const Bone &root = skeleton->bones[skeleton->rootNodeId];
            Engine::Core::Log::Info(std::format("[ANIM_DEBUG] Root ({}) Final T: ({:.2f}, {:.2f}, {:.2f})",
                                                root.name, root.finalTransformation[3].x, root.finalTransformation[3].y, root.finalTransformation[3].z));
            Engine::Core::Log::Info(std::format("[ANIM_DEBUG] Root ({}) IBM T: ({:.2f}, {:.2f}, {:.2f})",
                                                root.name, root.inverseBindMatrix[3].x, root.inverseBindMatrix[3].y, root.inverseBindMatrix[3].z));
        }
    }
}