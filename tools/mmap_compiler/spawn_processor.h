// // tools/mmap_compiler/spawn_processor.h

#pragma once

#include <vector>
#include <fstream>

#include "../../engine/deps/cgltf/cgltf.h" 
#include "../../shared/mmap_format/SceneFileFormat.h" 

namespace Compiler {

    /**
     * @brief Processa um nó GLTF e extrai todos os dados necessários para o NPC Spawn.
     * @param node O nó GLTF (cgltf_node) a ser lido.
     * @return O struct NPCSpawnData preenchido com as custom properties.
     */

    // NOVO: Declaração da função que estava faltando
    NPCSpawnData process_npc_spawn(cgltf_node* node); 

    // O helper de escrita do array de NPCs
    void write_npc_section(const std::vector<NPCSpawnData>& npc_spawns, 
                           SceneFileHeader& header, 
                           std::ofstream& outfile, 
                           uint64_t& current_offset);

} // namespace Compiler