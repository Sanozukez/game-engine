// engine/render/material.h
#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

// Forward declaration para a classe Texture
namespace Engine
{
    namespace Render
    {
        class Texture;
        class Shader; // NOVO: Forward declaration para Shader (necessário para activate())
    }
} // namespace Engine

namespace Engine
{
    namespace Render
    {

        // Classe para representar um Material PBR (Physically Based Rendering)
        // Encapsula os diferentes mapas de textura e propriedades do material.
        class Material
        {
        public:
            // Construtor padrão
            Material();
            ~Material();

            // Funções para definir os mapas de textura.
            // Aceita um unique_ptr para assumir a propriedade da textura.
            void setBaseColorMap(std::unique_ptr<Texture> texture);
            void setNormalMap(std::unique_ptr<Texture> texture);
            void setRoughnessMap(std::unique_ptr<Texture> Texture);
            void setMetallicMap(std::unique_ptr<Texture> texture);
            void setAmbientOcclusionMap(std::unique_ptr<Texture> texture);
            void setEmissiveMap(std::unique_ptr<Texture> texture);

            // Funções para obter os mapas de textura (const reference para acesso)
            const Texture *getBaseColorMap() const { return m_baseColorMap.get(); }
            const Texture *getNormalMap() const { return m_normalMap.get(); }
            const Texture *getRoughnessMap() const { return m_roughnessMap.get(); }
            const Texture *getMetallicMap() const { return m_metallicMap.get(); }
            const Texture *getAmbientOcclusionMap() const { return m_ambientOcclusionMap.get(); }
            const Texture *getEmissiveMap() const { return m_emissiveMap.get(); }

            // Propriedades PBR que podem ser definidas como uniformes no shader
            glm::vec4 baseColorFactor = glm::vec4(1.0f); // Fator de cor base (RGBA)
            float metallicFactor = 0.0f;                 // **** MUDANÇA: Padrão para 0.0 (não metálico) ****
            float roughnessFactor = 1.0f;                // **** MUDANÇA: Padrão para 1.0 (não reflexivo/áspero) ****
            glm::vec3 emissiveFactor = glm::vec3(0.0f);  // Fator emissivo (RGB)
            float normalScale = 1.0f;                    // Escala do normal map
            float occlusionStrength = 1.0f;              // Força do ambient occlusion map

            // **** NOVOS: Métodos para ativar/desativar o material no shader ****
            void activate(const Shader &shader) const;
            void deactivate() const;

        private:
            std::unique_ptr<Texture> m_baseColorMap;
            std::unique_ptr<Texture> m_normalMap;
            std::unique_ptr<Texture> m_roughnessMap;
            std::unique_ptr<Texture> m_metallicMap;
            std::unique_ptr<Texture> m_ambientOcclusionMap;
            std::unique_ptr<Texture> m_emissiveMap;

            // **** NOVOS: Flags para indicar se um mapa existe (para otimizar no shader) ****
            bool m_hasBaseColorMap = false;
            bool m_hasNormalMap = false;
            bool m_hasRoughnessMap = false;
            bool m_hasMetallicMap = false;
            bool m_hasAmbientOcclusionMap = false;
            bool m_hasEmissiveMap = false;
        };

    } // namespace Render
} // namespace Engine