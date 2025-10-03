// // tools/mmap_compiler/array_processor.h (CORREÇÃO)

#pragma once

#include <vector>
#include "../../engine/deps/cgltf/cgltf.h" 
#include "../../shared/mmap_format/SceneFileFormat.h" 
#include "mmap_compiler_data.h" // <-- ESTE ARQUIVO DEFINE COMPILERNODE

namespace Compiler {

// Forward declaration: Utilidade para cálculo de array
float calculate_distance(float x1, float y1, float z1, float x2, float y2, float z2);

// FUNÇÃO CORRIGIDA: Usa CompilerNode em vez de TempSceneNode
void expand_arrays(cgltf_data *data,
                   const std::vector<CompilerNode> &initial_nodes, // <-- CORRIGIDO
                   std::vector<SceneNode> &final_scene_nodes,
                   SceneNodeID &next_entity_id);

} // namespace Compiler