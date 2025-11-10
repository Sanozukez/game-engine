// engine/asset/animation_data_mapper.cpp
//
// CORREÇÃO: Respeitando os namespaces (Engine:: vs Engine::Asset::)
// e salvando a Pose de Descanso (Nodes) no Model.

#include "animation_data_mapper.h"
#include "asset_manager.h"
#include "../core/log.h"
#include "../deps/cgltf/cgltf.h"
#include "../math/quat.h"

// Includes completos para TODOS os tipos que usamos
#include "skeleton.h"
#include "animation.h"
#include "model.h" // Necessário para Engine::Asset::Model e Engine::Asset::Node

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <unordered_set>
#include <algorithm>

// Usamos apenas o namespace Core para Log
using namespace Engine::Core;

// --------------------------------------------------------------------------------
// AUXILIARES
// --------------------------------------------------------------------------------

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

static bool IsRootBoneName(const std::string &name)
{
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    return lowerName.find("root") != std::string::npos || lowerName.find("pelvis") != std::string::npos;
}

// --------------------------------------------------------------------------------
// IMPLEMENTAÇÃO DO HELPER DE HIERARQUIA (VERSÃO CORRIGIDA E ROBUSTA)
// --------------------------------------------------------------------------------

// A definição agora corresponde ao .h (usa Engine::Asset::Model, Engine::Bone)
void Engine::AnimationDataMapper::processBoneNode(
    Engine::Asset::Model &model, // <-- Tipo Corrigido
    int nodeIndex,
    int parentId, // ID do 'pai' osso (propagado na recursão)
    const cgltf_data *gltf_data,
    std::map<std::string, int> &boneNameMap,
    std::vector<Engine::Bone> &bones // <-- Tipo Corrigido
)
{
    const cgltf_node *node = &gltf_data->nodes[nodeIndex];
    std::string boneName = node->name ? node->name : std::string("Node_") + std::to_string(nodeIndex);

    // --- LOG DE DEBUG (Mantido) ---
    //Core::Log::Info(std::format("[DEBUG_MAP] processBoneNode: Processando nó '{}' (ID: {}). ParentID recebido: {}", boneName, nodeIndex, parentId));

    int currentBoneId = -1; // ID deste nó (se for um osso)

    // 1. ESTE NÓ É UM OSSO?
    auto boneIt = boneNameMap.find(boneName);
    if (boneIt != boneNameMap.end())
    {
        // Sim, é um osso.
        currentBoneId = boneIt->second;
        Engine::Bone &bone = bones[currentBoneId];

        // Define o parentId do osso
        bone.parentId = parentId;

       // Core::Log::Info(std::format("[DEBUG_MAP] ...Nó '{}' É um osso (ID: {}). Definindo ParentId = {}", boneName, currentBoneId, parentId));
    }
    // (Se não for um osso, 'currentBoneId' permanece -1, e o 'parentId'
    // recebido será passado aos filhos, pulando este nó não-osso)


    // 2. SALVAR A POSE DE DESCANSO (Rest Pose)
    // Devemos salvar a pose de descanso de TODOS os nós na hierarquia,
    // não apenas ossos, para que 'getNodeLocalTransform' funcione corretamente.
    Engine::Asset::Node nodeData;
    nodeData.name = boneName;
    nodeData.localTransform = getGltfNodeTransform(node); // Usa o helper

    // --- LOG DE DEBUG (Mantido) ---
    // glm::vec3 restPos = glm::vec3(nodeData.localTransform[3]);
    // Core::Log::Info(std::format("[DEBUG_MAP] processBoneNode: SALVANDO Rest Pose para '{}'. Posição: ({:.2f}, {:.2f}, {:.2f})",
    //                             boneName, restPos.x, restPos.y, restPos.z));
    

    // 3. RECURSÃO (A LÓGICA CORRIGIDA)
    // Devemos recursar para TODOS os filhos, incondicionalmente.

    for (cgltf_size i = 0; i < node->children_count; ++i)
    {
        const cgltf_node *childNode = node->children[i];
        std::string childName = childNode->name ? childNode->name : std::string("Node_") + std::to_string(childNode - gltf_data->nodes);

        nodeData.childrenNames.push_back(childName);

        // Determina qual 'parentId' passar para o próximo nível:
        int nextParentId = (currentBoneId != -1) ? currentBoneId : parentId;

        // A recursão agora acontece FORA do 'if (childBoneIt ...)'
        processBoneNode(
            model,
            static_cast<int>(childNode - gltf_data->nodes),
            nextParentId, // Passa o ID do pai correto
            gltf_data,
            boneNameMap,
            bones);
    }

    // 4. PREENCHER 'childrenIds' (Separado da recursão)
    // (Isto é para a struct Bone, para referência futura, não afeta o bug atual)
    if (currentBoneId != -1)
    {
        Engine::Bone &bone = bones[currentBoneId];
        for (const auto& childName : nodeData.childrenNames)
        {
            auto childBoneIt = boneNameMap.find(childName);
            if (childBoneIt != boneNameMap.end()) {
                // O filho direto é um osso
                bone.childrenIds.push_back(childBoneIt->second);
            }
        }
    }

    // 5. SALVA O NÓ NO MODELO (APÓS A RECURSÃO)
    model.addNode(nodeData); // <-- SALVA O NÓ/BONE NO MODELO
}

