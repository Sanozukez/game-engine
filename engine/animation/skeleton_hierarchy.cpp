// engine/animation/skeleton_hierarchy.cpp
//
// Módulo de Cinemática Forward: SRP Focado na aplicação da hierarquia.

#include "skeleton_hierarchy.h"
#include "../../core/log.h"

// Includes para as definições completas de Skeleton e Bone
// (Assumindo que estão no namespace Engine::, como você indicou)
#include "../../asset/skeleton.h"
#include "../../asset/animation.h" 

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <format>
#include <map>

// ** A CORREÇÃO CRÍTICA **
// A implementação está dentro de 'namespace Engine', pois a classe é Engine::SkeletonHierarchy
namespace Engine {

    // --------------------------------------------------------------------------------
    // 1. IMPLEMENTAÇÃO DO MÉTODO PÚBLICO (Inicia a Recursão)
    // --------------------------------------------------------------------------------
    
    // Esta é a definição do método estático público da classe SkeletonHierarchy
    void SkeletonHierarchy::traverseAndCalculateFinalTransforms(
        Engine::Skeleton& skeleton, 
        const std::map<int, glm::mat4>& currentBoneTransformations,
        const glm::mat4& rootTransform // <-- NOVO ARGUMENTO
    ) {
        // 1. Verificação de segurança no nó raiz
        if (skeleton.rootNodeId != -1 && skeleton.rootNodeId < (int)skeleton.bones.size()) {
                       
            // Chama o helper 'static private' da PRÓPRIA CLASSE
            calculateBoneGlobalTransform(
                skeleton, 
                skeleton.rootNodeId, 
                currentBoneTransformations, 
                rootTransform // <-- Passa a matriz correta
            );
        } else {
            Core::Log::Warn(std::format("[HIERARCHY_FAIL] Root Node ID ({}) é inválido ou não setado. Max bones: {}.",
                skeleton.rootNodeId, skeleton.bones.size()));
        }
    }

    // --------------------------------------------------------------------------------
    // 2. IMPLEMENTAÇÃO DO HELPER 'static private'
    // --------------------------------------------------------------------------------

    // A definição agora corresponde ao .h: Engine::SkeletonHierarchy::calculateBoneGlobalTransform
    void Engine::SkeletonHierarchy::calculateBoneGlobalTransform(
        Engine::Skeleton& skeleton,
        int boneId,
        const std::map<int, glm::mat4>& currentBoneTransformations,
        const glm::mat4& parentGlobalTransform
    ) {
        // 1. VERIFICAÇÃO DE SEGURANÇA CRÍTICA (Evita o crash)
        if (boneId < 0 || boneId >= (int)skeleton.bones.size()) {
            Core::Log::Error(std::format("[HIERARCHY_CRASH_PREVENT] boneId ({}) fora dos limites (max {}). Parando recursão.", 
                boneId, skeleton.bones.size()));
            return;
        }

        Engine::Bone& currentBone = skeleton.bones[boneId];
        
        Core::Log::Debug(std::format("[HIERARCHY_DBG] Processando Bone ID {} ({}), Parent ID {}.",
            boneId, currentBone.name, currentBone.parentId));


        // 2. Obtém a Matriz Local (T*R*S)
        glm::mat4 localTransform = glm::mat4(1.0f);
        auto it = currentBoneTransformations.find(boneId);
        if (it != currentBoneTransformations.end()) {
            localTransform = it->second;
        }
        
        // 3. Calcula a Matriz Global (Cinemática Forward): Parent * Local
        glm::mat4 globalTransform = parentGlobalTransform * localTransform;

        // 4. Calcula a Matriz Final para o Shader: Global * IBM
        currentBone.finalTransformation = globalTransform * currentBone.inverseBindMatrix;

        // --- CORREÇÃO ---
        // 5. Salva a Matriz Global (antes da IBM) para Debug
        currentBone.debug_GlobalTransform = globalTransform;
        // --- FIM DA CORREÇÃO ---

        // 6. Propaga recursivamente
        for (int childId : currentBone.childrenIds) {
            calculateBoneGlobalTransform(skeleton, childId, currentBoneTransformations, globalTransform);
        }
    }

} // namespace Engine