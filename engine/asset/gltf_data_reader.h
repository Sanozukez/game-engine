// engine/asset/gltf_data_reader.h
#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "../deps/cgltf/cgltf.h"
#include "model.h"

namespace Engine {
namespace Render { class Material; class Texture; }

namespace Asset {

// Este novo módulo tem a responsabilidade ÚNICA (SRP) de extrair os dados
// brutos de uma primitiva GLTF (vértices, índices, materiais).
class GltfDataReader
{
public:
    // Função principal que carrega os dados de uma primitiva e a adiciona ao Model.
    static void loadPrimitive(
        const cgltf_primitive *gltfPrimitive,
        Engine::Asset::Model &model,
        const std::string &baseDirectory);
        
    // Função auxiliar para carregar texturas embedadas ou externas
    static std::unique_ptr<Render::Texture> loadGltfTexture(
        const cgltf_texture* gltfTexture, 
        const std::string& baseDirectory);
};

} // namespace Asset
} // namespace Engine