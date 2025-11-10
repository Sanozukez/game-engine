// engine/animation/forward_kinematics.h

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include "../asset/skeleton.h"

namespace Engine::Animation {

class ForwardKinematics {
public:
    static void computeGlobalTransforms(
        const Engine::Skeleton* skeleton,
        const std::map<int, glm::mat4>& localTransforms,
        std::vector<glm::mat4>& outGlobalTransforms);

private:
    static void initializeRootTransforms(
        const Engine::Skeleton* skeleton,
        const std::map<int, glm::mat4>& localTransforms,
        std::vector<glm::mat4>& globalTransforms);
};

} // namespace Engine::Animation