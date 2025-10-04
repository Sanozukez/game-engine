// engine/render/material.cpp
#include "material.h"
#include "texture.h"       // Incluir Texture para o construtor/destrutor
#include "shader.h"        // Incluir Shader
#include "./../core/log.h" // Para logs
#include <format>

namespace Engine
{
    namespace Render
    {

        // Unidade de Textura Global (começamos em 0)
        // Definimos esta enumeração ou constantes para evitar confusão de números mágicos no shader
        enum TextureUnit
        {
            TU_BASE_COLOR = 0,
            TU_NORMAL_MAP = 1,
            TU_ROUGHNESS_MAP = 2,
            TU_METALLIC_MAP = 3,
            TU_OCCLUSION_MAP = 4,
            TU_EMISSIVE_MAP = 5,
            TU_COUNT // Total de unidades
        };

        Material::Material() = default;
        Material::~Material() = default;

        // Implementações dos setters e flags
        void Material::setBaseColorMap(std::unique_ptr<Texture> texture)
        {
            m_hasBaseColorMap = (texture != nullptr && texture->isLoaded());
            m_baseColorMap = std::move(texture);
            Engine::Log::Debug(std::format("Material: BaseColorMap set. Has map: {}.", m_hasBaseColorMap));
        }
        void Material::setNormalMap(std::unique_ptr<Texture> texture)
        {
            m_hasNormalMap = (texture != nullptr && texture->isLoaded());
            m_normalMap = std::move(texture);
            Engine::Log::Debug(std::format("Material: NormalMap set. Has map: {}.", m_hasNormalMap));
        }
        void Material::setRoughnessMap(std::unique_ptr<Texture> texture)
        {
            m_hasRoughnessMap = (texture != nullptr && texture->isLoaded());
            m_roughnessMap = std::move(texture);
            Engine::Log::Debug(std::format("Material: RoughnessMap set. Has map: {}.", m_hasRoughnessMap));
        }
        void Material::setMetallicMap(std::unique_ptr<Texture> texture)
        {
            m_hasMetallicMap = (texture != nullptr && texture->isLoaded());
            m_metallicMap = std::move(texture);
            Engine::Log::Debug(std::format("Material: MetallicMap set. Has map: {}.", m_hasMetallicMap));
        }
        void Material::setAmbientOcclusionMap(std::unique_ptr<Texture> texture)
        {
            m_hasAmbientOcclusionMap = (texture != nullptr && texture->isLoaded());
            m_ambientOcclusionMap = std::move(texture);
            Engine::Log::Debug(std::format("Material: AmbientOcclusionMap set. Has map: {}.", m_hasAmbientOcclusionMap));
        }
        void Material::setEmissiveMap(std::unique_ptr<Texture> texture)
        {
            m_hasEmissiveMap = (texture != nullptr && texture->isLoaded());
            m_emissiveMap = std::move(texture);
            Engine::Log::Debug(std::format("Material: EmissiveMap set. Has map: {}.", m_hasEmissiveMap));
        }

