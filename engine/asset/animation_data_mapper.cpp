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
// IMPLEMENTAÇÃO DO HELPER DE HIERARQUIA
// --------------------------------------------------------------------------------

// A definição agora corresponde ao .h (usa Engine::Asset::Model, Engine::Bone)
void Engine::AnimationDataMapper::processBoneNode(
    Engine::Asset::Model& model, // <-- Tipo Corrigido
    int nodeIndex, 
    int parentId,
    const cgltf_data* gltf_data,
    std::map<std::string, int>& boneNameMap, 
    std::vector<Engine::Bone>& bones // <-- Tipo Corrigido
) {
    const cgltf_node* node = &gltf_data->nodes[nodeIndex];
    std::string boneName = node->name ? node->name : std::string("Node_") + std::to_string(nodeIndex);

    auto boneIt = boneNameMap.find(boneName);
    if (boneIt != boneNameMap.end()) 
    {
        int boneId = boneIt->second;
        Engine::Bone& bone = bones[boneId]; // <-- Tipo Corrigido
        
        bone.parentId = parentId;

        // --- CORREÇÃO: SALVAR A POSE DE DESCANSO (Rest Pose) ---
        Engine::Asset::Node nodeData; // <-- Usar o tipo Node (do model.h)
        nodeData.name = boneName;
        nodeData.localTransform = getGltfNodeTransform(node); // Usa o helper
        
        for (cgltf_size i = 0; i < node->children_count; ++i) 
        {
            const cgltf_node* childNode = node->children[i];
            std::string childName = childNode->name ? childNode->name : std::string("Node_") + std::to_string(childNode - gltf_data->nodes);
            
            nodeData.childrenNames.push_back(childName);
            
            auto childBoneIt = boneNameMap.find(childName);
            if (childBoneIt != boneNameMap.end()) {
                 bone.childrenIds.push_back(childBoneIt->second); 
                 
                 processBoneNode(
                    model, // <-- Passa o modelo
                    static_cast<int>(childNode - gltf_data->nodes), 
                    boneId, 
                    gltf_data, 
                    boneNameMap, 
                    bones
                );
            }
        }
        
        model.addNode(nodeData); // <-- SALVA O NÓ/BONE NO MODELO
        // --- FIM DA CORREÇÃO ---
    }
}


// --------------------------------------------------------------------------------
// IMPLEMENTAÇÃO DO MAPPER DE ESQUELETO (mapSkeleton)
// --------------------------------------------------------------------------------

