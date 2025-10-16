// // engine/asset/animation_utils.h
#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <memory>
#include "./model.h" // Precisamos do Model para acessar os dados do GLTF

namespace Engine::Asset
{
    // Forward declarations
    struct AnimationData; // Estrutura futura que conterá os dados de um GLTF Anim
    
    // Utilitário de SRP para cálculo de Bone Transforms
    class AnimationUtils
    {
    public:
        // Função PRINCIPAL: Calcula as transformações finais dos ossos.
        // É aqui que a lógica de Keyframe, SLERP e Matrix Palettes acontece.
        static void calculateBoneTransforms(
            const std::shared_ptr<Model>& model,        // Dados do asset (Skeleton, Keyframes)
            uint32_t currentAnimationID, 
            uint32_t previousAnimationID, 
            float currentTime, 
            float blendFactor, 
            std::vector<glm::mat4>& finalBoneTransforms // O vetor de saída (Componente)
        );

        // Utilitário que você já usou no PlayerSystem
        static uint32_t getAnimationHashID(const std::string& name);

    private:
        // Funções internas para lógica de animação (futuro)
        static glm::mat4 calculateLocalTransform(
            const AnimationData& anim, float time, uint32_t boneIndex);
        
        // ... (etc.)
    };
}