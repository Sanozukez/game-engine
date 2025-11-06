// engine/ecs/systems/animation_system.cpp
//
// CORREÇÃO: Usando os nomes corretos (AnimationComponent/AnimationAsset)
// e a arquitetura SRP (Escreve resultados no Componente).

#include "animation_system.h"
#include "../world.h"
#include "../components/transform_component.h"
#include "../components/movement_component.h"
#include "../components/animation_component.h" // <-- Inclui AnimationComponent
#include "../components/mesh_component.h"
#include "../../asset/skeleton.h"
#include "../../asset/animation.h" // <-- Inclui AnimationAsset
#include "../../animation/keyframe_sampler.h"
#include "../../animation/skeleton_hierarchy.h"
#include "../../core/log.h"
#include "../../asset/model.h"

#include <glm/gtx/matrix_decompose.hpp> // para glm::decompose
#include <glm/gtc/matrix_transform.hpp> // para glm::translate/scale
#include <glm/gtx/norm.hpp>
#include <format>
#include <map>
#include <stdexcept>
#include <memory>

// (Removendo 'using namespace' para evitar conflitos de nome)
using namespace Engine::ECS;
using namespace Engine::ECS::System;

// --- ADICIONADO: Helpers (que estavam em animation_utils) ---
// (No futuro, isto vai para PoseBlender.cpp, como planeámos)
static void decomposeTRS(const glm::mat4 &m, glm::vec3 &T, glm::quat &R, glm::vec3 &S)
{
    glm::vec3 skew;
    glm::vec4 persp;
    glm::decompose(m, S, R, T, skew, persp);
    R = glm::normalize(R);
}
static glm::mat4 composeTRS(const glm::vec3 &T, const glm::quat &R, const glm::vec3 &S)
{
    glm::mat4 M(1.0f);
    M = glm::translate(M, T);
    M *= glm::mat4_cast(R);
    M = glm::scale(M, S);
    return M;
}
// --- FIM DOS HELPERS ---

AnimationSystem::AnimationSystem(Engine::Asset::AssetManager &assetManager)
    : m_assetManager(assetManager)
{
}