std::unique_ptr<Engine::Skeleton> Engine::AnimationDataMapper::mapSkeleton(
    Engine::Asset::Model& model, // <-- PARÂMETRO ADICIONADO
    const cgltf_data* data, 
    const cgltf_skin* skin
)
{
    if (!skin || skin->joints_count == 0) {
        return nullptr;
    }
    
    // ... (Lógica de leitura de IBMs)
    std::vector<glm::mat4> inverseBindMatrices;
    if (skin->inverse_bind_matrices) {
        const cgltf_accessor *ibm_accessor = skin->inverse_bind_matrices;
        inverseBindMatrices.resize(ibm_accessor->count);
        for (cgltf_size i = 0; i < ibm_accessor->count; ++i)
        {
            float data_mat[16]; 
            if (cgltf_accessor_read_float(ibm_accessor, i, data_mat, 16))
            {
                inverseBindMatrices[i] = glm::make_mat4(data_mat);
            }
            else
            {
                inverseBindMatrices[i] = glm::mat4(1.0f);
                Log::Error("[AnimDataMap] Falha ao ler Matriz Inverse Bind (IBM). Usando Identity.");
            }
        }
    }
    else {
        inverseBindMatrices.resize(skin->joints_count, glm::mat4(1.0f));
    }

    auto skeleton = std::make_unique<Engine::Skeleton>();
    skeleton->bones.resize(skin->joints_count);
    
    for (size_t i = 0; i < skin->joints_count; ++i)
    {
        const cgltf_node *joint = skin->joints[i];
        std::string boneName = joint->name ? joint->name : std::string("Bone_") + std::to_string(i);

        Engine::Bone& bone = skeleton->bones[i]; // <-- Tipo Corrigido
        bone.name = boneName;
        bone.id = static_cast<int>(i);
        bone.inverseBindMatrix = (i < inverseBindMatrices.size()) ? inverseBindMatrices[i] : glm::mat4(1.0f);
        
        skeleton->boneNameMap[boneName] = bone.id;
    }

    // ... (Lógica de encontrar Root ID)
    int detectedRootId = -1;
    auto rootIt = std::find_if(skeleton->bones.begin(), skeleton->bones.end(), [](const Engine::Bone& b) {
        return IsRootBoneName(b.name);
    });
    if (rootIt != skeleton->bones.end()) {
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
    
    // 4. Conecta os Bones na Hierarquia (Multi-Root Traversal)
    std::unordered_set<const cgltf_node *> jointSet;
    jointSet.reserve(skin->joints_count);
    for (cgltf_size i = 0; i < skin->joints_count; ++i)
        jointSet.insert(skin->joints[i]);

    std::vector<const cgltf_node *> skeletonRoots;
    
    for (cgltf_size i = 0; i < skin->joints_count; ++i)
    {
        const cgltf_node *j = skin->joints[i];
        const cgltf_node *parent = j->parent;
        if (!parent || !jointSet.count(parent))
        {
            skeletonRoots.push_back(j);
        }
    }
    
    for (const cgltf_node *rootNode : skeletonRoots)
    {
        int rootNodeIndex = static_cast<int>(rootNode - data->nodes);
        auto rootBoneIt = skeleton->boneNameMap.find(rootNode->name);
        if (rootBoneIt != skeleton->boneNameMap.end()) {
            // CHAMADA CORRIGIDA (Passando o model)
            processBoneNode(
                model, 
                rootNodeIndex, 
                -1, 
                data, 
                skeleton->boneNameMap, 
                skeleton->bones
            );
        } 
    }
    
    if (skeleton->rootNodeId != -1) {
        skeleton->bones[skeleton->rootNodeId].parentId = -1;
    }

    return skeleton;
}


// --------------------------------------------------------------------------------
// IMPLEMENTAÇÃO DO MAPPER DE ANIMAÇÃO (mapAnimations)
// --------------------------------------------------------------------------------

std::vector<std::unique_ptr<Engine::Animation>> Engine::AnimationDataMapper::mapAnimations(const cgltf_data* data, Engine::Skeleton& skeleton)
{
    std::vector<std::unique_ptr<Engine::Animation>> animations;

    if (data->animations_count == 0)
    {
        return animations;
    }

    for (cgltf_size clip_idx = 0; clip_idx < data->animations_count; ++clip_idx)
    {
        const cgltf_animation *gltfAnim = &data->animations[clip_idx];
        auto clip = std::make_unique<Engine::Animation>();
        clip->name = gltfAnim->name ? gltfAnim->name : std::string("DEFAULT_CLIP_") + std::to_string(clip_idx);
        float maxTime = 0.0f;

        for (cgltf_size i = 0; i < gltfAnim->channels_count; ++i)
        {
            const cgltf_animation_channel *channel = &gltfAnim->channels[i];
            const cgltf_animation_sampler *anim_sampler = channel->sampler;

            if (!anim_sampler || !channel->target_node || !anim_sampler->input || !anim_sampler->output)
                continue;

            std::string boneName = channel->target_node->name ? channel->target_node->name : "";
            auto boneIt = skeleton.boneNameMap.find(boneName);
            if (boneName.empty() || boneIt == skeleton.boneNameMap.end())
                continue;

            Engine::BoneChannel &boneChannel = clip->channels[boneName];
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
                for (size_t j = 0; j < keyTimes.size(); ++j)
                {
                    glm::vec4 value;
                    cgltf_accessor_read_float(outputAccessor, j, glm::value_ptr(value), 4);
                    boneChannel.rotationKeys.push_back(
                        {keyTimes[j], Engine::Math::Quat(glm::quat(value.w, value.x, value.y, value.z))}
                    );
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