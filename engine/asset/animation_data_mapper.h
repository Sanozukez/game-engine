// engine/asset/animation_data_mapper.h
#pragma once

#include "skeleton.h"
#include "animation.h"
#include <vector>
#include <memory>
// Forward declarations da dependência externa (cgltf)
struct cgltf_data;
struct cgltf_skin;

namespace Engine
{
    namespace Asset { class Model; }
    

        /**
         * @brief Mapeia dados brutos de animação (ex: cgltf) para as estruturas da Engine (Skeleton/Animation).
         * SRP: Responsabilidade única de tradução de formatos.
         */
        class AnimationDataMapper
        {
        public:
            AnimationDataMapper() = delete;

            static std::unique_ptr<Skeleton> mapSkeleton(
            Engine::Asset::Model& model, // <-- ADICIONAR ESTE PARÂMETRO
            const cgltf_data* gltf_data, 
            const cgltf_skin* gltf_skin
        );

            static std::vector<std::unique_ptr<Animation>> mapAnimations(const cgltf_data *gltf_data, Skeleton &skeleton);

        private:
            static void processBoneNode(
                Engine::Asset::Model& model,
                int nodeIndex,
                int parentId,
                const cgltf_data *gltf_data,
                std::map<std::string, int> &boneNameMap,
                std::vector<Bone>& bones);
            // Outras funções de utilidade para ler buffers virão no .cpp
        };

    } // namespace Engine
