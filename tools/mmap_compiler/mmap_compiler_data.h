// tools/mmap_compiler/mmap_compiler_data.h

#pragma once

#include <string>
#include <vector>
#include "../../shared/mmap_format/SceneFileFormat.h"
#include "../../engine/deps/cgltf/cgltf.h" // Necessário para cgltf_node

// Constante para o comprimento base do módulo
const float ARRAY_BASE_MODULE_LENGTH = 1.0f; 

namespace Compiler { // <--- NOVO: Envolve a struct CompilerNode

// Estrutura essencial para ligar o dado da Engine ao dado do GLB durante a compilação.
struct CompilerNode {
    SceneNode scene_node;
    cgltf_node* source_node_ptr; // Ponteiro para o nó GLB original
};

// Funções Utilitárias forward declarations (serão implementadas em asset_processor.cpp)
bool is_scene_asset(const char *name);
std::string get_string_metadata(cgltf_node* node, const std::string& key);
float get_float_metadata(cgltf_node* node, const std::string& key, float default_value);

} // namespace Compiler