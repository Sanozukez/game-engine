#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <string> // Para o nome da animação (debug/editor)

namespace Engine
{
    namespace ECS
    {
        namespace Component
        {

            // Componente para armazenar o estado de animação da Entidade
            struct Animation
            {
                // 1. DADOS DE ASSET

                // O ID do asset (do AssetManager) que contem o skeleton e as animações
                uint32_t animationAssetID = 0;

                // 2. ESTADO ATUAL

                // ID da animação atual (ex: hash da string "run")
                uint32_t currentAnimationID = 0;

                // Tempo atual dentro do ciclo da animação (0.0f a duração)
                float currentTime = 0.0f;

                // 3. BLEND/TRANSIÇÃO

                // Animação anterior para transição suave
                uint32_t previousAnimationID = 0;

                // Fator de blend entre previous e current (0.0f = 100% previous, 1.0f = 100% current)
                float blendFactor = 1.0f;

                // Taxa de blend (para animar o blendFactor)
                float blendSpeed = 5.0f; // Ex: 5.0f/s para transição rápida

                // 4. BONE TRANSFORMS (O RESULTADO FINAL DO SISTEMA)

                // Vetor que armazena a matriz de transformação final de cada osso (lida pelo Renderer)
                // O Renderer usará estas matrizes para desenhar a mesh na posição correta do osso.
                std::vector<glm::mat4> finalBoneTransforms;

                // Constante para o número máximo de ossos suportados (evita realocação)
                static constexpr uint32_t MAX_BONES = 100;

                // Construtor default: inicializa a lista de transforms
                Animation()
                {
                    finalBoneTransforms.resize(MAX_BONES, glm::mat4(1.0f));
                }
            };

        } // namespace Component
    } // namespace ECS
} // namespace Engine