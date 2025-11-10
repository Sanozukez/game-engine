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

    // Escolhe a trilha de tempo "fonte" para o canal:
    // 1) se houver rotationKeys, usa eles (ângulos quase sempre animam!);
    // 2) senão, se houver positionKeys, usa eles;
    // 3) senão, se houver scaleKeys, usa eles;
    // 4) vazio => sem animação (progress=0 e indices=0).
    static inline size_t pickKeyCount(const Asset::AnimationChannel &c, int &which)
    {
        if (!c.rotationKeys.empty()) { which = 0; return c.rotationKeys.size(); }
        if (!c.positionKeys.empty()) { which = 1; return c.positionKeys.size(); }
        if (!c.scaleKeys.empty())    { which = 2; return c.scaleKeys.size(); }
        which = -1; return 0;
    }

    static inline float getKeyTimeAt(const Asset::AnimationChannel &c, int which, size_t i)
    {
        switch (which)
        {
        case 0: return c.rotationKeys[i].time;
        case 1: return c.positionKeys[i].time;
        case 2: return c.scaleKeys[i].time;
        default: return 0.0f;
        }
    }

    float KeyframeSampler::findKeyframePairAndGetProgress(
        const Asset::AnimationChannel &channel,
        float animationTime,
        size_t &indexA,
        size_t &indexB)
    {
        int which = -1;
        const size_t keyCount = pickKeyCount(channel, which);
        if (which < 0 || keyCount == 0)
        {
            indexA = indexB = 0;
            return 0.0f;
        }

        if (keyCount == 1)
        {
            indexA = indexB = 0;
            return 0.0f;
        }

        // Se o tempo está além do último, prende no último par
        const float lastTime = getKeyTimeAt(channel, which, keyCount - 1);
        if (animationTime >= lastTime)
        {
            indexA = keyCount - 1;
            indexB = indexA;
            return 0.0f;
        }

        // Busca linear simples (os counts são pequenos; dá pra otimizar depois)
        for (size_t i = 0; i < keyCount - 1; ++i)
        {
            const float t0 = getKeyTimeAt(channel, which, i);
            const float t1 = getKeyTimeAt(channel, which, i + 1);
            if (animationTime < t1)
            {
                indexA = i;
                indexB = i + 1;
                const float dt = (t1 - t0);
                return (dt > 0.0f) ? (animationTime - t0) / dt : 0.0f;
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