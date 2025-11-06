// engine/asset/animation_data_mapper.h
#pragma once

// #include "skeleton.h"
// #include "animation.h"
#include <vector>
#include <memory>
#include <map>
#include <string>
// Forward declarations da dependência externa (cgltf)
struct cgltf_data;
struct cgltf_skin;

namespace Engine
{
    class Skeleton; // (Esqueleto final)
    struct Bone;    // (Osso individual)
    namespace Asset
    {
        class Model;
        struct AnimationAsset; // <-- Fwd declaration para o tipo Asset
    }
}

namespace Engine
{
    // (SRP: Mapeia dados GLTF brutos para as estruturas de Asset (Skeleton e AnimationAsset))
    class AnimationDataMapper {
    public:
        AnimationDataMapper() = delete;

        static std::unique_ptr<Skeleton> mapSkeleton(
            Engine::Asset::Model &model,
            const cgltf_data *gltf_data,
            const cgltf_skin *gltf_skin);

        static std::vector<std::unique_ptr<Engine::Asset::AnimationAsset>> mapAnimations(
            const cgltf_data* data, 
            Skeleton& skeleton
        );

    private:
        static void processBoneNode(
            Engine::Asset::Model &model,
            int nodeIndex,
            int parentId,
            const cgltf_data *gltf_data,
            std::map<std::string, int> &boneNameMap,
            std::vector<Bone> &bones);
        // Outras funções de utilidade para ler buffers virão no .cpp
    };

} // namespace Engine
