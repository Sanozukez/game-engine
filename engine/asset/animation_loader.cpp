// engine/asset/animation_loader.cpp
#include "animation_loader.h"
#include "model.h"
#include "asset_manager.h"
#include "../core/log.h"
#include "../deps/cgltf/cgltf.h"
#include "../asset/animation_utils.h"
#include <glm/gtc/type_ptr.hpp> // Para glm::make_mat4
#include <glm/gtx/quaternion.hpp>
#include <unordered_set>

using namespace Engine::Asset;

// MOVIDO DE gltf_loader.cpp: Converte a transformação do nodo GLTF para uma matriz GLM
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

        // ANTES (errado): glm::make_quat(node->rotation)
        // DEPOIS (certo): glTF = (x,y,z,w)  → GLM quat(w,x,y,z)
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

// --------------------------------------------------------------------------------
// IMPLEMENTAÇÃO DE processGltfSkins (AGORA LÊ O BUFFER GLTF)
// --------------------------------------------------------------------------------
int AnimationLoader::processGltfSkins(
    const cgltf_data *data,
    Engine::Asset::Model &model)
{
    if (data->skins_count == 0)
    {
        return -1; // Sem skins
    }

    const cgltf_skin *skin = &data->skins[0];

    // ----------------------------------------------------------------
    // 1. LER AS TRANSFORMAÇÕES INVERSAS DE BIND (IBM) - LÓGICA CORRETA
    // ----------------------------------------------------------------
    std::vector<glm::mat4> inverseBindMatrices;

    // As matrizes de bind inversa (IBM) estão no acessor 'inverse_bind_matrices'
    if (skin->inverse_bind_matrices)
    {
        const cgltf_accessor *ibm_accessor = skin->inverse_bind_matrices;
        inverseBindMatrices.resize(ibm_accessor->count);

        for (cgltf_size i = 0; i < ibm_accessor->count; ++i)
        {
            float data_mat[16]; // Matriz 4x4 (16 floats)
            // Usa a API cgltf para ler os dados diretamente do buffer
            if (cgltf_accessor_read_float(ibm_accessor, i, data_mat, 16))
            {
                inverseBindMatrices[i] = glm::make_mat4(data_mat); // Converte array para glm::mat4
            }
            else
            {
                // Fallback de segurança
                inverseBindMatrices[i] = glm::mat4(1.0f);
                Engine::Core::Log::Error("[AnimLoader] Falha ao ler Matriz Inverse Bind (IBM). Usando Identity.");
            }
        }
    }
    else
    {
        // Se não houver IBM, o número de matrizes deve ser igual ao número de joints, todas Identity.
        inverseBindMatrices.resize(skin->joints_count, glm::mat4(1.0f));
    }

    // 2. PREENCHER O MODEL COM BONE INFO
    int rootNodeIndex = -1; // Índice GLTF do nó raiz.
    std::string detectedRootName = "";
    bool rootFound = false;

    for (size_t i = 0; i < skin->joints_count; ++i)
    {
        const cgltf_node *joint = skin->joints[i];

        glm::mat4 ibm = (i < inverseBindMatrices.size()) ? inverseBindMatrices[i] : glm::mat4(1.0f);

        std::string boneName = joint->name ? joint->name : std::string("Bone_") + std::to_string(i);

        model.addBone(boneName, ibm, static_cast<int>(i));

        // 3. NOVO: Priorizar o nó raiz pela nomenclatura, se ainda não encontrado.
        if (!rootFound && Engine::Asset::AnimationUtils::IsRootBoneName(boneName))
        {
            detectedRootName = boneName;
            rootNodeIndex = static_cast<int>(joint - data->nodes);
            rootFound = true; // Define o primeiro match como o Root Bone e para de procurar.
            // Não mais define model.setSkeletonRootName(boneName) aqui.
            // Fazemos isso no final para garantir que a lógica de fallback seja aplicada.
        }
    }

    // 4. Lógica de Fallback: Se a busca por "root" falhou, retorna à lógica original (o primeiro joint).
    if (!rootFound && skin->joints_count > 0)
    {
        const cgltf_node *firstJoint = skin->joints[0];
        detectedRootName = firstJoint->name ? firstJoint->name : std::string("Bone_0");
        rootNodeIndex = static_cast<int>(firstJoint - data->nodes);
    }

    // 5. Salvar o nome detectado no Model.
    if (!detectedRootName.empty())
    {
        model.setSkeletonRootName(detectedRootName);
        Engine::Core::Log::Info(std::format("[AnimLoader] Nó Raiz do Esqueleto definido como: {}.", detectedRootName));
    }
    // else { Log de erro se for vazio e o modelo tem joints. }
    {
        const auto &map = model.getBoneInfoMap();
        Engine::Core::Log::Info(std::format("[BONE_MAP] total={} (listando ate 64)", map.size()));
        int printed = 0;
        for (const auto &kv : map)
        {
            Engine::Core::Log::Info(std::format("  [BONE_MAP] name='{}' id={}", kv.first, kv.second.id));
            if (++printed >= 64)
                break;
        }
    }

    // Retorna o índice GLTF do nó raiz para a próxima função (traverseAndStoreHierarchy).
    return rootNodeIndex;
}

