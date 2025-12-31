// engine/ecs/systems/animation_system.cpp
//
// CORREÇÃO: Usando os nomes corretos (AnimationComponent/AnimationAsset)
// e a arquitetura SRP (Escreve resultados no Componente).

#include "animation_system.h"
#include "../../animation/animation_config.h"
#include "../../animation/forward_kinematics.h"
#include "../../animation/pose_utils.h"
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
#include "../../core/profiler.h"  // <-- PROFILING
#include "../../asset/model.h"
#include "../../math/transform_utils.h"

#include <glm/gtx/matrix_decompose.hpp> // para glm::decompose
#include <glm/gtc/matrix_transform.hpp> // para glm::translate/scale
#include <glm/gtc/quaternion.hpp>

#include <glm/gtx/norm.hpp>
#include <cmath> // std::fabs
#include <format>
#include <map>
#include <stdexcept>
#include <memory>

// (Removendo 'using namespace' para evitar conflitos de nome)
using namespace Engine::ECS;
using namespace Engine::ECS::System;

// Atualizar o construtor para corresponder à declaração no .h
AnimationSystem::AnimationSystem(Engine::Asset::AssetManager& assetManager)
    : m_assetManager(assetManager)
{
    m_config.useRestPoseOnly = false;
    m_config.disableScaleKeys = true;
}

void AnimationSystem::playAnimation(Component::AnimationComponent& animComp, uint32_t newAnimationID, float blendDuration)
{
    // Se já está tocando essa animação, não fazer nada
    if (animComp.currentAnimationID == newAnimationID && animComp.blendFactor >= 1.0f) {
        return;
    }

    // Configurar transição
    animComp.previousAnimationID = animComp.currentAnimationID;
    animComp.currentAnimationID = newAnimationID;
    animComp.currentTime = 0.0f; // Resetar tempo para nova animação
    animComp.blendFactor = 0.0f; // Começar do início da transição
    animComp.blendSpeed = (blendDuration > 0.0f) ? (1.0f / blendDuration) : 1000.0f; // Blend speed em 1/s
}

// =========================================================================
// NOVO v101: playAnimationByName - Usa AnimationMapping do AssetManager
// =========================================================================
void AnimationSystem::playAnimationByName(Component::AnimationComponent& animComp, 
                                         const std::shared_ptr<Engine::Asset::Model>& model,
                                         const std::string& engineName)
{
    if (!model) {
        Engine::Core::Log::Error("playAnimationByName: Model inválido!");
        return;
    }

    // 1. Calcular hash do engine_name
    std::hash<std::string> hasher;
    uint32_t engineNameHash = static_cast<uint32_t>(hasher(engineName));

    // 2. Buscar AnimationMapping no AssetManager
    const AnimationMapping* mapping = m_assetManager.getAnimationMapping(engineNameHash);
    
    if (!mapping) {
        // FALLBACK: Tentar "idle"
        Engine::Core::Log::Warn(std::format("Animação '{}' não encontrada no asset dictionary! Tentando 'idle'...", engineName));
        
        if (engineName != "idle") {
            playAnimationByName(animComp, model, "idle"); // Recursão com fallback
            return;
        } else {
            // Último fallback: primeira animação disponível
            if (model->getAnimationCount() > 0) {
                Engine::Core::Log::Warn("'idle' também não encontrado! Usando primeira animação disponível.");
                
                // Pegar o hash da primeira animação do map
                // NOTA: Como m_animations é privado, vamos tentar buscar pelo nome que sabemos existir
                // Por enquanto, vamos logar o erro e retornar
                Engine::Core::Log::Error("FALLBACK CRÍTICO: Nenhuma animação nomeada corretamente! Verifique o asset dictionary.");
                return;
            } else {
                Engine::Core::Log::Error("Model não tem NENHUMA animação! Impossível tocar animação.");
                return;
            }
        }
    }

    // 3. Buscar animação no Model usando source_name
    const Engine::Asset::AnimationAsset* anim = model->getAnimationByName(mapping->source_name);
    
    if (!anim) {
        Engine::Core::Log::Error(std::format("Animação '{}' mapeada para '{}', mas '{}' não existe no GLB!", 
                                             engineName, mapping->source_name, mapping->source_name));
        // Fallback para idle
        if (engineName != "idle") {
            playAnimationByName(animComp, model, "idle");
        }
        return;
    }

    // 4. Aplicar metadata do AnimationMapping
    uint32_t animHash = static_cast<uint32_t>(hasher(mapping->source_name)); // Usar HASH, não índice
    float blendTime = mapping->blend_in_time;
    
    // Atualizar playbackSpeed do componente (será usado no update)
    animComp.playbackSpeed = mapping->default_playback_speed;
    
    // 5. Iniciar animação com blend configurado
    playAnimation(animComp, animHash, blendTime);
    
    Engine::Core::Log::Info(std::format("Tocando animação: '{}' -> '{}' (speed: {:.2f}, blend: {:.2f}s)", 
                                        engineName, mapping->source_name, 
                                        mapping->default_playback_speed, blendTime));
}

