// engine/asset/gltf_loader.cpp
#include "gltf_loader.h"
#include "model.h"
#include "./../../engine/core/log.h"
#include "./../../engine/core/path_utils.h"
#include "animation_loader.h" 
#include "gltf_data_reader.h" // NOVO: Para extrair os dados
#include <cgltf.h> 
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <stdexcept>
#include <format>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

namespace Engine {
namespace Asset {
    
    // Forward declaration (para a recursão)
    static void processGltfNode(
        const cgltf_node *gltfNode,
        const cgltf_data *data,
        Engine::Asset::Model &model,
        const std::string &baseDirectory);


    // =========================================================================
    // IMPLEMENTAÇÃO: processGltfNode (SRP: Travessia da Hierarquia da Cena)
    // =========================================================================

    static void processGltfNode(
        const cgltf_node *gltfNode,
        const cgltf_data *data,
        Engine::Asset::Model &model,
        const std::string &baseDirectory)
    {
        if (!gltfNode) return;

        // 1. Processar Malhas: Se o nó atual tem uma malha, delegue a leitura dos dados.
        if (gltfNode->mesh) 
        {
            const cgltf_mesh* gltfMesh = gltfNode->mesh;
            Engine::Core::Log::Debug(std::format("GLTFLoader: Processando malha '{}' do nó '{}'.",
                                            gltfMesh->name ? gltfMesh->name : "Sem Nome",
                                            gltfNode->name ? gltfNode->name : "Sem Nome"));

            for (cgltf_size j = 0; j < gltfMesh->primitives_count; ++j) {
                // AQUI: A responsabilidade de extração de dados é DELEGADA (SRP)
                GltfDataReader::loadPrimitive(&gltfMesh->primitives[j], model, baseDirectory); 
            }
        }

        // 2. Recursão: Processar filhos
        for (cgltf_size i = 0; i < gltfNode->children_count; ++i) {
            processGltfNode(gltfNode->children[i], data, model, baseDirectory); 
        }
    }


    // =========================================================================
    // FUNÇÃO PRINCIPAL: GLTFLoader::loadGLTF (SRP: Orquestração do Carregamento)
    // =========================================================================

    std::unique_ptr<Model> GLTFLoader::loadGLTF(const std::string &filePath)
    {
        Engine::Core::Log::Info(std::format("GLTFLoader: Tentando carregar modelo GLTF de '{}'", filePath));

        std::filesystem::path fullPath = Engine::resolveEnginePath(filePath);
        std::string baseDirectory = fullPath.parent_path().string();
        cgltf_options options = {0};
        cgltf_data *data = nullptr;

        cgltf_result result = cgltf_parse_file(&options, fullPath.string().c_str(), &data);
        if (result != cgltf_result_success) { Engine::Core::Log::Error("GLTFLoader: Erro ao parsear GLTF."); if (data) { cgltf_free(data); } throw std::runtime_error("GLTFLoader: Falha ao parsear GLTF."); }
        
        result = cgltf_load_buffers(&options, data, fullPath.string().c_str()); 
        if (result != cgltf_result_success) { Engine::Core::Log::Error("GLTFLoader: Erro ao carregar buffers GLTF."); cgltf_free(data); throw std::runtime_error("GLTFLoader: Falha ao carregar buffers GLTF."); }
        Engine::Core::Log::Debug("GLTFLoader: Carregamento de buffers binários GLTF concluído.");


        auto model = std::make_unique<Model>();

        // 1. Processar Esqueletos e Keyframes (SRP: Delegado ao AnimationLoader)
        int rootNodeIndex = AnimationLoader::processAnimationData(data, *model); 
        
        // 2. Traversa a Hierarquia da Cena (O loop principal)
        for (cgltf_size scene_idx = 0; scene_idx < data->scenes_count; ++scene_idx) {
            const cgltf_scene *gltfScene = &data->scenes[scene_idx];
            
            for (cgltf_size node_idx = 0; node_idx < gltfScene->nodes_count; ++node_idx) {
                const cgltf_node *gltfNode = gltfScene->nodes[node_idx];
                processGltfNode(gltfNode, data, *model, baseDirectory); // Chama o processador recursivo
            }
        }

        cgltf_free(data);

        Engine::Core::Log::Info(std::format("GLTFLoader: Carregamento de GLTF '{}' concluído. Total de malhas no modelo: {}.",
                                      filePath, model->getMeshes().size()));
        return model;
    }

} // namespace Asset
} // namespace Engine