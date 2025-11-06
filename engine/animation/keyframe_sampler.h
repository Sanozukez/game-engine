// engine/animation/keyframe_sampler.h
#pragma once

#include "../asset/animation.h" // (Agora define AnimationChannel)
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// (Fwd declaration para Engine::Math::Quat se for usado aqui, 
// mas parece que o .h só usa glm::quat)

namespace Engine {

    // Fwd declaration para a struct que vem de animation.h
    namespace Asset {
        struct AnimationChannel;
    }

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
        // --- CORREÇÃO: Renomeado para AnimationChannel ---
        static float findKeyframePairAndGetProgress(
            const Asset::AnimationChannel& channel, // <-- MUDOU
            float animationTime,
            size_t& indexA,
            size_t& indexB
        );

        static glm::vec3 interpolateTranslation(const Asset::AnimationChannel& channel, float progress, size_t indexA, size_t indexB); // <-- MUDOU
        static glm::quat interpolateRotation(const Asset::AnimationChannel& channel, float progress, size_t indexA, size_t indexB); // <-- MUDOU
        static glm::vec3 interpolateScale(const Asset::AnimationChannel& channel, float progress, size_t indexA, size_t indexB); // <-- MUDOU
        // --- FIM DA CORREÇÃO ---
    };

} // namespace Engine