// --------------------------------------------------------------------------------
// IMPLEMENTAÇÃO DO MAPPER DE ESQUELETO (mapSkeleton)
// --------------------------------------------------------------------------------

std::unique_ptr<Engine::Skeleton> Engine::AnimationDataMapper::mapSkeleton(
    Engine::Asset::Model &model, // <-- PARÂMETRO ADICIONADO
    const cgltf_data *data,
    const cgltf_skin *skin)
{
    if (!skin || skin->joints_count == 0)
    {
        return nullptr;
    }

    // Leitura de IBMs COM CORREÇÃO
    std::vector<glm::mat4> inverseBindMatrices;
    if (skin->inverse_bind_matrices)
    {
        const cgltf_accessor *ibm_accessor = skin->inverse_bind_matrices;
        inverseBindMatrices.resize(ibm_accessor->count);
        
        // ÚNICA CORREÇÃO QUE MANTÉM ESTRUTURA: X=180°
        glm::mat4 coordinateCorrection = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        
        for (cgltf_size i = 0; i < ibm_accessor->count; ++i)
        {
            float data_mat[16];
            if (cgltf_accessor_read_float(ibm_accessor, i, data_mat, 16))
            {
                glm::mat4 ibm_original = glm::make_mat4(data_mat);
                // Aplicar correção X=180° nas IBMs
                inverseBindMatrices[i] = ibm_original * glm::inverse(coordinateCorrection);
            }
            else
            {
                inverseBindMatrices[i] = glm::mat4(1.0f);
                Log::Error("[AnimDataMap] Falha ao ler IBM. Usando Identity.");
            }
        }
    }
    else
    {
        inverseBindMatrices.resize(skin->joints_count, glm::mat4(1.0f));
    }

    auto skeleton = std::make_unique<Engine::Skeleton>();
    skeleton->bones.resize(skin->joints_count);

    for (size_t i = 0; i < skin->joints_count; ++i)
    {
        const cgltf_node *joint = skin->joints[i];
        // A lógica de fallback DEVE ser idêntica à de gltf_loader.cpp
        int nodeIndex = static_cast<int>(joint - data->nodes);
        std::string boneName = joint->name ? joint->name : std::string("Node_") + std::to_string(nodeIndex);

        Engine::Bone &bone = skeleton->bones[i]; // <-- Tipo Corrigido
        bone.name = boneName;
        bone.id = static_cast<int>(i);
        bone.inverseBindMatrix = (i < inverseBindMatrices.size()) ? inverseBindMatrices[i] : glm::mat4(1.0f);

        skeleton->boneNameMap[boneName] = bone.id;
    }

    // ... (Lógica de encontrar Root ID)
    int detectedRootId = -1;
    auto rootIt = std::find_if(skeleton->bones.begin(), skeleton->bones.end(), [](const Engine::Bone &b)
                               { return IsRootBoneName(b.name); });
    if (rootIt != skeleton->bones.end())
    {
        detectedRootId = rootIt->id;
    }
    else if (skin->joints_count > 0)
    {
        detectedRootId = skeleton->bones[0].id;
    }
    skeleton->rootNodeId = detectedRootId;
    if (detectedRootId != -1)
    {
        Log::Info(std::format("[AnimDataMap] Nó Raiz do Esqueleto detectado: {}.", skeleton->bones[detectedRootId].name));
    }

    // 4. Conecta os Bones na Hierarquia (preferir skin->skeleton quando existir)
const cgltf_node* startNode = nullptr;

if (skin->skeleton) {
    // Comece pelo nó raiz declarado do skin (pode NÃO ser um osso)
    startNode = skin->skeleton;
    Core::Log::Info("[AnimDataMap] Usando skin->skeleton como raiz da hierarquia.");
} else if (skeleton->rootNodeId != -1) {
    // Fallback: algum joint “root” detectado
    startNode = skin->joints[skeleton->rootNodeId];
    Core::Log::Warn("[AnimDataMap] skin->skeleton ausente; usando joint detectado como raiz.");
} else {
    Core::Log::Error("[AnimDataMap] Falha: sem skin->skeleton e sem root joint. Hierarquia não pode ser construída.");
    return skeleton; // retorna com bones mas sem hierarquia
}

int startIndex = static_cast<int>(startNode - data->nodes);

// Inicia a recursão a partir do startNode
processBoneNode(
    model,
    startIndex,
    -1, // pai inicial
    data,
    skeleton->boneNameMap,
    skeleton->bones
);

// Garanta que, se há um joint “root” no mapeamento, ele tenha parent -1
if (skeleton->rootNodeId != -1) {
    skeleton->bones[skeleton->rootNodeId].parentId = -1;
}

    return skeleton;
}

