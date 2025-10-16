// engine/asset/animation_loader.cpp
#include "animation_loader.h"
#include "model.h"
#include "asset_manager.h"
#include "../core/log.h"
#include "../deps/cgltf/cgltf.h"
#include <glm/gtc/type_ptr.hpp> // Para glm::make_mat4
#include <glm/gtx/quaternion.hpp>

using namespace Engine::Asset;

// MOVIDO DE gltf_loader.cpp: Converte a transformação do nodo GLTF para uma matriz GLM
static glm::mat4 getGltfNodeTransform(const cgltf_node *node)
{
    glm::mat4 matrix = glm::mat4(1.0f);
    if (node->has_matrix)
    {
        matrix = glm::make_mat4(node->matrix);
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
    for (size_t i = 0; i < skin->joints_count; ++i)
    {
        const cgltf_node *joint = skin->joints[i];

        // Garante que o índice não exceda o vetor de IBMs.
        glm::mat4 ibm = (i < inverseBindMatrices.size()) ? inverseBindMatrices[i] : glm::mat4(1.0f);

        std::string boneName = joint->name ? joint->name : std::string("Bone_") + std::to_string(i);

        model.addBone(boneName, ibm);

        // 3. Define o nó raiz (o primeiro nó da lista de joints é frequentemente o raiz)
        if (i == 0)
        {
            model.setSkeletonRootName(boneName);
        }
    }

    // Retorna o índice do nó raiz (o primeiro joint)
    return skin->joints_count > 0 ? (skin->joints[0] - data->nodes) : -1;
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

    // 3. Processa filhos recursivamente
    for (cgltf_size i = 0; i < node->children_count; ++i)
    {
        traverseAndStoreHierarchy(node->children[i], data, model);
    }
}

// --------------------------------------------------------------------------------
// FUNÇÃO PRINCIPAL: processAnimationData
// --------------------------------------------------------------------------------
int AnimationLoader::processAnimationData(
    const cgltf_data *data,
    Engine::Asset::Model &model)
{
    // 1. Processa a estrutura de Skeleton/Bones
    int rootIndex = processGltfSkins(data, model);

    // 2. Traversa a hierarquia de nodos (a partir do root)
    if (rootIndex != -1)
    {
        // Pega o nó raiz do esqueleto
        const cgltf_node *rootNode = &data->nodes[rootIndex];
        traverseAndStoreHierarchy(rootNode, data, model);
    }

    // 3. NOVO: Processa os clipes de Keyframe (o novo SRP)
    processAnimationClips(data, model);

    return rootIndex;
}