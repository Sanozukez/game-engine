// // tools/mmap_compiler/asset_processor.h

#pragma once

#include <string>
#include <vector>
#include "../../engine/deps/cgltf/cgltf.h" // Necessário para cgltf_node
#include "../../shared/mmap_format/SceneFileFormat.h" // Necessário para SceneNode

namespace Compiler {

// Estrutura que liga o dado do GLB ao dado da cena durante a compilação.
struct CompilerNode; 

// Funções de Checagem
bool is_scene_asset(const char *name);

// Funções de Metadados
std::string get_string_metadata(cgltf_node* node, const std::string& key);
float get_float_metadata(cgltf_node* node, const std::string& key, float default_value);

// Funções de Criação
SceneNode create_scene_node(SceneNodeID id, cgltf_node* node);

} // namespace Compiler