std::vector<std::unique_ptr<Engine::Asset::AnimationAsset>> Engine::AnimationDataMapper::mapAnimations(const cgltf_data* data, Engine::Skeleton& skeleton)
{
    std::vector<std::unique_ptr<Engine::Asset::AnimationAsset>> animations;

    if (data->animations_count == 0)
    {
        return animations;
    }

    for (cgltf_size clip_idx = 0; clip_idx < data->animations_count; ++clip_idx)
    {
        const cgltf_animation *gltfAnim = &data->animations[clip_idx];
        auto clip = std::make_unique<Engine::Asset::AnimationAsset>(); // <-- CORREÇÃO
        clip->name = gltfAnim->name ? gltfAnim->name : std::string("DEFAULT_CLIP_") + std::to_string(clip_idx);
        float maxTime = 0.0f;

        for (cgltf_size i = 0; i < gltfAnim->channels_count; ++i)
        {
            const cgltf_animation_channel *channel = &gltfAnim->channels[i];
            const cgltf_animation_sampler *anim_sampler = channel->sampler;

            if (!anim_sampler || !channel->target_node || !anim_sampler->input || !anim_sampler->output)
                continue;

            // (A lógica de fallback do nome do osso que corrigimos permanece)
            int nodeIndex = static_cast<int>(channel->target_node - data->nodes);
            std::string boneName = channel->target_node->name ? channel->target_node->name : std::string("Node_") + std::to_string(nodeIndex);
            
            auto boneIt = skeleton.boneNameMap.find(boneName);
            if (boneIt == skeleton.boneNameMap.end())
            {
                 // (Log de aviso [DEBUG_MAP] FALHA... vai aqui)
                 Core::Log::Warn(std::format("[DEBUG_MAP] FALHA: Canal para '{}' não encontrado no boneNameMap. Canal de animação pulado.", boneName));
                 continue;
            }
            
            // (Log [DEBUG_MAP] SUCESSO... vai aqui)
            // Core::Log::Info(std::format("[DEBUG_MAP] SUCESSO: Canal para '{}' (ID: {}) encontrado. Carregando keyframes...", boneName, boneIt->second));

            Engine::Asset::AnimationChannel &boneChannel = clip->channels[boneName]; // <-- CORREÇÃO
            boneChannel.boneName = boneName;
            boneChannel.boneId = boneIt->second;

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

            const cgltf_accessor *outputAccessor = anim_sampler->output;

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
                
                // ÚNICA CORREÇÃO QUE MANTÉM ESTRUTURA: X=180°
                // Consequências: Inverte Y (de ponta cabeça) + Z (olhando pra trás)
                // Workarounds necessários em Transform + Controles
                glm::quat coordinateCorrection = glm::angleAxis(glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                
                for (size_t j = 0; j < keyTimes.size(); ++j)
                {
                    glm::vec4 value;
                    cgltf_accessor_read_float(outputAccessor, j, glm::value_ptr(value), 4);
                    
                    // GLTF stores as (x,y,z,w), convert to GLM (w,x,y,z)
                    glm::quat rot(value.w, value.x, value.y, value.z);
                    
                    // Aplicar correção X=180°
                    glm::quat correctedRot = coordinateCorrection * rot;
                    
                    boneChannel.rotationKeys.push_back(
                        {keyTimes[j], Engine::Math::Quat(correctedRot)});
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

        clip->duration = maxTime;
        animations.push_back(std::move(clip));
    }

    return animations;
}