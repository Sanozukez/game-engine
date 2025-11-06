// engine/asset/animation.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <map>
#include "../math/quat.h"

namespace Engine
{

    /**
     * @brief Dados de transformação de um Bone em um Keyframe.
     */
    // struct Keyframe {
    //     float time;
    //     glm::vec3 translation;
    //     glm::quat rotation;
    //     glm::vec3 scale;
    // };
    template <typename T>
    struct KeyFrame
    {
        float time;
        T value;
    };

    /**
     * @brief Um canal de animação (Animation Channel) para um bone específico.
     */
    struct BoneChannel
    {
        std::string boneName;
        int boneId = -1;

        // FIX C2039: Adicionar as listas de keys que o AnimationSystem espera
        std::vector<KeyFrame<glm::vec3>> positionKeys;          // <-- CORRIGIDO
        std::vector<KeyFrame<Engine::Math::Quat>> rotationKeys; // <-- CORRIGIDO
        std::vector<KeyFrame<glm::vec3>> scaleKeys;             // <-- CORRIGIDO
    };

    /**
     * @brief Contém os dados de uma única animação.
     */
    class Animation
    {
    public:
        Animation() = default;

        std::string name;
        float duration = 0.0f;
        float ticksPerSecond = 25.0f;

        // Mapeamento nome do bone -> canal de animação.
        std::map<std::string, BoneChannel> channels;

        const BoneChannel *getChannel(const std::string &boneName) const
        {
            auto it = channels.find(boneName);
            return (it != channels.end()) ? &it->second : nullptr;
        }
    };

} // namespace Engine