void AnimationSystem::update(World& world, float dt)
{
    PROFILE_SCOPE("AnimationSystem::update"); // <-- PROFILING
    
    if (m_entities.empty()) {
        return;
    }

    for (const EntityID entityID : m_entities) {
        updateEntityAnimation(world, entityID, dt);
    }
}

void AnimationSystem::updateEntityAnimation(World& world, EntityID entityID, float dt)
{
    // Obter componentes
    auto& animComp = world.getComponent<Component::AnimationComponent>(entityID);
    auto& meshComp = world.getComponent<Component::Mesh>(entityID);

    std::shared_ptr<Engine::Asset::Model> model = m_assetManager.getModel(meshComp.assetID);
    Engine::Skeleton* skeleton = model ? model->getSkeleton() : nullptr;
    const Engine::Asset::AnimationAsset* currentAnim = 
        model ? model->getAnimation(animComp.currentAnimationID) : nullptr;

    // Validar inputs
    if (!validateAnimationInputs(skeleton, currentAnim)) {
        return;
    }

    // Atualizar blend factor (transição entre animações)
    updateBlendFactor(animComp, dt);

    // Atualizar tempo de animação
    updateAnimationTime(animComp, currentAnim, dt);

    std::map<int, glm::mat4> boneFinalLocalTransforms;

    // Se estamos em transição (blending), interpolar entre previous e current
    if (animComp.blendFactor < 1.0f && animComp.previousAnimationID != 0) {
        const Engine::Asset::AnimationAsset* previousAnim = 
            model->getAnimation(animComp.previousAnimationID);
        
        if (previousAnim) {
            blendAnimations(skeleton, model, animComp, currentAnim, previousAnim, boneFinalLocalTransforms);
        } else {
            // Se previous animation inválida, usar apenas current
            initializeNodeTransforms(skeleton, model, boneFinalLocalTransforms);
            if (!m_config.useRestPoseOnly) {
                for (const auto& pair : currentAnim->channels) {
                    const auto& channel = pair.second;
                    if (channel.boneId == -1) continue;

                    glm::mat4 nodeTransform = boneFinalLocalTransforms[channel.boneId];
                    glm::mat4 animatedTransform;
                    
                    applyAnimationChannel(channel, nodeTransform, animComp.currentTime, animatedTransform);
                    boneFinalLocalTransforms[channel.boneId] = animatedTransform;
                }
            }
        }
    } else {
        // Sem blending - apenas current animation
        initializeNodeTransforms(skeleton, model, boneFinalLocalTransforms);

        if (!m_config.useRestPoseOnly) {
            for (const auto& pair : currentAnim->channels) {
                const auto& channel = pair.second;
                if (channel.boneId == -1) continue;

                glm::mat4 nodeTransform = boneFinalLocalTransforms[channel.boneId];
                glm::mat4 animatedTransform;
                
                applyAnimationChannel(channel, nodeTransform, animComp.currentTime, animatedTransform);
                boneFinalLocalTransforms[channel.boneId] = animatedTransform;
            }
        }
    }

    // Atualizar transformações dos ossos
    updateBoneTransforms(skeleton, boneFinalLocalTransforms, animComp);
}

void AnimationSystem::updateAnimationTime(
    Component::AnimationComponent& animComp,
    const Engine::Asset::AnimationAsset* currentAnim,
    float dt)
{
    if (!currentAnim) return;

    // GLTF usa SEGUNDOS diretamente, não ticks!
    // duration e keyframe times já estão em segundos
    // Aplica playbackSpeed para controlar velocidade de reprodução (ex: Sprint = 1.5x)
    animComp.currentTime += dt * animComp.playbackSpeed;
    float duration = currentAnim->duration;
    
    if (duration > 0.0f) {
        animComp.currentTime = std::fmod(animComp.currentTime, duration);
    }
}

void AnimationSystem::initializeNodeTransforms(
    const Engine::Skeleton* skeleton,
    const std::shared_ptr<Engine::Asset::Model>& model,
    std::map<int, glm::mat4>& outLocalTransforms)
{
    if (!skeleton || !model) return;

    // PIPELINE DE IMPORTAÇÃO: Correção aplicada nos dados, não no runtime
    // Node transforms já corrigidos durante carregamento
    for (const auto& bone : skeleton->bones) {
        glm::mat4 nodeTransform = model->getNodeLocalTransform(bone.name);
        
        // CORREÇÃO JÁ APLICADA em animation_data_mapper.cpp - não duplicar!
        // A rest pose vem pura, mas keyframes já têm X=180° aplicado
        outLocalTransforms[bone.id] = nodeTransform;
    }
}

