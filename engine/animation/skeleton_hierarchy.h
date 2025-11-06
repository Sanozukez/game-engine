// engine/animation/skeleton_hierarchy.h
#pragma once

#include "../asset/skeleton.h"
#include <glm/glm.hpp>
#include <map>

namespace Engine {

    /**
     * @brief Gerencia a hierarquia do esqueleto e calcula as transformações globais.
     * SRP: Foco na aplicação da Cinemática Forward e cálculo da Matriz Final.
     */
    class SkeletonHierarchy {
    public:
        SkeletonHierarchy() = delete; 

        /**
         * @brief Percorre recursivamente o esqueleto e calcula a matriz final de cada bone.
         * @param currentBoneTransformations O mapa das matrizes locais (T*R*S) calculadas para o frame atual.
         */
        static void traverseAndCalculateFinalTransforms(
            Skeleton& skeleton,
            const std::map<int, glm::mat4>& currentBoneTransformations
        );

    private:
        static void calculateBoneGlobalTransform(
            Skeleton& skeleton,
            int boneId,
            const std::map<int, glm::mat4>& currentBoneTransformations,
            const glm::mat4& parentGlobalTransform
        );
    };

} // namespace Engine