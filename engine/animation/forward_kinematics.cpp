// engine/animation/forward_kinematics.cpp

#include "forward_kinematics.h"
#include "pose_utils.h"
#include "../core/log.h"
#include <format>

namespace Engine::Animation {

void ForwardKinematics::computeGlobalTransforms(
    const Engine::Skeleton* skeleton,
    const std::map<int, glm::mat4>& localTransforms,
    std::vector<glm::mat4>& outGlobalTransforms)
{
    if (!skeleton) {
        return;
    }

    const size_t numBones = skeleton->bones.size();
    if (numBones == 0) {
        return;
    }

    // Inicializa todas as transformações com as locais
    outGlobalTransforms.clear();
    outGlobalTransforms.resize(numBones);
    
    // Aplicar transformações locais
    for (size_t i = 0; i < numBones; ++i) {
        const auto& bone = skeleton->bones[i];
        outGlobalTransforms[i] = localTransforms.at(bone.id);
    }

    // Processa os ossos em ordem de hierarquia, dos pais para os filhos
    std::vector<bool> processed(numBones, false);
    
    // Primeiro processa o root
    if (skeleton->rootNodeId >= 0) {
        processed[skeleton->rootNodeId] = true;
    }

    // Continua processando até que todos os ossos estejam processados
    bool madePrgress;
    do {
        madePrgress = false;
        for (size_t i = 0; i < numBones; ++i) {
            if (processed[i]) continue;

            const auto& bone = skeleton->bones[i];
            if (bone.parentId >= 0 && bone.parentId < static_cast<int>(numBones)) {
                if (processed[bone.parentId]) {
                    // Log debug para os primeiros 3 bones
                    if (i < 3) {
                        glm::vec3 parentT = glm::vec3(outGlobalTransforms[bone.parentId][3]);
                        glm::vec3 localT = glm::vec3(outGlobalTransforms[i][3]);
                        
                        // Log the rotation component (Y-axis vector) of the local transform
                        glm::vec3 localYAxis = glm::vec3(outGlobalTransforms[i][1]);
                        
                        // Engine::Core::Log::Info(std::format(
                        //     "[FK_DEBUG] Bone {} ({}): Parent T=({:.3f},{:.3f},{:.3f}), Local T=({:.3f},{:.3f},{:.3f}), Local Y-axis=({:.3f},{:.3f},{:.3f})",
                        //     i, bone.name, parentT.x, parentT.y, parentT.z, localT.x, localT.y, localT.z, localYAxis.x, localYAxis.y, localYAxis.z));
                    }
                    
                    // Só processa se o pai já foi processado
                    outGlobalTransforms[i] = outGlobalTransforms[bone.parentId] * outGlobalTransforms[i];
                    processed[i] = true;
                    madePrgress = true;
                    
                    // Log resultado
                    // if (i < 3) {
                    //     glm::vec3 resultT = glm::vec3(outGlobalTransforms[i][3]);
                    //     Engine::Core::Log::Info(std::format(
                    //         "[FK_DEBUG] Bone {} ({}) Result T=({:.3f},{:.3f},{:.3f})",
                    //         i, bone.name, resultT.x, resultT.y, resultT.z));
                    // }
                }
            }
        }
    } while (madePrgress);

    // Log de debug para a transformação da raiz
    // if (skeleton->rootNodeId >= 0 && skeleton->rootNodeId < static_cast<int>(numBones)) {
    //     const int rootId = skeleton->rootNodeId;
    //     const glm::vec3 rootTranslation = glm::vec3(outGlobalTransforms[rootId][3]);
    //     Engine::Core::Log::Info(std::format(
    //         "[FK] Root bone '{}' global position: ({:.3f}, {:.3f}, {:.3f})",
    //         skeleton->bones[rootId].name, 
    //         rootTranslation.x, 
    //         rootTranslation.y, 
    //         rootTranslation.z));
    // }
}

void ForwardKinematics::initializeRootTransforms(
    const Engine::Skeleton* skeleton,
    const std::map<int, glm::mat4>& localTransforms,
    std::vector<glm::mat4>& globalTransforms)
{
    const size_t numBones = skeleton->bones.size();
    
    for (size_t i = 0; i < numBones; ++i) {
        const auto& bone = skeleton->bones[i];
        // Se é um osso raiz (sem pai ou pai inválido)
        if (bone.parentId < 0 || bone.parentId >= static_cast<int>(numBones)) {
            // Usa a transformação local diretamente como global
            globalTransforms[i] = localTransforms.at(bone.id);
        }
    }
}

} // namespace Engine::Animation