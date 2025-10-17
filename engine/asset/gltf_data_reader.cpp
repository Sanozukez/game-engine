// engine/asset/gltf_data_reader.cpp
#include "gltf_data_reader.h"
#include "model.h"
#include "../core/log.h"
#include "../deps/cgltf/cgltf.h"
#include "../render/material.h"
#include "../render/texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <format>
#include <stdexcept>
#include <string>
#include <cstring>
#include <memory>

using namespace Engine::Asset;
using namespace Engine::Render;

// =========================================================================
// 1. IMPLEMENTAÇÃO: loadGltfTexture (Logica de Textura)
// =========================================================================
// Esta função é uma cópia da lógica de carregamento de textura que estava no gltf_loader.cpp
std::unique_ptr<Texture> GltfDataReader::loadGltfTexture(const cgltf_texture *gltfTexture, const std::string &baseDirectory)
{
    if (!gltfTexture || !gltfTexture->image) { return nullptr; }
    const cgltf_image *gltfImage = gltfTexture->image;

    // Lógica COMPLETA para carregar textura de buffer view (dados binários diretos)
    if (gltfImage->buffer_view) {
        const cgltf_buffer_view *bufferView = gltfImage->buffer_view;
        if (!bufferView->buffer || !bufferView->buffer->data || bufferView->size == 0) {
            Engine::Core::Log::Error("GltfDataReader: Buffer da imagem binária direta é nulo ou vazio.");
            return nullptr;
        }
        int width, height, numChannels;
        unsigned char *data = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc *>(static_cast<const char *>(bufferView->buffer->data) + bufferView->offset),
            static_cast<int>(bufferView->size), &width, &height, &numChannels, 0); 
        if (!data) {
            Engine::Core::Log::Error(std::format("GltfDataReader: Falha ao carregar dados da imagem binária direta. Erro: {}.", stbi_failure_reason()));
            return nullptr;
        }
        auto texture_obj = std::make_unique<Texture>(width, height, numChannels, data);
        stbi_image_free(data);
        return texture_obj;
    } 
    // Se a imagem é externa (URI)
    else if (gltfImage->uri) {
        std::string texturePath = baseDirectory + "/" + gltfImage->uri;
        try {
            return std::make_unique<Texture>(texturePath);
        } catch (const std::exception &e) {
            Engine::Core::Log::Error(std::format("GltfDataReader: Erro ao carregar textura externa '{}': {}", texturePath, e.what()));
            return nullptr;
        }
    }
    return nullptr;
}


// =========================================================================
// 2. IMPLEMENTAÇÃO: loadPrimitive (Logica Principal de Leitura de Atributos)
// =========================================================================