void AnimationSystem::applyAnimationChannel(
    const Engine::Asset::AnimationChannel& channel,
    const glm::mat4& nodeTransform,
    float currentTime,
    glm::mat4& outLocalTransform)
{
    // Decompor node transform original como fallback
    glm::vec3 nodeT, nodeS;
    glm::quat nodeR;
    Engine::Animation::decomposeTRS(nodeTransform, nodeT, nodeR, nodeS);

    // GLTF: Começar com node transform, keyframes sobrescrevem componentes específicos
    glm::vec3 T = nodeT;
    glm::quat R = nodeR;
    glm::vec3 S = nodeS;

    size_t indexA = 0, indexB = 0;
    float progress = 0.0f;

    // Se tem keyframes de posição, usar animação (senão, manter node transform)
    if (!channel.positionKeys.empty()) {
        progress = Engine::KeyframeSampler::findKeyframePairAndGetProgress(
            channel, currentTime, indexA, indexB);
        T = Engine::KeyframeSampler::interpolateTranslation(
            channel, progress, indexA, indexB);
        T = Engine::Animation::sanitizeVector3(T, nodeT);
    }

    // Se tem keyframes de rotação, usar animação (senão, manter node transform)
    if (!channel.rotationKeys.empty()) {
        progress = Engine::KeyframeSampler::findKeyframePairAndGetProgress(
            channel, currentTime, indexA, indexB);
        R = Engine::KeyframeSampler::interpolateRotation(
            channel, progress, indexA, indexB);
        R = Engine::Animation::sanitizeQuaternion(R, nodeR);
        
        // Log rotation for bone 1
        // if (channel.boneId == 1) {
        //     Engine::Core::Log::Info(std::format(
        //         "[ANIM_CHANNEL] Bone {} rotation from anim: R(w:{:.3f},x:{:.3f},y:{:.3f},z:{:.3f})",
        //         channel.boneId, R.w, R.x, R.y, R.z));
        // }
    }

    // Se tem keyframes de escala, usar animação (senão, manter node transform)
    if (!channel.scaleKeys.empty() && !m_config.disableScaleKeys) {
        progress = Engine::KeyframeSampler::findKeyframePairAndGetProgress(
            channel, currentTime, indexA, indexB);
        S = Engine::KeyframeSampler::interpolateScale(
            channel, progress, indexA, indexB);
        S = Engine::Animation::sanitizeScale(S);
    }

    outLocalTransform = Engine::Animation::composeTRS(T, R, S);
}

void AnimationSystem::updateBoneTransforms(
    const Engine::Skeleton* skeleton,
    const std::map<int, glm::mat4>& localTransforms,
    Component::AnimationComponent& animComp)
{
    if (!skeleton) return;

    const size_t numBones = skeleton->bones.size();
    if (numBones == 0) return;

    std::vector<glm::mat4> globalTransforms;
    Engine::Animation::ForwardKinematics::computeGlobalTransforms(
        skeleton,
        localTransforms,
        globalTransforms
    );

    // Log após FK (SEM correção global - mantém hierarquia)
    for (size_t i = 0; i < skeleton->bones.size(); ++i) {
        const auto& bone = skeleton->bones[i];
        
        if (i < 3) {
            glm::vec3 T, S;
            glm::quat R;
            // Engine::Animation::decomposeTRS(globalTransforms[i], T, R, S);
            // Engine::Core::Log::Info(std::format(
            //     "[FK_RESULT] Bone '{}' (ID:{}) - T({:.3f}, {:.3f}, {:.3f})",
            //     bone.name, bone.id, T.x, T.y, T.z));
        }
    }

    for (size_t i = 0; i < numBones; ++i) {
        const Engine::Bone& bone = skeleton->bones[i];
        const int jointIndex = (bone.jointIndex >= 0) ? 
            bone.jointIndex : static_cast<int>(i);

        if (static_cast<size_t>(jointIndex) >= animComp.finalBoneTransforms.size()) {
            animComp.finalBoneTransforms.resize(jointIndex + 1, glm::mat4(1.0f));
        }

        // A correção de orientação já foi aplicada nos keyframes em animation_data_mapper.cpp (Local Space)
        // Não aplicar mais correções aqui - deixar para o World Space (Transform da entidade)
        glm::mat4 correctedGlobal = globalTransforms[i];
        
        // Para a mesh: aplicar IBM na transformação global
        glm::mat4 finalTransform = correctedGlobal * bone.inverseBindMatrix;
        animComp.finalBoneTransforms[jointIndex] = finalTransform;
        
        // Para o debug skeleton: usar a transformação corrigida SEM IBM (espaço mundial)
        Engine::Bone& mutableBone = const_cast<Engine::Bone&>(bone);
        mutableBone.finalTransformation = correctedGlobal;
    }

    if (m_config.debug.showFKDebug && skeleton->rootNodeId != -1) {
        const auto& root = skeleton->bones[skeleton->rootNodeId];
        const glm::vec3 t = glm::vec3(root.finalTransformation[3]);
        Engine::Core::Log::Info(std::format(
            "[FK_DBG] Root '{}' final position: ({:.3f}, {:.3f}, {:.3f})",
            root.name, t.x, t.y, t.z));
    }
}