void AnimationSystem::update(World &world, float dt)
{
    if (m_entities.empty())
    {
        return;
    }

    for (const EntityID entityID : m_entities)
    {
        // 1. OBTER COMPONENTES (Usando nomes completos)
        Engine::ECS::Component::AnimationComponent &animComp = world.getComponent<Engine::ECS::Component::AnimationComponent>(entityID);
        Engine::ECS::Component::Mesh &meshComp = world.getComponent<Engine::ECS::Component::Mesh>(entityID);

        std::shared_ptr<Engine::Asset::Model> model = m_assetManager.getModel(meshComp.assetID);

        // (Log [DEBUG_PTR])
        Engine::Core::Log::Error(std::format("[DEBUG_PTR] AnimationSystem: Entidade {} usa Model@0x{:X}", 
            static_cast<uint32_t>(entityID), 
            reinterpret_cast<uintptr_t>(model.get())
        ));

        // 2. OBTER ESQUELETO E ASSET DE ANIMAÇÃO
        Engine::Skeleton *skeleton = model->getSkeleton();

        // --- CORREÇÃO: Usa AnimationAsset ---
        const Engine::Asset::AnimationAsset *currentAnim = model->getAnimation(animComp.currentAnimationID);

        // 3. VERIFICAÇÃO DE SEGURANÇA
        if (!skeleton || !currentAnim)
        {
            continue;
        }

        // 4. ATUALIZAR TEMPO (no Componente)
        animComp.currentTime += dt * currentAnim->ticksPerSecond;
        float duration = currentAnim->duration;
        if (duration > 0.0f)
        {
            animComp.currentTime = std::fmod(animComp.currentTime, duration);
        }

       // 5. PASSO 1 (Rest Pose)
        std::map<int, glm::mat4> boneFinalLocalTransforms;
        for (const auto &bone : skeleton->bones)
        {
            glm::mat4 restPoseTransform = model->getNodeLocalTransform(bone.name);
            if (bone.id == skeleton->rootNodeId) {
                Engine::Core::Log::Info(std::format("[ANIM_DEBUG] Rest Pose T (Root): ({:.2f}, {:.2f}, {:.2f})",
                                                    restPoseTransform[3].x, restPoseTransform[3].y, restPoseTransform[3].z));
            }
            boneFinalLocalTransforms[bone.id] = restPoseTransform;
        }

        // 6. PASSO 2 (Override da Animação)
        // --- ESTA É A LÓGICA CORRETA (que impede o colapso) ---
        for (const auto &pair : currentAnim->channels)
        {
            const Engine::Asset::AnimationChannel &channel = pair.second; 
            if (channel.boneId == -1) continue;

            // 1. Obter a Rest Pose (do Passo 5)
            // (Esta é a nossa base, ex: T(0, 0.68, 0))
            glm::mat4 restPoseTransform = boneFinalLocalTransforms[channel.boneId];
            
            // 2. Decompô-la em T, R, S
            glm::vec3 restT, restS;
            glm::quat restR;
            decomposeTRS(restPoseTransform, restT, restR, restS);

            // 3. Definir os valores base como a Rest Pose
            glm::vec3 T = restT;
            glm::quat R = restR;
            glm::vec3 S = restS;

            size_t indexA, indexB;
            float progress;

            // 4. Sobrescrever T, R, S APENAS se o canal de animação existir
            if (!channel.positionKeys.empty())
            {
                // (O seu 'animation_utils' antigo tinha uma lógica 'isRoot' aqui.
                // Vamos omiti-la por agora, mas ela pode ser a causa
                // da "animação bugada" se o seu 'root' tiver T=0)
                progress = KeyframeSampler::findKeyframePairAndGetProgress(channel, animComp.currentTime, indexA, indexB);
                T = KeyframeSampler::interpolateTranslation(channel, progress, indexA, indexB);
            }
            
            if (!channel.rotationKeys.empty())
            {
                progress = KeyframeSampler::findKeyframePairAndGetProgress(channel, animComp.currentTime, indexA, indexB);
                R = KeyframeSampler::interpolateRotation(channel, progress, indexA, indexB);
            }

            if (!channel.scaleKeys.empty())
            {
                progress = KeyframeSampler::findKeyframePairAndGetProgress(channel, animComp.currentTime, indexA, indexB);
                S = KeyframeSampler::interpolateScale(channel, progress, indexA, indexB);
            }

            // 5. Compor a matriz final (ex: T(rest), R(anim), S(rest))
            glm::mat4 localTransform = composeTRS(T, R, S);

            // 6. Salvar a matriz final no mapa
            boneFinalLocalTransforms[channel.boneId] = localTransform;
            
            if (channel.boneId == 0) {
                 Engine::Core::Log::Info(std::format("[DEBUG_SYS] Root (ID 0) LocalTransform Sendo Usada: T({:.2f}, {:.2f}, {:.2f}), S({:.2f}, {:.2f}, {:.2f})",
                    T.x, T.y, T.z, S.x, S.y, S.z));
            }
        }
        // --- FIM DA LÓGICA CORRETA ---
        
        
        // 7. PASSO 3 (Cinemática Forward)
        const glm::mat4& skeletonRootTransform = model->getSkeletonBindTransform();
        SkeletonHierarchy::traverseAndCalculateFinalTransforms(
            *skeleton, 
            boneFinalLocalTransforms,
            skeletonRootTransform
        );

        // 8. COPIA OS RESULTADOS PARA O COMPONENTE (A CORREÇÃO SRP)
        skeleton->getFinalBoneTransforms(animComp.finalBoneTransforms);

        // 9. Debug (Logs mantidos)
        if (skeleton->rootNodeId != -1)
        {
            const Engine::Bone &root = skeleton->bones[skeleton->rootNodeId]; 
            Engine::Core::Log::Info(std::format("[ANIM_DEBUG] Root ({}) Final T: ({:.2f}, {:.2f}, {:.2f})",
                                                root.name, root.finalTransformation[3].x, root.finalTransformation[3].y, root.finalTransformation[3].z));
            Engine::Core::Log::Info(std::format("[ANIM_DEBUG] Root ({}) IBM T: ({:.2f}, {:.2f}, {:.2f})",
                                                root.name, root.inverseBindMatrix[3].x, root.inverseBindMatrix[3].y, root.inverseBindMatrix[3].z));
        }
    }
}