// engine/animation/keyframe_sampler.h
#pragma once

#include "../asset/animation.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Engine {

    /**
     * @brief Utilitário para buscar keyframes e realizar interpolação.
     * SRP: Foco na Matemática da Animação.
     */
    class KeyframeSampler {
    public:
        KeyframeSampler() = delete; 

        /**
         * @brief Encontra o par de keyframes (A e B) para o tempo atual e calcula o progresso (0.0 a 1.0).
         */
        static float findKeyframePairAndGetProgress(
            const BoneChannel& channel, 
            float animationTime,
            size_t& indexA,
            size_t& indexB
        );

        static glm::vec3 interpolateTranslation(const BoneChannel& channel, float progress, size_t indexA, size_t indexB);
        static glm::quat interpolateRotation(const BoneChannel& channel, float progress, size_t indexA, size_t indexB);
        static glm::vec3 interpolateScale(const BoneChannel& channel, float progress, size_t indexA, size_t indexB);
    };

} // namespace Engine