bool AnimationSystem::validateAnimationInputs(
    const Engine::Skeleton* skeleton,
    const Engine::Asset::AnimationAsset* currentAnim) const
{
    if (!skeleton) {
        if (m_config.validateInputs) {
            Engine::Core::Log::Error("[AnimSystem] Invalid skeleton");
        }
        return false;
    }

    if (!currentAnim) {
        if (m_config.validateInputs) {
            Engine::Core::Log::Error("[AnimSystem] Invalid animation asset");
        }
        return false;
    }

    if (skeleton->bones.empty()) {
        if (m_config.validateInputs) {
            Engine::Core::Log::Error("[AnimSystem] Skeleton has no bones");
        }
        return false;
    }

    return true;
}

void AnimationSystem::updateBlendFactor(Component::AnimationComponent& animComp, float dt)
{
    if (animComp.blendFactor < 1.0f) {
        animComp.blendFactor += dt * animComp.blendSpeed;
        if (animComp.blendFactor >= 1.0f) {
            animComp.blendFactor = 1.0f;
            animComp.previousAnimationID = 0; // Transição completa
        }
    }
}

void AnimationSystem::blendAnimations(
    const Engine::Skeleton* skeleton,
    const std::shared_ptr<Engine::Asset::Model>& model,
    Component::AnimationComponent& animComp,
    const Engine::Asset::AnimationAsset* currentAnim,
    const Engine::Asset::AnimationAsset* previousAnim,
    std::map<int, glm::mat4>& outBlendedTransforms)
{
    if (!skeleton || !model || !currentAnim || !previousAnim) return;

    // Calcular transforms para animação anterior (em seu tempo atual)
    std::map<int, glm::mat4> previousTransforms;
    initializeNodeTransforms(skeleton, model, previousTransforms);
    
    if (!m_config.useRestPoseOnly) {
        for (const auto& pair : previousAnim->channels) {
            const auto& channel = pair.second;
            if (channel.boneId == -1) continue;

            glm::mat4 nodeTransform = previousTransforms[channel.boneId];
            glm::mat4 animatedTransform;
            
            // Usar o tempo atual da animação (já está progredindo)
            applyAnimationChannel(channel, nodeTransform, animComp.currentTime, animatedTransform);
            previousTransforms[channel.boneId] = animatedTransform;
        }
    }

    // Calcular transforms para animação atual
    std::map<int, glm::mat4> currentTransforms;
    initializeNodeTransforms(skeleton, model, currentTransforms);
    
    if (!m_config.useRestPoseOnly) {
        for (const auto& pair : currentAnim->channels) {
            const auto& channel = pair.second;
            if (channel.boneId == -1) continue;

            glm::mat4 nodeTransform = currentTransforms[channel.boneId];
            glm::mat4 animatedTransform;
            
            applyAnimationChannel(channel, nodeTransform, animComp.currentTime, animatedTransform);
            currentTransforms[channel.boneId] = animatedTransform;
        }
    }

    // Interpolar entre previous e current usando blendFactor
    for (const auto& bone : skeleton->bones) {
        glm::vec3 prevT, prevS, currT, currS;
        glm::quat prevR, currR;
        
        // Decompor previous
        Engine::Animation::decomposeTRS(previousTransforms[bone.id], prevT, prevR, prevS);
        
        // Decompor current
        Engine::Animation::decomposeTRS(currentTransforms[bone.id], currT, currR, currS);
        
        // Interpolar (blendFactor: 0.0 = 100% previous, 1.0 = 100% current)
        glm::vec3 blendedT = glm::mix(prevT, currT, animComp.blendFactor);
        glm::quat blendedR = glm::slerp(prevR, currR, animComp.blendFactor);
        glm::vec3 blendedS = glm::mix(prevS, currS, animComp.blendFactor);
        
        // Recompor matriz
        outBlendedTransforms[bone.id] = Engine::Animation::composeTRS(blendedT, blendedR, blendedS);
    }
}
