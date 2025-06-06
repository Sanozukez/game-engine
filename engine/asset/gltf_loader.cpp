// engine/asset/gltf_loader.cpp
#include "gltf_loader.h"
#include "model.h"           // Inclui a definição de Engine::Asset::Model
#include "./../../engine/render/texture.h" // Inclui a definição de Engine::Render::Texture
#include "./../../engine/render/material.h" // Inclui a definição de Engine::Render::Material
#include "./../../engine/core/log.h"
#include "./../../engine/core/path_utils.h" // Para carregar arquivos de assets

#include <cgltf.h> // Inclua cgltf.h aqui

#include <glm/glm.hpp>       // Para tipos GLM
#include <vector>            // Para std::vector
#include <memory>            // Para std::unique_ptr
#include <stdexcept>         // Para exceções
#include <format>            // Para std::format
#include <cstdint>           // Para uint16_t, uint32_t
#include <glm/gtc/type_ptr.hpp> // Para glm::value_ptr

// Para stb_image_load_from_memory
#include <stb_image.h> 


namespace Engine {
namespace Asset {

// Helper function to load texture from cgltf_image/texture
std::unique_ptr<Render::Texture> loadGltfTexture(const cgltf_texture* gltfTexture, const std::string& baseDirectory) {
    if (!gltfTexture || !gltfTexture->image) {
        return nullptr;
    }

    const cgltf_image* gltfImage = gltfTexture->image;

    // Se a imagem está embedada (data URI)
    if (gltfImage->uri && strncmp(gltfImage->uri, "data:", 5) == 0) {
        // cgltf já decodifica para gltfImage->buffer_view internamente se ela puder.
        // Então, a lógica de buffer_view abaixo deve lidar com isso.
        Engine::Log::Debug(std::format("GLTFLoader: Textura de imagem embedada (data URI) '{}'.", gltfImage->uri));
        // Vamos direto para a lógica de buffer_view
    } 
    
    // **** CORRIGIDO: Lógica COMPLETA para carregar textura de buffer view (dados binários diretos) ****
    if (gltfImage->buffer_view) {
        Engine::Log::Debug(std::format("GLTFLoader: Tentando carregar textura binária direta (buffer view) para '{}'.", gltfImage->name ? gltfImage->name : "Sem Nome"));

        const cgltf_buffer_view* bufferView = gltfImage->buffer_view;
        if (!bufferView->buffer || !bufferView->buffer->data || bufferView->size == 0) {
            Engine::Log::Error("GLTFLoader: Buffer da imagem binária direta é nulo ou vazio. Dados não disponíveis.");
            return nullptr;
        }

        int width, height, numChannels; 
        unsigned char* data = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(static_cast<const char*>(bufferView->buffer->data) + bufferView->offset), 
            static_cast<int>(bufferView->size),                                           
            &width, &height, &numChannels, 0); // 0 = default (4 canais se tiver alfa)

        if (!data) {
            Engine::Log::Error(std::format("GLTFLoader: Falha ao carregar dados da imagem binária direta para '{}'. Erro: {}.", 
                                           gltfImage->name ? gltfImage->name : "Sem Nome", stbi_failure_reason()));
            return nullptr;
        }

        // Usar o construtor da Texture que aceita dados brutos
        auto texture_obj = std::make_unique<Render::Texture>(width, height, numChannels, data);
        stbi_image_free(data); 

        Engine::Log::Info(std::format("GLTFLoader: Textura binária direta '{}' carregada ({}x{}, {} canais). ID: {}.", 
                                      gltfImage->name ? gltfImage->name : "Sem Nome", width, height, numChannels, texture_obj->getID()));
        return texture_obj;

    } 
    // Se a imagem é externa (URI)
    else if (gltfImage->uri) {
        std::string texturePath = baseDirectory + "/" + gltfImage->uri;
        Engine::Log::Debug(std::format("GLTFLoader: Tentando carregar textura externa: '{}'", texturePath));
        try {
            return std::make_unique<Render::Texture>(texturePath); 
        } catch (const std::exception& e) {
            Engine::Log::Error(std::format("GLTFLoader: Erro ao carregar textura externa '{}': {}", texturePath, e.what()));
            return nullptr;
        }
    }
    return nullptr;
}


std::unique_ptr<Model> GLTFLoader::loadGLTF(const std::string &filePath) {
    Engine::Log::Info(std::format("GLTFLoader: Tentando carregar modelo GLTF de '{}'", filePath));

    std::filesystem::path fullPath = Engine::resolveEnginePath(filePath);
    std::string baseDirectory = fullPath.parent_path().string(); // Obtém o diretório pai


    cgltf_options options = {0};
    cgltf_data *data = nullptr;
    
    // cgltf_parse_file funciona para .gltf e .glb
    cgltf_result result = cgltf_parse_file(&options, fullPath.string().c_str(), &data);

    if (result != cgltf_result_success) {
        Engine::Log::Error(std::format("GLTFLoader: Erro ao parsear GLTF: {}", filePath));
        if (data) { cgltf_free(data); }
        throw std::runtime_error(std::format("GLTFLoader: Falha ao parsear GLTF: {}", filePath));
    }
    Engine::Log::Debug("GLTFLoader: Parsing inicial do GLTF concluído.");

    // **** RE-ADICIONADO: cgltf_load_buffers é essencial para preencher data->buffers[i].data ****
    Engine::Log::Debug("GLTFLoader: Carregando buffers bin├írios com cgltf_load_buffers...");
    result = cgltf_load_buffers(&options, data, fullPath.string().c_str()); 

    if (result != cgltf_result_success) {
        Engine::Log::Error(std::format("GLTFLoader: Erro ao carregar buffers GLTF para '{}'. Código de erro: {}.", 
                                       filePath, static_cast<int>(result)));
        cgltf_free(data);
        throw std::runtime_error(std::format("GLTFLoader: Falha ao carregar buffers GLTF: {}", filePath));
    }
    Engine::Log::Debug("GLTFLoader: Carregamento de buffers binários GLTF concluído.");


    auto model = std::make_unique<Model>();

    Engine::Log::Info(std::format("GLTFLoader: Processando {} malhas no GLTF '{}'.", data->meshes_count, filePath));
    for (cgltf_size scene_idx = 0; scene_idx < data->scenes_count; ++scene_idx) {
        const cgltf_scene* gltfScene = &data->scenes[scene_idx];
        Engine::Log::Debug(std::format("GLTFLoader: Processando cena '{}' (nós: {}).", 
                                       gltfScene->name ? gltfScene->name : "Sem Nome", gltfScene->nodes_count));
        
        for (cgltf_size node_idx = 0; node_idx < gltfScene->nodes_count; ++node_idx) {
            const cgltf_node* gltfNode = gltfScene->nodes[node_idx];
            
            if (gltfNode->mesh) { // Se o nó tem uma malha associada
                const cgltf_mesh* gltfMesh = gltfNode->mesh;
                Engine::Log::Debug(std::format("GLTFLoader: Processando malha '{}' do nó '{}' (primitivas: {}).", 
                                               gltfMesh->name ? gltfMesh->name : "Sem Nome", 
                                               gltfNode->name ? gltfNode->name : "Sem Nome", gltfMesh->primitives_count));
                
                for (cgltf_size j = 0; j < gltfMesh->primitives_count; ++j) {
                    const cgltf_primitive* gltfPrimitive = &gltfMesh->primitives[j];
                    
                    std::vector<glm::vec3> positions; 
                    std::vector<glm::vec3> normals;   
                    std::vector<glm::vec2> texCoords; 
                    std::vector<glm::vec3> tangents; 
                    std::vector<GLuint> indices;     

                    cgltf_size current_vertex_count = 0;

                    for (cgltf_size attr_idx = 0; attr_idx < gltfPrimitive->attributes_count; ++attr_idx) {
                        const cgltf_attribute* attribute = &gltfPrimitive->attributes[attr_idx];
                        const cgltf_accessor* accessor = attribute->data;

                        current_vertex_count = accessor->count; 

                        if (attribute->type == cgltf_attribute_type_position) {
                            positions.resize(accessor->count); 
                            for (cgltf_size k = 0; k < accessor->count; ++k) {
                                cgltf_accessor_read_float(accessor, k, glm::value_ptr(positions[k]), 3); 
                            }
                        } else if (attribute->type == cgltf_attribute_type_normal) {
                            normals.resize(accessor->count); 
                            for (cgltf_size k = 0; k < accessor->count; ++k) {
                                cgltf_accessor_read_float(accessor, k, glm::value_ptr(normals[k]), 3); 
                            }
                        } else if (attribute->type == cgltf_attribute_type_texcoord) {
                            texCoords.resize(accessor->count); 
                            for (cgltf_size k = 0; k < accessor->count; ++k) {
                                cgltf_accessor_read_float(accessor, k, glm::value_ptr(texCoords[k]), 2); 
                            }
                        } else if (attribute->type == cgltf_attribute_type_tangent) { 
                            tangents.resize(accessor->count);
                            for (cgltf_size k = 0; k < accessor->count; ++k) {
                                cgltf_accessor_read_float(accessor, k, glm::value_ptr(tangents[k]), 3); 
                            }
                        }
                    }
                    
                    std::vector<Vertex> finalVertices;
                    finalVertices.reserve(current_vertex_count); 

                    for (cgltf_size v_idx = 0; v_idx < current_vertex_count; ++v_idx) {
                        Vertex newVertex;
                        newVertex.Position = (v_idx < positions.size()) ? positions[v_idx] : glm::vec3(0.0f);
                        newVertex.Normal = (v_idx < normals.size()) ? normals[v_idx] : glm::vec3(0.0f, 1.0f, 0.0f);
                        newVertex.TexCoords = (v_idx < texCoords.size()) ? texCoords[v_idx] : glm::vec2(0.0f);
                        newVertex.Tangent = (v_idx < tangents.size()) ? tangents[v_idx] : glm::vec3(0.0f); 
                        finalVertices.push_back(newVertex);
                    }

                    if (gltfPrimitive->indices) {
                        const cgltf_accessor* accessor = gltfPrimitive->indices;
                        indices.resize(accessor->count);
                        
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
                            Engine::Log::Error(std::format("GLTFLoader: Tipo de componente de índice não suportado para GLTF: {}", static_cast<int>(accessor->component_type))); 
                        }
                    } else {
                        Engine::Log::Warn(std::format("GLTFLoader: Primitiva sem índices (malha '{}', primitiva {}). Criando índices sequenciais.",
                                                      gltfMesh->name ? gltfMesh->name : "Sem Nome", j));
                        for (cgltf_size k = 0; k < finalVertices.size(); ++k) {
                            indices.push_back(static_cast<GLuint>(k));
                        }
                    }
                    
                    std::unique_ptr<Render::Material> material = std::make_unique<Render::Material>(); 
                    if (gltfPrimitive->material) {
                        const cgltf_material* gltfMaterial = gltfPrimitive->material;
                        Engine::Log::Debug(std::format("GLTFLoader: Processando material '{}'.", gltfMaterial->name ? gltfMaterial->name : "Sem Nome"));

                        material->baseColorFactor = glm::vec4(gltfMaterial->pbr_metallic_roughness.base_color_factor[0],
                                                              gltfMaterial->pbr_metallic_roughness.base_color_factor[1],
                                                              gltfMaterial->pbr_metallic_roughness.base_color_factor[2],
                                                              gltfMaterial->pbr_metallic_roughness.base_color_factor[3]);
                        material->metallicFactor = gltfMaterial->pbr_metallic_roughness.metallic_factor;
                        material->roughnessFactor = gltfMaterial->pbr_metallic_roughness.roughness_factor;
                        material->normalScale = gltfMaterial->normal_texture.scale;
                        material->occlusionStrength = gltfMaterial->occlusion_texture.scale; 
                        material->emissiveFactor = glm::vec3(gltfMaterial->emissive_factor[0],
                                                             gltfMaterial->emissive_factor[1],
                                                             gltfMaterial->emissive_factor[2]);

                        if (gltfMaterial->pbr_metallic_roughness.base_color_texture.texture) {
                            material->setBaseColorMap(loadGltfTexture(gltfMaterial->pbr_metallic_roughness.base_color_texture.texture, baseDirectory));
                        }
                        if (gltfMaterial->normal_texture.texture) {
                            material->setNormalMap(loadGltfTexture(gltfMaterial->normal_texture.texture, baseDirectory));
                        }
                        if (gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture) {
                            material->setRoughnessMap(loadGltfTexture(gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture, baseDirectory));
                            Engine::Log::Debug("GLTFLoader: Metallic-Roughness map carregado como RoughnessMap. Shader precisa de lógica de separação de canais.");
                        }
                        if (gltfMaterial->occlusion_texture.texture) {
                            material->setAmbientOcclusionMap(loadGltfTexture(gltfMaterial->occlusion_texture.texture, baseDirectory));
                        }
                        if (gltfMaterial->emissive_texture.texture) {
                            material->setEmissiveMap(loadGltfTexture(gltfMaterial->emissive_texture.texture, baseDirectory));
                        }
                    } else {
                        Engine::Log::Debug("GLTFLoader: Primitiva sem material. Usando material padrão.");
                    }

                    if (!finalVertices.empty() && !indices.empty()) {
                        model->addMesh(std::make_unique<Mesh>(std::move(finalVertices), std::move(indices), std::move(material)));
                        Engine::Log::Debug(std::format("GLTFLoader: Malha (primitiva {}) adicionada ao modelo. Vértices: {}, Índices: {}.",
                                                        j, model->getMeshes().back()->getVertexCount(), model->getMeshes().back()->getIndexCount())); 
                    } else {
                        Engine::Log::Warn(std::format("GLTFLoader: Malha (primitiva {}) não possui vértices ou índices válidos. Ignorando.", j));
                    }
                }
            } else {
                Engine::Log::Debug(std::format("GLTFLoader: Nó '{}' não possui malha associada. Ignorando.", gltfNode->name ? gltfNode->name : "Sem Nome"));
            }
        }
    }

    cgltf_free(data);

    Engine::Log::Info(std::format("GLTFLoader: Carregamento detalhado de GLTF '{}' concluído. Total de malhas no modelo: {}.",
                                  filePath, model->getMeshes().size()));
    return model;
}

} // namespace Asset
} // namespace Engine