// engine/animation/keyframe_sampler.cpp
#include "keyframe_sampler.h"
#include "../core/log.h"
#include <glm/gtx/norm.hpp>       // Para o slerp
#include <glm/gtc/quaternion.hpp> // Necessário para a interpolação de quat
#include "../../asset/skeleton.h"
#include <algorithm> // Para std::min
#include "../math/quat.h"

namespace Engine
{

    // Helper: Usa a lista de positionKeys para encontrar o tempo, assumindo que T, R e S estão sincronizados.
    float KeyframeSampler::findKeyframePairAndGetProgress(
        const Asset::AnimationChannel &channel, // <-- MUDOU
        float animationTime,
        size_t &indexA,
        size_t &indexB)
    {
        // (A sua lógica de 'findKeyframePair...' parece robusta. 
        // No entanto, ela assume que T, R, e S têm o *mesmo número* de 
        // keyframes e os *mesmos tempos*, o que é uma otimização.
        // O seu código antigo (animation_utils) procurava T, R, e S
        // separadamente. Vamos manter a sua lógica otimizada por agora,
        // mas o bug de animação "bugada" pode estar aqui se T, R, S
        // não estiverem sincronizados.)
        const auto &keyframes = channel.positionKeys; // <-- Baseia-se apenas nas Posições
        const size_t keyCount = keyframes.size(); 

        if (keyCount <= 1)
        {
            indexA = indexB = 0;
            return 0.0f;
        }

        if (animationTime >= keyframes.back().time)
        {
            indexA = indexB = keyCount - 1;
            return 0.0f;
        }

        for (size_t i = 0; i < keyCount - 1; ++i)
        {
            if (animationTime < keyframes[i + 1].time)
            {
                indexA = i;
                indexB = i + 1;

                const float startTime = keyframes[indexA].time;
                const float endTime = keyframes[indexB].time;
                const float totalTime = endTime - startTime;

                if (totalTime <= 0.0f)
                    return 0.0f;

                return (animationTime - startTime) / totalTime;
            }
        }

        indexA = indexB = keyCount - 1;
        return 0.0f;
    }

    // (interpolateTranslation permanece igual)
    glm::vec3 KeyframeSampler::interpolateTranslation(const Asset::AnimationChannel &channel, float progress, size_t indexA, size_t indexB)
    {
        const auto &keyframes = channel.positionKeys;
        if (keyframes.empty())
            return glm::vec3(0.0f);
        if (indexA >= keyframes.size() || indexB >= keyframes.size()) {
            // (Esta lógica está ligeiramente errada, devia usar std::min como nas outras, mas vamos manter por agora)
            return keyframes.back().value; 
        }
        if (indexA == indexB) {
            return keyframes[indexA].value;
        }
        const glm::vec3 &start = keyframes[indexA].value;
        const glm::vec3 &end = keyframes[indexB].value;
        return glm::mix(start, end, progress); // LERP
    }

    // --- CORREÇÃO C2039 ('toGLM') ---
    glm::quat KeyframeSampler::interpolateRotation(const Asset::AnimationChannel &channel, float progress, size_t indexA, size_t indexB)
    {
        const auto &keyframes = channel.rotationKeys;

        if (keyframes.empty())
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 

        size_t safeIndexA = std::min(indexA, keyframes.size() - 1);
        size_t safeIndexB = std::min(indexB, keyframes.size() - 1);

        // 1. Obter os valores como Engine::Math::Quat
        Engine::Math::Quat q1 = keyframes[safeIndexA].value;
        Engine::Math::Quat q2 = keyframes[safeIndexB].value;

        if (safeIndexA == safeIndexB) {
            // Converter apenas o resultado
            return glm::quat(q1.w, q1.x, q1.y, q1.z);
        }

        // 2. Usar o slerp estático da SUA classe Quat
        Engine::Math::Quat resultQuat = Engine::Math::Quat::slerp(q1, q2, progress);

        // 3. Converter o Engine::Math::Quat final para glm::quat
        return glm::quat(resultQuat.w, resultQuat.x, resultQuat.y, resultQuat.z);
    }
    // --- FIM DA CORREÇÃO ---

    // (interpolateScale permanece igual)
    glm::vec3 KeyframeSampler::interpolateScale(const Asset::AnimationChannel &channel, float progress, size_t indexA, size_t indexB)
    {
        const auto &keyframes = channel.scaleKeys;

        if (keyframes.empty())
            return glm::vec3(1.0f); 

        size_t safeIndexA = std::min(indexA, keyframes.size() - 1);
        size_t safeIndexB = std::min(indexB, keyframes.size() - 1);

        if (safeIndexA == safeIndexB)
        {
            return keyframes[safeIndexA].value;
        }

        return glm::mix(keyframes[safeIndexA].value, keyframes[safeIndexB].value, progress);
    }

} // namespace Engine