// --------------------------------------------------------------------------------
// IMPLEMENTAÇÃO DE processAnimationClips (Lê dados de Keyframe)
// --------------------------------------------------------------------------------
static void processAnimationClips(
    const cgltf_data *data,
    Engine::Asset::Model &model)
{
    if (data->animations_count == 0)
    {
        return;
    }

    // Apenas o primeiro clipe é processado por enquanto
    const cgltf_animation *gltfAnim = &data->animations[0];

    // Calcula o Hash ID (CORREÇÃO: Usando a qualificação completa)
    uint32_t clipID = gltfAnim->name
                          ? Engine::Asset::AssetManager::getAssetIDByName(gltfAnim->name)
                          : Engine::Asset::AssetManager::getAssetIDByName("DEFAULT_CLIP");

    // Cria o clipe de animação
    AnimationClip clip;

    float maxTime = 0.0f;

    // 1. Itera sobre os canais de animação
    for (cgltf_size i = 0; i < gltfAnim->channels_count; ++i)
    {
        // CORREÇÃO CRÍTICA: Usando cgltf_animation_channel (a struct correta que você encontrou)
        const cgltf_animation_channel *channel = &gltfAnim->channels[i]; // <--- CORREÇÃO AQUI

        // CORREÇÃO CRÍTICA: O sampler agora é o tipo correto (cgltf_animation_sampler)
        const cgltf_animation_sampler *anim_sampler = channel->sampler; // <--- CORREÇÃO AQUI

        if (!anim_sampler || !channel->target_node)
            continue;

        // Garante que os acessors existam (input/output)
        if (!anim_sampler->input || !anim_sampler->output)
            continue;

        std::string boneName = channel->target_node->name ? channel->target_node->name : "";
        if (boneName.empty() || !model.getBoneInfoMap().count(boneName))
            continue;

        // Garante que o BoneChannel exista no mapa do clipe
        BoneChannel &boneChannel = clip.boneChannels[boneName];
        boneChannel.boneName = boneName;

        // 2. LER OS TEMPOS (Inputs do Sampler)
        const cgltf_accessor *inputAccessor = anim_sampler->input;
        std::vector<float> keyTimes(inputAccessor->count);
        for (cgltf_size j = 0; j < inputAccessor->count; ++j)
        {
            cgltf_accessor_read_float(inputAccessor, j, &keyTimes[j], 1);
        }
        if (!keyTimes.empty())
        {
            maxTime = glm::max(maxTime, keyTimes.back());
        }

        // 3. LER OS VALORES (Outputs do Sampler)
        const cgltf_accessor *outputAccessor = anim_sampler->output;

        // Ação baseada no Tipo de Alvo (Translation, Rotation, Scale)
        if (channel->target_path == cgltf_animation_path_type_translation)
        {
            boneChannel.positionKeys.reserve(keyTimes.size());
            for (size_t j = 0; j < keyTimes.size(); ++j)
            {
                glm::vec3 value;
                cgltf_accessor_read_float(outputAccessor, j, glm::value_ptr(value), 3);
                boneChannel.positionKeys.push_back({keyTimes[j], value});
            }
        }
        else if (channel->target_path == cgltf_animation_path_type_rotation)
        {
            boneChannel.rotationKeys.reserve(keyTimes.size());
            for (size_t j = 0; j < keyTimes.size(); ++j)
            {
                glm::vec4 value;
                cgltf_accessor_read_float(outputAccessor, j, glm::value_ptr(value), 4);
                boneChannel.rotationKeys.push_back({keyTimes[j], Engine::Math::Quat(glm::quat(value.w, value.x, value.y, value.z))});
            }
        }
        else if (channel->target_path == cgltf_animation_path_type_scale)
        {
            boneChannel.scaleKeys.reserve(keyTimes.size());
            for (size_t j = 0; j < keyTimes.size(); ++j)
            {
                glm::vec3 value;
                cgltf_accessor_read_float(outputAccessor, j, glm::value_ptr(value), 3);
                boneChannel.scaleKeys.push_back({keyTimes[j], value});
            }
        }
    }

    // 4. Salvar o clipe no Model
    clip.duration = maxTime;
    model.addAnimationClip(clipID, std::move(clip));
}

