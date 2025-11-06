// engine/asset/skeleton.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <map>
#include <memory>

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
            outTransforms.clear();
            outTransforms.reserve(bones.size());
            for (const auto &bone : bones)
            {
                outTransforms.push_back(bone.finalTransformation);
            }
        }
    };

} // namespace Engine