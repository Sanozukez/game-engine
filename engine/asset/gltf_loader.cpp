// engine/asset/gltf_loader.cpp
#include "gltf_loader.h"
#include "model.h"
#include "./../../engine/core/log.h"
#include "./../../engine/core/path_utils.h"
#include "asset_manager.h" // Necessário para AssetManager::Get()
#include "animation_data_mapper.h"
#include "gltf_data_reader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cgltf.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <stdexcept>
#include <format>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <functional>

namespace Engine
{
    namespace Asset
    {
        // =========================================================================
        // 1. FUNÇÕES AUXILIARES DE TRANSFORMAÇÃO (Helpers)
        // =========================================================================

        /**
         * @brief Matriz de correção do sistema de coordenadas do GLTF
         */
        static glm::mat4 getCoordinateSystemCorrection() {
            const float angleX = glm::radians(-180.0f);
            const float angleY = glm::radians(180.0f);
            glm::mat4 correction = glm::mat4(1.0f);
            correction = glm::rotate(correction, angleX, glm::vec3(1.0f, 0.0f, 0.0f));
            correction = glm::rotate(correction, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
            return correction;
        }

        /**
         * @brief Converte a transformação do nodo GLTF para uma matriz GLM (Usada para Skeleton Bind).
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
        // 2. IMPLEMENTAÇÃO: processGltfNode (Travessia e Carregamento de Mesh)
        // Definido aqui para ser visível a loadGLTF.
        // =========================================================================

        static void processGltfNode(
            const cgltf_node *gltfNode,
            const cgltf_data *data,
            Engine::Asset::Model &model,
            const std::string &baseDirectory,
            const glm::mat4 &parentGlobal // << NOVO
        )
        {
            if (!gltfNode)
                return;

            // IMPORTANTE: Local PURO sem correção (usado para animação/FK)
            glm::mat4 nodeLocal = getGltfNodeTransform(gltfNode);
            
            // Global COM correção (usado para renderização de meshes)
            glm::mat4 nodeGlobal = parentGlobal * nodeLocal;

            // ---- Salvar hierarquia (LOCAL PURO sem correção) ----
            Engine::Asset::Node nodeData;
            nodeData.name = gltfNode->name ? gltfNode->name
                                           : std::string("Node_") + std::to_string(gltfNode - data->nodes);
            nodeData.localTransform = nodeLocal;  // PURO, sem correção
            for (cgltf_size i = 0; i < gltfNode->children_count; ++i)
            {
                nodeData.childrenNames.push_back(
                    gltfNode->children[i]->name ? gltfNode->children[i]->name
                                                : std::string("Node_") + std::to_string(gltfNode->children[i] - data->nodes));
            }
            model.addNode(nodeData);

            // ---- Salvar GLOBAL do nó (será usado como uNode) ----
            model.setNodeGlobalTransform(nodeData.name, nodeGlobal);

            // ---- LOG Útil (1x por nó) ----
            Engine::Core::Log::Info(std::format(
                "[GLTF_NODE] '{}'  Tglob=({:.3f},{:.3f},{:.3f})",
                nodeData.name, nodeGlobal[3].x, nodeGlobal[3].y, nodeGlobal[3].z));

            // Malhas: passe o GLOBAL para a Mesh
            if (gltfNode->mesh)
            {
                const cgltf_mesh *gltfMesh = gltfNode->mesh;
                for (cgltf_size j = 0; j < gltfMesh->primitives_count; ++j)
                {
                    auto meshPtr = GltfDataReader::loadPrimitive(
                        &gltfMesh->primitives[j], model, baseDirectory, nodeGlobal /* << AQUI GLOBAL */
                    );
                    if (meshPtr)
                        model.addMesh(std::move(meshPtr));
                }
            }

            // Recursão com GLOBAL deste nó
            for (cgltf_size i = 0; i < gltfNode->children_count; ++i)
            {
                processGltfNode(gltfNode->children[i], data, model, baseDirectory, nodeGlobal);
            }
        }

        // =========================================================================
        // 3. FUNÇÃO PRINCIPAL: GLTFLoader::loadGLTF (Orquestração Corrigida)
        // =========================================================================

        std::unique_ptr<Model> GLTFLoader::loadGLTF(const std::string &filePath)
        {
            Engine::Core::Log::Info(std::format("GLTFLoader: Tentando carregar modelo GLTF de '{}'", filePath));

            std::filesystem::path fullPath = Engine::resolveEnginePath(filePath);
            std::string baseDirectory = fullPath.parent_path().string();
            cgltf_options options = {0};
            cgltf_data *data = nullptr;

            // 1. Parsear e Carregar Buffers
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

            // 2. CARREGAMENTO DE ANIMAÇÃO E ESQUELETO (USANDO O MAPPER)
            if (data->skins_count > 0)
            {
                const cgltf_skin *skin = &data->skins[0];
                auto skeleton = AnimationDataMapper::mapSkeleton(*model, data, skin);

                if (skeleton)
                {
                    model->setSkeleton(std::move(skeleton));
                    Skeleton *rawSkeleton = model->getSkeleton();
                    auto animations = AnimationDataMapper::mapAnimations(data, *rawSkeleton);

                    for (auto &anim : animations)
                    {
                        uint32_t clipID = AssetManager::Get().getAssetIDByName(anim->name);
                        model->addAnimation(clipID, std::move(anim));
                    }
                }

                // === PREENCHE jointIndex e inverseBindMatrix DE ACORDO COM skin.joints ===
                if (model->hasSkeleton() && skin)
                {
                    Engine::Skeleton *skel = model->getSkeleton();

                    // 3.1) jointIndex: mapeia por NOME do nó
                    for (cgltf_size j = 0; j < skin->joints_count; ++j)
                    {
                        const cgltf_node *jointNode = skin->joints[j];
                        if (!jointNode || !jointNode->name)
                            continue;

                        auto it = skel->boneNameMap.find(jointNode->name);
                        if (it != skel->boneNameMap.end())
                        {
                            const int boneId = it->second;
                            skel->bones[boneId].jointIndex = static_cast<int>(j);
                        }
                        else
                        {
                            Engine::Core::Log::Warn(std::format(
                                "[SKIN_MAP] joint '{}' não encontrado em boneNameMap. (Será ignorado / mantém fallback j=i).",
                                jointNode->name));
                        }
                    }

                    // 3.2) inverseBindMatrices: ler na MESMA ordem do skin.joints
                    // =====================================================================
                    // IMPORTANTE: leia o ELEMENTO INTEIRO (16 floats) de uma vez.
                    // cgltf_accessor_read_float(acc, j, dst, 16) → copia a matriz toda.
                    // =====================================================================
                    if (skin->inverse_bind_matrices && skin->inverse_bind_matrices->count == skin->joints_count)
                    {
                        cgltf_accessor *acc = skin->inverse_bind_matrices;

                        // Sanidade: o accessor deveria ser MAT4 (cgltf converte se não for)
                        if (acc->type != cgltf_type_mat4)
                        {
                            Engine::Core::Log::Warn("[SKIN_IBM] inverse_bind_matrices com type != MAT4; cgltf ainda converte, mas verifique export.");
                        }

                        std::vector<float> tmp(16);

                        for (cgltf_size j = 0; j < skin->joints_count; ++j)
                        {
                            // Lê a matriz inteira (16 floats) do elemento j
                            bool ok = cgltf_accessor_read_float(acc, j, tmp.data(), 16);
                            if (!ok)
                            {
                                Engine::Core::Log::Warn("[SKIN_IBM] Falha ao ler matriz inteira; usando Identity.");
                                for (int k = 0; k < 16; ++k)
                                    tmp[k] = (k % 5 == 0) ? 1.0f : 0.0f; // Identity
                            }

                            glm::mat4 ibm = glm::make_mat4(tmp.data()); // glTF é column-major → compatível

                            const cgltf_node *jointNode = skin->joints[j];
                            if (!jointNode || !jointNode->name)
                                continue;

                            auto it = skel->boneNameMap.find(jointNode->name);
                            if (it != skel->boneNameMap.end())
                            {
                                const int boneId = it->second;
                                skel->bones[boneId].inverseBindMatrix = ibm;
                                
                                // Log detalhado da IBM para os primeiros 3 bones
                                if (j < 3) {
                                    Engine::Core::Log::Info(std::format(
                                        "[IBM_DEBUG] Bone '{}' (ID:{}) IBM = \n"
                                        "  [{:.3f}, {:.3f}, {:.3f}, {:.3f}]\n"
                                        "  [{:.3f}, {:.3f}, {:.3f}, {:.3f}]\n"
                                        "  [{:.3f}, {:.3f}, {:.3f}, {:.3f}]\n"
                                        "  [{:.3f}, {:.3f}, {:.3f}, {:.3f}]",
                                        jointNode->name, boneId,
                                        ibm[0][0], ibm[0][1], ibm[0][2], ibm[0][3],
                                        ibm[1][0], ibm[1][1], ibm[1][2], ibm[1][3],
                                        ibm[2][0], ibm[2][1], ibm[2][2], ibm[2][3],
                                        ibm[3][0], ibm[3][1], ibm[3][2], ibm[3][3]));
                                }
                            }
                        }
                    }
                    else
                    {
                        // =================================================================
                        // FALLBACK ROBUSTO: não há accessor de IBM → IBM = inverse(globalRest)
                        // diretamente do grafo do glTF (subindo pais).
                        // =================================================================
                        // Local puro, sem correção (seria aplicada depois no nível da cena)
                        auto getNodeLocal = [](const cgltf_node *n) -> glm::mat4
                        {
                            glm::mat4 matrix = glm::mat4(1.0f);
                            if (!n) return matrix;

                            if (n->has_matrix)
                            {
                                matrix = glm::make_mat4(n->matrix);
                            }
                            else
                            {
                                glm::vec3 T = n->has_translation ? glm::make_vec3(n->translation) : glm::vec3(0.0f);
                                glm::quat R = n->has_rotation
                                                ? glm::quat(n->rotation[3], n->rotation[0], n->rotation[1], n->rotation[2])
                                                : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                                glm::vec3 S = n->has_scale ? glm::make_vec3(n->scale) : glm::vec3(1.0f);

                                matrix = glm::translate(glm::mat4(1.0f), T);
                                matrix *= glm::mat4_cast(R);
                                matrix = glm::scale(matrix, S);
                            }
                            return matrix;
                        };

                        std::function<glm::mat4(const cgltf_node *)> getNodeGlobal =
                            [&](const cgltf_node *n) -> glm::mat4
                        {
                            if (!n)
                                return glm::mat4(1.0f);
                            glm::mat4 g = getNodeLocal(n);
                            const cgltf_node *p = n->parent;
                            while (p)
                            {
                                g = getNodeLocal(p) * g;
                                p = p->parent;
                            }
                            return g;
                        };

                        for (cgltf_size j = 0; j < skin->joints_count; ++j)
                        {
                            const cgltf_node *jointNode = skin->joints[j];
                            if (!jointNode || !jointNode->name)
                                continue;

                            auto it = skel->boneNameMap.find(jointNode->name);
                            if (it == skel->boneNameMap.end())
                                continue;

                            const int boneId = it->second;
                            glm::mat4 globalRest = getNodeGlobal(jointNode);
                            glm::mat4 ibm = glm::inverse(globalRest);
                            skel->bones[boneId].inverseBindMatrix = ibm;
                        }

                        Engine::Core::Log::Warn("[SKIN_IBM] inverse_bind_matrices ausente/invalid → IBMs calculadas de globalRest (fallback).");
                    }

                    // 3.3) Log rápido (só os 5 primeiros para confirmação)
                    int printed = 0;
                    for (size_t i = 0; i < skel->bones.size() && printed < 5; ++i, ++printed)
                    {
                        const auto &b = skel->bones[i];
                        Engine::Core::Log::Info(std::format("[SKIN_ORDER] boneId={} name='{}' jointIndex={}",
                                                            b.id, b.name, b.jointIndex));
                    }
                }

                glm::mat4 skeletonBind = glm::mat4(1.0f);
                if (skin->skeleton)
                {
                    skeletonBind = getGltfNodeTransform(skin->skeleton);
                    model->setSkeletonBindTransform(skeletonBind);
                }
            }

            // 3. TRAVESSIA DA CENA E CARREGAMENTO DE MALHAS
            if (data->scene)
            {
                // NÃO aplicamos correção aqui - será feita no uModel do renderer
                glm::mat4 identityMatrix = glm::mat4(1.0f);

                // Processa todos os nós raiz da cena SEM correção
                for (cgltf_size i = 0; i < data->scene->nodes_count; ++i)
                {
                    processGltfNode(
                        data->scene->nodes[i],
                        data,
                        *model,
                        baseDirectory,
                        identityMatrix // Inicia SEM correção
                    );
                }
            }
            else
            {
                Engine::Core::Log::Warn("GLTFLoader: Nenhum nó raiz de cena encontrado. Malhas não serão carregadas.");
            }

            // 4. LIMPEZA E RETORNO
            cgltf_free(data);

            Engine::Core::Log::Info(std::format("GLTFLoader: Carregamento de GLTF '{}' concluído. Total de malhas no modelo: {}.",
                                                filePath, model->getMeshes().size()));
            return model;
        }

    } // namespace Asset
} // namespace Engine