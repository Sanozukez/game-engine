// engine/asset/skeleton.h
// (adicione o campo jointIndex no struct Bone)

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "./../../engine/core/log.h"

namespace Engine
{
    struct Bone
    {
        std::string name;
        int id = -1;
        int parentId = -1;
        std::vector<int> childrenIds;

        // NOVO: índice do joint conforme a ordem em skin->joints (glTF)
        // se não houver mapeamento, fica -1 e caímos no fallback (dst = i)
        int jointIndex = -1;

        glm::mat4 inverseBindMatrix = glm::mat4(1.0f);
        glm::mat4 finalTransformation = glm::mat4(1.0f);

        // debug opcional
        glm::mat4 debug_GlobalTransform = glm::mat4(1.0f);
    };

    class Skeleton
    {
    public:
        static constexpr int MAX_BONES = 100;

        int rootNodeId = -1;
        std::vector<Bone> bones;
        std::map<std::string, int> boneNameMap;

        const glm::mat4 &getFinalBoneTransform(int boneId) const
        {
            if (boneId >= 0 && boneId < (int)bones.size())
                return bones[boneId].finalTransformation;
            return *(new glm::mat4(1.0f));
        }

        void getFinalBoneTransforms(std::vector<glm::mat4> &outTransforms) const
        {
            if (outTransforms.size() < bones.size())
                outTransforms.resize(bones.size());

            for (size_t i = 0; i < bones.size(); ++i)
                outTransforms[i] = bones[i].finalTransformation;

            for (size_t i = bones.size(); i < outTransforms.size(); ++i)
                outTransforms[i] = glm::mat4(1.0f);
        }
    };
}
