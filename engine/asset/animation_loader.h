// engine/asset/animation_loader.h
#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../deps/cgltf/cgltf.h" // Dependência GLTF

namespace Engine::Asset
{
    class Model; // Forward declaration

    // Este módulo terá a responsabilidade única (SRP) de extrair dados
    // de animação (Skeleton, Keyframes) do formato GLTF.
    class AnimationLoader
    {
    public:
        // Função principal que carrega os dados de animação/esqueleto e preenche o Model.
        // Retorna o índice do nó raiz do esqueleto, se encontrado.
        static int processAnimationData(
            const cgltf_data *data,
            Engine::Asset::Model &model);

    private:
        // Processa as skins (esqueletos) e preenche o Model::m_boneInfoMap.
        static int processGltfSkins(
            const cgltf_data *data,
            Engine::Asset::Model &model);

        // NOVO: Função auxiliar para travar a hierarquia de nodos GLTF
        // Esta é a função que estava faltando.
        static void traverseAndStoreHierarchy(
            const cgltf_node *node,
            const cgltf_data *data,
            Engine::Asset::Model &model);
    };
}