// // engine/math/transform_utils.h

#pragma once

#include "../ecs/components/transform_component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp> // Para glm::mat4_cast

namespace Engine
{
    namespace Math
    {

        // Função utilitária para calcular a matriz Model a partir dos dados ECS.
        inline glm::mat4 getTransformMatrix(const ECS::Component::Transform &transform)
        {
            glm::mat4 modelMatrix = glm::mat4(1.0f);

            // 1. Translação (T) - Deve ser aplicado primeiro (mais à esquerda na multiplicação)
            // Isso garante que a rotação e a escala sejam feitas no espaço local.
            modelMatrix = glm::translate(modelMatrix, transform.position.toGLM());

            // 2. Rotação (R)
            // Multiplicada pela matriz de Translação (T * R)
            // Note: Usamos multiplicação simples (modelMatrix = modelMatrix * R)
            // ou no caso de GLM, você pode usar a sintaxe direta:
            modelMatrix = modelMatrix * glm::mat4_cast(transform.rotation);

            // 3. Escala (S)
            // Multiplicada pelas matrizes T e R (T * R * S)
            modelMatrix = glm::scale(modelMatrix, transform.scale.toGLM());

            return modelMatrix;
        }

    } // namespace Math
} // namespace Engine