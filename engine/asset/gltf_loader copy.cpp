// engine/asset/gltf_loader.cpp
#include "gltf_loader.h"
#include "model.h"
#include "./../../engine/core/log.h"
#include "./../../engine/core/path_utils.h"
#include "animation_data_mapper.h"
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

        /**
         * @brief Converte a transformação do nodo GLTF para uma matriz GLM.
         */
        static glm::mat4 getGltfNodeTransform(const cgltf_node *node)
        {
            glm::mat4 matrix = glm::mat4(1.0f);
            if (node->has_matrix)
            {
                matrix = glm::make_mat4(node->matrix);
            }
            else
            {
                glm::vec3 T = node->has_translation ? glm::make_vec3(node->translation) : glm::vec3(0.0f);

                // CORREÇÃO: glTF = (x,y,z,w)  → GLM quat(w,x,y,z)
                glm::quat R = node->has_rotation
                                  ? glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2])
                                  : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

                glm::vec3 S = node->has_scale ? glm::make_vec3(node->scale) : glm::vec3(1.0f);

                matrix = glm::translate(glm::mat4(1.0f), T);
                matrix *= glm::mat4_cast(R);
                matrix = glm::scale(matrix, S);
            }
            return matrix;
        }

        // =========================================================================
        // FUNÇÃO PRINCIPAL: GLTFLoader::loadGLTF (CORRIGIDA)
        // =========================================================================

        std::unique_ptr<Model> GLTFLoader::loadGLTF(const std::string &filePath)
        {
            Engine::Core::Log::Info(std::format("GLTFLoader: Tentando carregar modelo GLTF de '{}'", filePath));

            std::filesystem::path fullPath = Engine::resolveEnginePath(filePath);
            std::string baseDirectory = fullPath.parent_path().string();
            cgltf_options options = {0};
            cgltf_data *data = nullptr;

            // 1. Parsear e Carregar Buffers (MANTIDO)
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

            // -----------------------------------------------------------------------
            // 2. CARREGAMENTO DE ANIMAÇÃO E ESQUELETO (RE-INSERIDO AQUI)
            // -----------------------------------------------------------------------
            if (data->skins_count > 0)
            {
                const cgltf_skin *skin = &data->skins[0];
                auto skeleton = AnimationDataMapper::mapSkeleton(data, skin);

                if (skeleton)
                {
                    model->setSkeleton(std::move(skeleton));

                    // Carregar os clipes de animação
                    Skeleton *rawSkeleton = model->getSkeleton();
                    auto animations = AnimationDataMapper::mapAnimations(data, *rawSkeleton);

                    for (auto &anim : animations)
                    {
                        uint32_t clipID = Engine::Asset::AssetManager::Get().getAssetIDByName(anim->name);
                        model->addAnimation(clipID, std::move(anim));
                    }
                }

                // Capturar a matriz skin->skeleton (Bind Transform)
                glm::mat4 skeletonBind = glm::mat4(1.0f);
                if (skin->skeleton)
                {
                    skeletonBind = getGltfNodeTransform(skin->skeleton);
                    model->setSkeletonBindTransform(skeletonBind);
                }
            }

            // -----------------------------------------------------------------------
            // 3. TRAVESSIA DA CENA E CARREGAMENTO DE MALHAS (RE-INSERIDO AQUI)
            // -----------------------------------------------------------------------
            if (data->scene) // Assume a cena default existe
            {
                for (cgltf_size i = 0; i < data->scene->nodes_count; ++i)
                {
                    processGltfNode(
                        data->scene->nodes[i],
                        data,
                        *model,
                        baseDirectory);
                }
            }
            else
            {
                Engine::Core::Log::Warn("GLTFLoader: Nenhum nó raiz de cena encontrado. Malhas não serão carregadas.");
            }

            // -----------------------------------------------------------------------
            // 4. LIMPEZA E RETORNO (POSICIONADO CORRETAMENTE)
            // -----------------------------------------------------------------------
            cgltf_free(data); // <--- AGORA É SEGURO LIBERAR A MEMÓRIA

            Engine::Core::Log::Info(std::format("GLTFLoader: Carregamento de GLTF '{}' concluído. Total de malhas no modelo: {}.",
                                                filePath, model->getMeshes().size()));
            return model;
        }

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

            cgltf_free(data);

            Engine::Core::Log::Info(std::format("GLTFLoader: Carregamento de GLTF '{}' concluído. Total de malhas no modelo: {}.",
                                                filePath, model->getMeshes().size()));
            return model;
        }

    } // namespace Asset
} // namespace Engine