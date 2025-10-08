// // tools/mmap_compiler/asset_processor.cpp (Final)

#include "asset_processor.h"
#include "mmap_compiler_data.h"

#include <iostream>
#include <cstring>
#include <string>

// Incluído aqui, pois as funções o utilizam diretamente
#include <nlohmann/json.hpp> 

using json = nlohmann::json;

namespace Compiler {

// =========================================================================
// 1. FUNÇÕES DE CHECAGEM DE PREFIXOS
// =========================================================================

bool is_scene_asset(const char *name)
{
    if (!name) return false;
    return strncmp(name, "SPAWN_", 6) == 0 || 
           strncmp(name, "STATIC_", 7) == 0 || 
           strncmp(name, "ARRAY_", 6) == 0 || 
           strncmp(name, "TER_", 4) == 0;
}

// =========================================================================
// 2. FUNÇÕES DE EXTRAÇÃO DE METADADOS (JSON EXTRAS)
// =========================================================================

std::string get_string_metadata(cgltf_node* node, const std::string& key) {
    if (node->extras.data) { 
        try {
            json extras = json::parse(node->extras.data); 
            if (extras.contains(key)) {
                return extras[key].get<std::string>();
            }
        } 
        catch (const json::parse_error& e) {
            std::cerr << "AVISO: Falha ao analisar extras JSON para o node '" 
                      << node->name << "'. Erro: " << e.what() << std::endl;
        }
    }
    return "";
}

float get_float_metadata(cgltf_node* node, const std::string& key, float default_value) {
    if (node->extras.data) { 
        try {
            json extras = json::parse(node->extras.data); 

            if (extras.contains(key) && extras[key].is_number()) {
                return extras[key].get<float>();
            }
        } 
        catch (const json::parse_error& e) {
            // Ignora o erro
        }
    }
    return default_value;
}

// =========================================================================
// 3. FUNÇÃO DE CRIAÇÃO DE NODE
// =========================================================================

SceneNode create_scene_node(SceneNodeID id, cgltf_node* node) {
    SceneNode scene_node = {}; 
    scene_node.entity_id = id;

    // NOVO ESSENCIAL: COPIA O NOME DO NÓ GLTF PARA O ARRAY DE CHAR DO MMAP
    if (node->name) {
        // Usa strncpy para copiar o nome de forma segura para o array de tamanho fixo
        // O MAX_NAME_LENGTH deve ser 64 (o tamanho do array).
        std::strncpy(scene_node.name, node->name, 64 - 1); 
        scene_node.name[64 - 1] = '\0'; // Garantir terminação nula
    }
    // Extrai a Transformação (Posição, Rotação Quatérnio, Escala)
    if (node->has_translation) {
        memcpy(scene_node.position, node->translation, sizeof(float) * 3);
    }
    // ... (Lógica de Quatérnio e Escala, que estão corretas) ...
    if (node->has_rotation) {
        scene_node.rotation_quat[0] = node->rotation[3]; 
        scene_node.rotation_quat[1] = node->rotation[0]; 
        scene_node.rotation_quat[2] = node->rotation[1]; 
        scene_node.rotation_quat[3] = node->rotation[2]; 
    } else {
        scene_node.rotation_quat[0] = 1.0f;
    }
    if (node->has_scale) {
        memcpy(scene_node.scale, node->scale, sizeof(float) * 3);
    } else { 
        scene_node.scale[0] = scene_node.scale[1] = scene_node.scale[2] = 1.0f; 
    }
    
    scene_node.type = EntityType::TYPE_UNKNOWN; 

    return scene_node;
}

} // namespace Compiler