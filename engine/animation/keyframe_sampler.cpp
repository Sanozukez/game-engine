// engine/animation/keyframe_sampler.cpp
#include "keyframe_sampler.h"
#include "../core/log.h"
#include <glm/gtx/norm.hpp>       // Para o slerp
#include <glm/gtc/quaternion.hpp> // Necessário para a interpolação de quat
#include "../../asset/skeleton.h"
#include <algorithm> // Para std::min

namespace Engine
{

    // Helper: Usa a lista de positionKeys para encontrar o tempo, assumindo que T, R e S estão sincronizados.
    float KeyframeSampler::findKeyframePairAndGetProgress(
        const BoneChannel &channel,
        float animationTime,
        size_t &indexA,
        size_t &indexB)
    {
        const auto &keyframes = channel.positionKeys;
        const size_t keyCount = keyframes.size(); // Usar um contador de tamanho

        // 1. CASO DE SEGURANÇA (0 ou 1 keyframe):
        // Se a contagem for <= 1, não há interpolação possível, e o índice de acesso é 0.
        if (keyCount <= 1)
        {
            indexA = indexB = 0;
            return 0.0f;
        }

        // 2. CASO LIMITE: Tempo além do fim do clipe (keyCount >= 2)
        // keyframes.back() é seguro porque keyCount >= 2
        if (animationTime >= keyframes.back().time)
        {
            indexA = indexB = keyCount - 1;
            return 0.0f;
        }

        // 3. BUSCA E INTERPOLAÇÃO (keyCount >= 2)
        // O loop itera de 0 até keyCount - 2, garantindo que [i + 1] seja seguro.
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

        // Fallback de segurança (nunca deveria ser alcançado)
        indexA = indexB = keyCount - 1;
        return 0.0f;
    }

    glm::vec3 KeyframeSampler::interpolateTranslation(const BoneChannel &channel, float progress, size_t indexA, size_t indexB)
    {
        const auto &keyframes = channel.positionKeys;

        // Verificação de segurança (deve ser redundante se findKeyframePair... estiver correto, mas é segura)
        if (keyframes.empty())
            return glm::vec3(0.0f);
        if (indexA >= keyframes.size() || indexB >= keyframes.size())
        {
            return keyframes.back().value;
        }

        if (indexA == indexB)
        {
            return keyframes[indexA].value;
        }
        const glm::vec3 &start = keyframes[indexA].value;
        const glm::vec3 &end = keyframes[indexB].value;
        return glm::mix(start, end, progress); // LERP
    }

    glm::quat KeyframeSampler::interpolateRotation(const BoneChannel &channel, float progress, size_t indexA, size_t indexB)
    {
        const auto &keyframes = channel.rotationKeys;

        // *** CORREÇÃO CRÍTICA DE CRASH ***
        if (keyframes.empty())
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Retorno neutro

        // Prende os índices aos limites REAIS deste vetor
        size_t safeIndexA = std::min(indexA, keyframes.size() - 1);
        size_t safeIndexB = std::min(indexB, keyframes.size() - 1);

        if (safeIndexA == safeIndexB)
        {
            return keyframes[safeIndexA].value; // Conversão implícita
        }

        // Usa os índices seguros
        return glm::slerp(keyframes[safeIndexA].value, keyframes[safeIndexB].value, progress);
    }

    glm::vec3 KeyframeSampler::interpolateScale(const BoneChannel &channel, float progress, size_t indexA, size_t indexB)
    {
        const auto &keyframes = channel.scaleKeys;

        // *** CORREÇÃO CRÍTICA DE CRASH ***
        if (keyframes.empty())
            return glm::vec3(1.0f); // Retorno neutro

        // Prende os índices aos limites REAIS deste vetor
        size_t safeIndexA = std::min(indexA, keyframes.size() - 1);
        size_t safeIndexB = std::min(indexB, keyframes.size() - 1);

        if (safeIndexA == safeIndexB)
        {
            return keyframes[safeIndexA].value;
        }

        // Usa os índices seguros
        return glm::mix(keyframes[safeIndexA].value, keyframes[safeIndexB].value, progress);
    }

} // namespace Engine