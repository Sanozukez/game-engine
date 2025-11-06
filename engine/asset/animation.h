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
    namespace Asset
    {

        // Dados de transformação de um Bone em um Keyframe.
        template <typename T>
        struct KeyFrame
        {
            float time;
            T value;
        };

        // --- RENOMEADO DE 'BoneChannel' PARA 'AnimationChannel' ---
        // Contém os keyframes para um único osso (T, R, S)
        struct AnimationChannel
        {
            std::string boneName;
            int boneId = -1;

            std::vector<KeyFrame<glm::vec3>> positionKeys;
            std::vector<KeyFrame<Engine::Math::Quat>> rotationKeys;
            std::vector<KeyFrame<glm::vec3>> scaleKeys;
        };

        // Define um clipe de animação (ex: "idle", "run")
        struct AnimationAsset
        {
            std::string name;
            float duration = 0.0f;
            float ticksPerSecond = 25.0f;

            // Mapeia nome do osso -> Canal de animação
            std::map<std::string, AnimationChannel> channels;
        };        

    } // namespace Asset
} // namespace Engine