// engine/asset/gltf_loader.cpp
#include "gltf_loader.h"
#include "model.h"
#include "./../../engine/core/log.h"
#include "./../../engine/core/path_utils.h"
#include "animation_loader.h"
#include "gltf_data_reader.h"           // NOVO: Para extrair os dados
#include <glm/gtc/matrix_transform.hpp> // <-- necessário
#include <glm/gtc/quaternion.hpp>       // <-- necessário
#include <cgltf.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <stdexcept>
#include <format>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

namespace Engine
{
    namespace Asset
    {

        // Forward declaration (para a recursão)
        static void processGltfNode(
            const cgltf_node *gltfNode,
            const cgltf_data *data,
            Engine::Asset::Model &model,
            const std::string &baseDirectory);

        // =========================================================================
        // IMPLEMENTAÇÃO: processGltfNode (SRP: Travessia da Hierarquia da Cena)
        // =========================================================================

        // helper LOCAL (mesma lógica do seu getGltfNodeTransform do animation_loader)
        static glm::mat4 getGltfNodeTransformLocal(const cgltf_node *node)
        {
            glm::mat4 M(1.0f);
            if (!node)
                return M;

            if (node->has_matrix)
            {
                return glm::make_mat4(node->matrix);
            }

            glm::vec3 T = node->has_translation ? glm::make_vec3(node->translation) : glm::vec3(0.0f);
            glm::quat R = node->has_rotation
                              ? glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]) // glTF (x,y,z,w) → GLM (w,x,y,z)
                              : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 S = node->has_scale ? glm::make_vec3(node->scale) : glm::vec3(1.0f);

            M = glm::translate(glm::mat4(1.0f), T);
            M *= glm::mat4_cast(R);
            M = glm::scale(M, S);
            return M;
        }

        static void processGltfNode(
            const cgltf_node *gltfNode,
            const cgltf_data *data,
            Engine::Asset::Model &model,
            const std::string &baseDirectory)
        {
            if (!gltfNode)
                return;

            glm::mat4 nodeLocal = getGltfNodeTransformLocal(gltfNode);

            // 1. Processar Malhas: Se o nó atual tem uma malha, delegue a leitura dos dados.
            if (gltfNode->mesh)
            {
                const cgltf_mesh *gltfMesh = gltfNode->mesh;
                Engine::Core::Log::Debug(std::format("GLTFLoader: Processando malha '{}' do nó '{}'.",
                                                     gltfMesh->name ? gltfMesh->name : "Sem Nome",
                                                     gltfNode->name ? gltfNode->name : "Sem Nome"));

                for (cgltf_size j = 0; j < gltfMesh->primitives_count; ++j)
                {
                    // ALTERAR: loadPrimitive agora retorna std::unique_ptr<Mesh> e recebe nodeLocal
                    auto meshPtr = GltfDataReader::loadPrimitive(&gltfMesh->primitives[j], model, baseDirectory, nodeLocal);
                    if (meshPtr)
                    {
                        model.addMesh(std::move(meshPtr));
                    }
                }
            }

            // 2. Recursão: Processar filhos
            for (cgltf_size i = 0; i < gltfNode->children_count; ++i)
            {
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
            if (result != cgltf_result_success)
            {
                Engine::Core::Log::Error("GLTFLoader: Erro ao parsear GLTF.");
                if (data)
                {
                    cgltf_free(data);
                }
                throw std::runtime_error("GLTFLoader: Falha ao parsear GLTF.");
            }

            result = cgltf_load_buffers(&options, data, fullPath.string().c_str());
            if (result != cgltf_result_success)
            {
                Engine::Core::Log::Error("GLTFLoader: Erro ao carregar buffers GLTF.");
                cgltf_free(data);
                throw std::runtime_error("GLTFLoader: Falha ao carregar buffers GLTF.");
            }
            Engine::Core::Log::Debug("GLTFLoader: Carregamento de buffers binários GLTF concluído.");

            auto model = std::make_unique<Model>();

            // 1. Processar Esqueletos e Keyframes (SRP: Delegado ao AnimationLoader)
            int rootNodeIndex = AnimationLoader::processAnimationData(data, *model);

            // 2. Traversa a Hierarquia da Cena (O loop principal)
            for (cgltf_size scene_idx = 0; scene_idx < data->scenes_count; ++scene_idx)
            {
                const cgltf_scene *gltfScene = &data->scenes[scene_idx];

                for (cgltf_size node_idx = 0; node_idx < gltfScene->nodes_count; ++node_idx)
                {
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