        // Implementação de activate para configurar uniforms do shader
        void Material::activate(const Shader &shader) const
        {
            // É crucial que o shader já esteja ativo (shader->use()) antes de chamar activate

            // --- 1. ENVIAR TEXTURAS (Samplers) ---
            // Usamos o setInt para informar ao shader QUAL unidade de textura (0, 1, 2, ...)
            // corresponde a QUAL sampler2D no struct uMaterial.

            // Base Color Map
            if (m_hasBaseColorMap)
            {
                m_baseColorMap->bind(TU_BASE_COLOR); // Ativa a textura na unidade 0
                shader.setInt("uMaterial.baseColorMap", TU_BASE_COLOR);
            }
            shader.setInt("uMaterial.hasBaseColorMap", m_hasBaseColorMap); // Flag

            // Normal Map
            if (m_hasNormalMap)
            {
                m_normalMap->bind(TU_NORMAL_MAP); // Ativa a textura na unidade 1
                shader.setInt("uMaterial.normalMap", TU_NORMAL_MAP);
            }
            shader.setInt("uMaterial.hasNormalMap", m_hasNormalMap); // Flag

            // Roughness Map
            if (m_hasRoughnessMap)
            {
                m_roughnessMap->bind(TU_ROUGHNESS_MAP);
                shader.setInt("uMaterial.roughnessMap", TU_ROUGHNESS_MAP);
            }
            shader.setInt("uMaterial.hasRoughnessMap", m_hasRoughnessMap);

            // Metallic Map
            if (m_hasMetallicMap)
            {
                m_metallicMap->bind(TU_METALLIC_MAP);
                shader.setInt("uMaterial.metallicMap", TU_METALLIC_MAP);
            }
            shader.setInt("uMaterial.hasMetallicMap", m_hasMetallicMap);

            // Occlusion Map
            if (m_hasAmbientOcclusionMap)
            {
                m_ambientOcclusionMap->bind(TU_OCCLUSION_MAP);
                shader.setInt("uMaterial.occlusionMap", TU_OCCLUSION_MAP);
            }
            shader.setInt("uMaterial.hasOcclusionMap", m_hasAmbientOcclusionMap);

            // Emissive Map
            if (m_hasEmissiveMap)
            {
                m_emissiveMap->bind(TU_EMISSIVE_MAP);
                shader.setInt("uMaterial.emissiveMap", TU_EMISSIVE_MAP);
            }
            shader.setInt("uMaterial.hasEmissiveMap", m_hasEmissiveMap);

            // --- 2. ENVIAR FATORES (Floats e Vecs) ---

            shader.setVec4("uMaterial.baseColorFactor", baseColorFactor);
            shader.setFloat("uMaterial.metallicFactor", metallicFactor);
            shader.setFloat("uMaterial.roughnessFactor", roughnessFactor);
            shader.setVec3("uMaterial.emissiveFactor", emissiveFactor);
            shader.setFloat("uMaterial.normalScale", normalScale);
            shader.setFloat("uMaterial.occlusionStrength", occlusionStrength);
        }

        void Material::deactivate() const
        {
            // Desvincula TODAS as unidades de textura que foram usadas
            for (int i = 0; i < TU_COUNT; ++i)
            {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }

        std::unique_ptr<Material> Material::clone() const
        {
            // 1. Cria um novo Material (Material::Material() é chamado)
            auto clonedMaterial = std::make_unique<Material>();

            // 2. Copia as propriedades PBR (valores float/vec4)
            clonedMaterial->baseColorFactor = this->baseColorFactor;
            clonedMaterial->metallicFactor = this->metallicFactor;
            clonedMaterial->roughnessFactor = this->roughnessFactor;
            clonedMaterial->emissiveFactor = this->emissiveFactor;
            clonedMaterial->normalScale = this->normalScale;
            clonedMaterial->occlusionStrength = this->occlusionStrength;

            // 3. Copia Profunda (Deep Copy) das Texturas
            // O Material chama o clone() de cada Texture, resolvendo o ownership do unique_ptr.

            // Base Color Map
            if (m_baseColorMap)
            {
                clonedMaterial->setBaseColorMap(m_baseColorMap->clone());
            }

            // Normal Map
            if (m_normalMap)
            {
                clonedMaterial->setNormalMap(m_normalMap->clone());
            }

            // Roughness Map
            if (m_roughnessMap)
            {
                clonedMaterial->setRoughnessMap(m_roughnessMap->clone());
            }

            // Metallic Map
            if (m_metallicMap)
            {
                clonedMaterial->setMetallicMap(m_metallicMap->clone());
            }

            // Ambient Occlusion Map
            if (m_ambientOcclusionMap)
            {
                clonedMaterial->setAmbientOcclusionMap(m_ambientOcclusionMap->clone());
            }

            // Emissive Map
            if (m_emissiveMap)
            {
                clonedMaterial->setEmissiveMap(m_emissiveMap->clone());
            }

            // NOTA: Os flags m_hasXMap são configurados automaticamente pelos setters (setBaseColorMap, etc.).

            return clonedMaterial;
        }

    } // namespace Render
} // namespace Engine