// --------------------------------------------------------------------------------
// IMPLEMENTAÇÃO DE traverseAndStoreHierarchy
// --------------------------------------------------------------------------------
// Esta função é o SHELL para a lógica de árvore que o GLTFLoader originalmente faria.
void AnimationLoader::traverseAndStoreHierarchy(
    const cgltf_node *node,
    const cgltf_data *data,
    Engine::Asset::Model &model)
{
    if (!node)
        return;

    // 1. Cria o nodo local
    Node nodeData;
    nodeData.name = node->name ? node->name : std::string("Node_") + std::to_string(node - data->nodes);
    nodeData.localTransform = getGltfNodeTransform(node); // Usa a função auxiliar

    // 2. Coleta os nomes dos filhos e armazena
    for (cgltf_size i = 0; i < node->children_count; ++i)
    {
        nodeData.childrenNames.push_back(
            node->children[i]->name ? node->children[i]->name : std::string("Node_") + std::to_string(node->children[i] - data->nodes));
    }
    model.addNode(nodeData); // <--- ARMAZENA NO MODEL
    // ⬇️ LOG opcional: verificando se o hips/pelvis tem rest T≠0
    if (nodeData.name == std::string("spine") || nodeData.name == std::string("spine.001"))
    {
        glm::vec3 restT = glm::vec3(nodeData.localTransform[3]);
        Engine::Core::Log::Info(std::format(
            "[HIER_REST] node={} restT=({:.4f},{:.4f},{:.4f})",
            nodeData.name, restT.x, restT.y, restT.z));
    }

    // 3. Processa filhos recursivamente
    Engine::Core::Log::Info(std::format("[AnimLoader] Encontrado {} clipes de animação.", data->animations_count));
    for (cgltf_size i = 0; i < node->children_count; ++i)
    {
        traverseAndStoreHierarchy(node->children[i], data, model);
    }
}

// --------------------------------------------------------------------------------
// FUNÇÃO PRINCIPAL: processAnimationData
// --------------------------------------------------------------------------------
int AnimationLoader::processAnimationData(const cgltf_data *data, Engine::Asset::Model &model)
{
    // 1. Processa a estrutura básica de bones
    int rootIndex = processGltfSkins(data, model);

    // NOVO: capturar a matriz do skin->skeleton (se houver)
    glm::mat4 skeletonBind = glm::mat4(1.0f);
    const cgltf_skin *skin = (data->skins_count > 0) ? &data->skins[0] : nullptr;
    if (skin && skin->skeleton)
    {
        skeletonBind = getGltfNodeTransform(skin->skeleton);
        Engine::Core::Log::Info(
            std::format("[SKEL_BIND] skin.skeleton='{}' T=({:.4f},{:.4f},{:.4f})",
                        (skin->skeleton->name ? skin->skeleton->name : "(unnamed)"),
                        skeletonBind[3].x, skeletonBind[3].y, skeletonBind[3].z));
    }
    model.setSkeletonBindTransform(skeletonBind);

    // LOG: tamanho do mapa de ossos
    {
        const auto &map = model.getBoneInfoMap();
        Engine::Core::Log::Info(std::format("[BONE_MAP] total_bones={} skins_count={}", map.size(), data->skins_count));

        auto logIf = [&](const char *name)
        {
            if (map.count(name))
            {
                Engine::Core::Log::Info(std::format("[BONE_MAP] {} id={}", name, map.at(name).id));
            }
            else
            {
                Engine::Core::Log::Info(std::format("[BONE_MAP] {} NAO encontrado no BoneInfoMap", name));
            }
        };
        logIf("root");
        logIf("spine");
        logIf("spine.001");
        logIf("pelvis.L");
        logIf("pelvis.R");
    }

    // const cgltf_skin *skin = (data->skins_count > 0) ? &data->skins[0] : nullptr;

    // --- Nova abordagem: multi-root traversal ---
    if (skin)
    {
        // cria set com todos os joints do skin
        std::unordered_set<const cgltf_node *> jointSet;
        jointSet.reserve(skin->joints_count);
        for (cgltf_size i = 0; i < skin->joints_count; ++i)
            jointSet.insert(skin->joints[i]);

        // detecta possíveis roots (sem pai dentro do mesmo skin)
        std::vector<const cgltf_node *> skeletonRoots;
        for (cgltf_size i = 0; i < skin->joints_count; ++i)
        {
            const cgltf_node *j = skin->joints[i];
            const cgltf_node *parent = j->parent;

            bool parentIsJoint = parent && jointSet.count(parent);
            if (!parentIsJoint)
            {
                skeletonRoots.push_back(j);
                Engine::Core::Log::Info(std::format(
                    "[AnimLoader] Multi-root detectado: {}", j->name ? j->name : "(null)"));
            }
        }

        // percorre todos os roots encontrados
        for (const cgltf_node *r : skeletonRoots)
        {
            traverseAndStoreHierarchy(r, data, model);
        }
    }
    else
    {
        Engine::Core::Log::Warn("[AnimLoader] Nenhum skin encontrado no GLTF!");
    }

    // 2. Clipes de animação
    processAnimationClips(data, model);

    return rootIndex;
}