void GltfDataReader::loadPrimitive(
    const cgltf_primitive *gltfPrimitive,
    Engine::Asset::Model &model,
    const std::string &baseDirectory) 
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> tangents;
    std::vector<glm::ivec4> boneIDs;
    std::vector<glm::vec4> weights;
    cgltf_size current_vertex_count = 0;

    // --- LOOP DE ATRIBUTOS ---
    for (cgltf_size attr_idx = 0; attr_idx < gltfPrimitive->attributes_count; ++attr_idx)
    {
        const cgltf_attribute *attribute = &gltfPrimitive->attributes[attr_idx];
        const cgltf_accessor *accessor = attribute->data;
        current_vertex_count = accessor->count;

        if (attribute->type == cgltf_attribute_type_position) {
            positions.resize(accessor->count);
            for (cgltf_size k = 0; k < accessor->count; ++k) { cgltf_accessor_read_float(accessor, k, glm::value_ptr(positions[k]), 3); }
        } else if (attribute->type == cgltf_attribute_type_normal) {
            normals.resize(accessor->count);
            for (cgltf_size k = 0; k < accessor->count; ++k) { cgltf_accessor_read_float(accessor, k, glm::value_ptr(normals[k]), 3); }
        } else if (attribute->type == cgltf_attribute_type_texcoord) {
            texCoords.resize(accessor->count);
            for (cgltf_size k = 0; k < accessor->count; ++k) { cgltf_accessor_read_float(accessor, k, glm::value_ptr(texCoords[k]), 2); }
        } else if (attribute->type == cgltf_attribute_type_tangent) {
            tangents.resize(accessor->count);
            for (cgltf_size k = 0; k < accessor->count; ++k) { cgltf_accessor_read_float(accessor, k, glm::value_ptr(tangents[k]), 3); }
        } else if (attribute->type == cgltf_attribute_type_joints) { // JOINTS_0 (IDs)
            boneIDs.resize(accessor->count);
            for (cgltf_size k = 0; k < accessor->count; ++k) {
                uint32_t temp_ids[4];
                if (cgltf_accessor_read_uint(accessor, k, temp_ids, 4)) {
                    boneIDs[k] = glm::ivec4(temp_ids[0], temp_ids[1], temp_ids[2], temp_ids[3]);
                }
            }
        } else if (attribute->type == cgltf_attribute_type_weights) { // WEIGHTS_0
            weights.resize(accessor->count);
            for (cgltf_size k = 0; k < accessor->count; ++k) {
                cgltf_accessor_read_float(accessor, k, glm::value_ptr(weights[k]), 4);
            }
        }
    }

    // --- MONTAGEM DO VERTEX FINAL ---
    std::vector<Vertex> finalVertices;
    finalVertices.reserve(current_vertex_count);
    for (cgltf_size v_idx = 0; v_idx < current_vertex_count; ++v_idx)
    {
        Vertex newVertex;
        newVertex.Position = (v_idx < positions.size()) ? positions[v_idx] : glm::vec3(0.0f);
        newVertex.Normal = (v_idx < normals.size()) ? normals[v_idx] : glm::vec3(0.0f, 1.0f, 0.0f);
        newVertex.TexCoords = (v_idx < texCoords.size()) ? texCoords[v_idx] : glm::vec2(0.0f);
        newVertex.Tangent = (v_idx < tangents.size()) ? tangents[v_idx] : glm::vec3(0.0f);
        newVertex.BoneIDs = (v_idx < boneIDs.size()) ? boneIDs[v_idx] : glm::ivec4(0);
        newVertex.Weights = (v_idx < weights.size()) ? weights[v_idx] : glm::vec4(0.0f);
        finalVertices.push_back(newVertex);
    }
    
    // --- ÍNDICES E MATERIAIS ---
    std::vector<uint32_t> indices;
    if (gltfPrimitive->indices) {
        const cgltf_accessor *accessor = gltfPrimitive->indices;
        indices.resize(accessor->count);
        // Lógica de leitura de índices R_16u / R_32u
        if (accessor->component_type == cgltf_component_type_r_16u) {
            for (cgltf_size k = 0; k < accessor->count; ++k) {
                uint16_t temp_idx = static_cast<uint16_t>(cgltf_accessor_read_index(accessor, k)); 
                indices[k] = static_cast<GLuint>(temp_idx);
            }
        } else if (accessor->component_type == cgltf_component_type_r_32u) {
            for (cgltf_size k = 0; k < accessor->count; ++k) {
                uint32_t temp_idx = static_cast<uint32_t>(cgltf_accessor_read_index(accessor, k)); 
                indices[k] = static_cast<GLuint>(temp_idx);
            }
        } else {
            Engine::Core::Log::Error(std::format("GltfDataReader: Tipo de componente de índice não suportado para GLTF: {}", static_cast<int>(accessor->component_type))); 
        }
    } else {
        // Criar índices sequenciais se faltarem...
        for (cgltf_size k = 0; k < finalVertices.size(); ++k) {
            indices.push_back(static_cast<GLuint>(k));
        }
    }
    
    // Lógica de leitura de Material PBR e Texturas
    std::unique_ptr<Render::Material> material = std::make_unique<Render::Material>(); 
    if (gltfPrimitive->material) {
        const cgltf_material* gltfMaterial = gltfPrimitive->material;
        
        material->baseColorFactor = glm::vec4(gltfMaterial->pbr_metallic_roughness.base_color_factor[0], gltfMaterial->pbr_metallic_roughness.base_color_factor[1], gltfMaterial->pbr_metallic_roughness.base_color_factor[2], gltfMaterial->pbr_metallic_roughness.base_color_factor[3]);
        material->metallicFactor = gltfMaterial->pbr_metallic_roughness.metallic_factor;
        material->roughnessFactor = gltfMaterial->pbr_metallic_roughness.roughness_factor;
        material->normalScale = gltfMaterial->normal_texture.scale;
        material->occlusionStrength = gltfMaterial->occlusion_texture.scale; 
        material->emissiveFactor = glm::vec3(gltfMaterial->emissive_factor[0], gltfMaterial->emissive_factor[1], gltfMaterial->emissive_factor[2]);

        if (gltfMaterial->pbr_metallic_roughness.base_color_texture.texture) { material->setBaseColorMap(loadGltfTexture(gltfMaterial->pbr_metallic_roughness.base_color_texture.texture, baseDirectory)); }
        if (gltfMaterial->normal_texture.texture) { material->setNormalMap(loadGltfTexture(gltfMaterial->normal_texture.texture, baseDirectory)); }
        if (gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture) { material->setRoughnessMap(loadGltfTexture(gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture, baseDirectory)); }
        if (gltfMaterial->occlusion_texture.texture) { material->setAmbientOcclusionMap(loadGltfTexture(gltfMaterial->occlusion_texture.texture, baseDirectory)); }
        if (gltfMaterial->emissive_texture.texture) { material->setEmissiveMap(loadGltfTexture(gltfMaterial->emissive_texture.texture, baseDirectory)); }
    }

    if (!finalVertices.empty() && !indices.empty()) {
        model.addMesh(std::make_unique<Engine::Asset::Mesh>(std::move(finalVertices), std::move(indices), std::move(material)));
        Engine::Core::Log::Debug("GltfDataReader: Malha adicionada ao modelo.");
    }
}