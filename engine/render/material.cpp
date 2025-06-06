// engine/render/material.cpp
#include "material.h"
#include "texture.h" // Incluir Texture para o construtor/destrutor
#include "shader.h" // Incluir Shader
#include "./../core/log.h" // Para logs
#include <format>

namespace Engine {
namespace Render {

Material::Material() = default; 
Material::~Material() = default; 

// Implementações dos setters e flags
void Material::setBaseColorMap(std::unique_ptr<Texture> texture) { 
    m_hasBaseColorMap = (texture != nullptr && texture->isLoaded());
    m_baseColorMap = std::move(texture); 
    Engine::Log::Debug(std::format("Material: BaseColorMap set. Has map: {}.", m_hasBaseColorMap));
}
void Material::setNormalMap(std::unique_ptr<Texture> texture) { 
    m_hasNormalMap = (texture != nullptr && texture->isLoaded());
    m_normalMap = std::move(texture); 
    Engine::Log::Debug(std::format("Material: NormalMap set. Has map: {}.", m_hasNormalMap));
}
void Material::setRoughnessMap(std::unique_ptr<Texture> texture) { 
    m_hasRoughnessMap = (texture != nullptr && texture->isLoaded());
    m_roughnessMap = std::move(texture); 
    Engine::Log::Debug(std::format("Material: RoughnessMap set. Has map: {}.", m_hasRoughnessMap));
}
void Material::setMetallicMap(std::unique_ptr<Texture> texture) { 
    m_hasMetallicMap = (texture != nullptr && texture->isLoaded());
    m_metallicMap = std::move(texture); 
    Engine::Log::Debug(std::format("Material: MetallicMap set. Has map: {}.", m_hasMetallicMap));
}
void Material::setAmbientOcclusionMap(std::unique_ptr<Texture> texture) { 
    m_hasAmbientOcclusionMap = (texture != nullptr && texture->isLoaded());
    m_ambientOcclusionMap = std::move(texture); 
    Engine::Log::Debug(std::format("Material: AmbientOcclusionMap set. Has map: {}.", m_hasAmbientOcclusionMap));
}
void Material::setEmissiveMap(std::unique_ptr<Texture> texture) { 
    m_hasEmissiveMap = (texture != nullptr && texture->isLoaded());
    m_emissiveMap = std::move(texture); 
    Engine::Log::Debug(std::format("Material: EmissiveMap set. Has map: {}.", m_hasEmissiveMap));
}

// Implementação de activate para configurar uniforms do shader
void Material::activate(const Shader& shader) const {
    Engine::Log::Debug("Material: Ativando material no shader.");

    // Definir fatores PBR
    shader.setVec4("uMaterial.baseColorFactor", baseColorFactor);
    shader.setFloat("uMaterial.metallicFactor", metallicFactor); 
    shader.setFloat("uMaterial.roughnessFactor", roughnessFactor); 
    shader.setVec3("uMaterial.emissiveFactor", emissiveFactor);
    shader.setFloat("uMaterial.normalScale", normalScale);
    shader.setFloat("uMaterial.occlusionStrength", occlusionStrength);

    // Bind e setar uniforms para os mapas de textura
    // Unidades de textura: 0: BaseColor, 1: Normal, 2: Roughness, 3: Metallic, 4: Occlusion, 5: Emissive

    shader.setInt("uMaterial.hasBaseColorMap", m_hasBaseColorMap);
    if (m_hasBaseColorMap && m_baseColorMap) {
        m_baseColorMap->bind(0);
        shader.setInt("uMaterial.baseColorMap", 0);
        Engine::Log::Debug(std::format("Material: BaseColorMap bound to unit 0. ID: {}.", m_baseColorMap->getID()));
    } else {
        shader.setInt("uMaterial.baseColorMap", 0); // Define um default, como uma textura branca 1x1
        Engine::Log::Trace("Material: No BaseColorMap, using default unit 0.");
    }

    shader.setInt("uMaterial.hasNormalMap", m_hasNormalMap);
    if (m_hasNormalMap && m_normalMap) {
        m_normalMap->bind(1);
        shader.setInt("uMaterial.normalMap", 1);
        Engine::Log::Debug(std::format("Material: NormalMap bound to unit 1. ID: {}.", m_normalMap->getID()));
    } else {
        shader.setInt("uMaterial.normalMap", 0); // Define um default
        Engine::Log::Trace("Material: No NormalMap, using default unit 1.");
    }

    shader.setInt("uMaterial.hasRoughnessMap", m_hasRoughnessMap); 
    if (m_hasRoughnessMap && m_roughnessMap) {
        m_roughnessMap->bind(2);
        shader.setInt("uMaterial.roughnessMap", 2);
        Engine::Log::Debug(std::format("Material: RoughnessMap bound to unit 2. ID: {}.", m_roughnessMap->getID()));
    } else {
        shader.setInt("uMaterial.roughnessMap", 0); // Define um default
        Engine::Log::Trace("Material: No RoughnessMap, using default unit 2.");
    }

    shader.setInt("uMaterial.hasMetallicMap", m_hasMetallicMap); 
    if (m_hasMetallicMap && m_metallicMap) {
        m_metallicMap->bind(3); 
        shader.setInt("uMaterial.metallicMap", 3);
        Engine::Log::Debug(std::format("Material: MetallicMap bound to unit 3. ID: {}.", m_metallicMap->getID()));
    } else {
        shader.setInt("uMaterial.metallicMap", 0); // Define um default
        Engine::Log::Trace("Material: No MetallicMap, using default unit 3.");
    }

    shader.setInt("uMaterial.hasOcclusionMap", m_hasAmbientOcclusionMap);
    if (m_hasAmbientOcclusionMap && m_ambientOcclusionMap) {
        m_ambientOcclusionMap->bind(4);
        shader.setInt("uMaterial.occlusionMap", 4);
        Engine::Log::Debug(std::format("Material: OcclusionMap bound to unit 4. ID: {}.", m_ambientOcclusionMap->getID()));
    } else {
        shader.setInt("uMaterial.occlusionMap", 0); // Define um default
        Engine::Log::Trace("Material: No OcclusionMap, using default unit 4.");
    }
    
    shader.setInt("uMaterial.hasEmissiveMap", m_hasEmissiveMap);
    if (m_hasEmissiveMap && m_emissiveMap) {
        m_emissiveMap->bind(5);
        shader.setInt("uMaterial.emissiveMap", 5);
        Engine::Log::Debug(std::format("Material: EmissiveMap bound to unit 5. ID: {}.", m_emissiveMap->getID()));
    } else {
        shader.setInt("uMaterial.emissiveMap", 0); // Define um default
        Engine::Log::Trace("Material: No EmissiveMap, using default unit 5.");
    }

    Engine::Log::Trace(std::format("Material: Ativado material. BaseColorMap: {}, NormalMap: {}, RoughnessMap: {}.",
                                    m_hasBaseColorMap, m_hasNormalMap, m_hasRoughnessMap));
}

void Material::deactivate() const {
    // Desvincular todas as unidades de textura que foram usadas
    // Não precisa desvincular aqui, pois a próxima chamada bind sobrescreverá.
    // Opcional se você tiver um gerenciador de estado OpenGL rigoroso.
    // Força desvinculação para cada unidade usada
    glActiveTexture(GL_TEXTURE5 + 1); // Garante que a unidade 5 (última usada) seja ativada
    glBindTexture(GL_TEXTURE_2D, 0); // Desvincula todas as unidades até 0
    glActiveTexture(GL_TEXTURE4 + 1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3 + 1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2 + 1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1 + 1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, 0);
    Engine::Log::Trace("Material: Material desativado.");
}

} // namespace Render
} // namespace Engine