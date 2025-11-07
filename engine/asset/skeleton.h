// engine/asset/skeleton.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "./../../engine/core/log.h"

namespace Engine
{

    /**
     * @brief Representa um único 'bone' (osso) na estrutura do esqueleto.
     * SRP: Armazena dados estáticos do bone (nome, hierarquia, pose de ligação).
     */
    struct Bone
    {
        std::string name;
        int id = -1;
        int parentId = -1;
        std::vector<int> childrenIds;

        // Matriz de Ligação Inversa (Inverse Bind Matrix - IBM)
        glm::mat4 inverseBindMatrix = glm::mat4(1.0f);

        // Matriz de Transformação Final (calculada a cada frame)
        glm::mat4 finalTransformation = glm::mat4(1.0f);

        // --- CORREÇÃO ---
        // Matriz Global (calculada a cada frame para debug/física)
        // Esta é a globalTransform (Parent * Local) ANTES da multiplicação pela IBM.
        glm::mat4 debug_GlobalTransform = glm::mat4(1.0f);
        // --- FIM DA CORREÇÃO ---
    };

    /**
     * @brief Contém a estrutura hierárquica completa do esqueleto.
     * SRP: Agrupar todos os bones, o nó raiz (root) e o mapa de nomes para ID.
     */
    class Skeleton
    {
    public:
        static constexpr int MAX_BONES = 100;

        Skeleton() = default;

        int rootNodeId = -1;
        std::vector<Bone> bones;
        std::map<std::string, int> boneNameMap;

        const glm::mat4 &getFinalBoneTransform(int boneId) const
        {
            if (boneId >= 0 && boneId < bones.size())
            {
                return bones[boneId].finalTransformation;
            }
            return glm::mat4(1.0f);
        }

        /**
         * @brief Extrai e retorna as matrizes finais de todos os bones.
         * SRP: Fornece o resultado do cálculo de animação para o sistema de renderização.
         */
        void getFinalBoneTransforms(std::vector<glm::mat4> &outTransforms) const
        {
            // --- NOVO LOG DE DEBUG ---
            if (bones.empty())
            {
                Engine::Core::Log::Error("[DEBUG_SKEL] getFinalBoneTransforms: 'skeleton->bones' ESTÁ VAZIO. Retornando vetor vazio.");
            }
            else
            {
                Engine::Core::Log::Info(std::format("[DEBUG_SKEL] getFinalBoneTransforms: 'skeleton->bones' tem {} ossos. Retornando matrizes.", bones.size()));
            }
            // --- CORREÇÃO CRÍTICA ---
            // Não destrua o vetor 'outTransforms'. O AnimationComponent
            // (que é o 'outTransforms') já está pré-dimensionado para MAX_BONES.
            // Apenas copie as N matrizes calculadas (bones.size()) para
            // as N primeiras posições do vetor de destino.

            // outTransforms.clear(); // <-- REMOVIDO
            // outTransforms.reserve(bones.size()); // <-- REMOVIDO

            // Verificação de segurança: O destino (100) deve ser capaz
            // de conter os ossos (20).
            if (outTransforms.size() < bones.size())
            {
                 Engine::Core::Log::Error(std::format("[DEBUG_SKEL] O vetor de destino (size={}) é menor que os ossos (size={}). Redimensionando.", 
                    outTransforms.size(), bones.size()));
                 // Isso não deveria acontecer se o AnimationComponent
                 // estiver usando MAX_BONES
                 outTransforms.resize(bones.size()); 
            }

            // Copia as matrizes dos ossos calculados
            for (size_t i = 0; i < bones.size(); ++i)
            {
                outTransforms[i] = bones[i].finalTransformation;
            }

            // (Opcional, mas seguro): Preenche o resto do vetor (de 20 até 100)
            // com Identity, caso o RenderSystem envie o vetor inteiro.
            for (size_t i = bones.size(); i < outTransforms.size(); ++i)
            {
                outTransforms[i] = glm::mat4(1.0f);
            }
        }
    };

} // namespace Engine