// // tools/mmap_compiler/spawn_processor.cpp

#include "spawn_processor.h"
// Inclui o header que expõe as funções de leitura JSON
#include "asset_processor.h" 
// Inclui a definição das structs NPCSpawnData e constantes
#include "mmap_compiler_data.h" 

#include <iostream>
#include <string>
#include <cstring>
#include <format> // Para o log de erro/aviso
#include <functional> // Para std::hash

namespace Compiler {

// =========================================================================
// 1. PROCESSAMENTO DE NÓ (CONVERSÃO GLTF -> STRUCT)
// =========================================================================

// Função que o loop principal chamará para converter o nó em dados de spawn
NPCSpawnData process_npc_spawn(cgltf_node* node) {
    NPCSpawnData spawn_data = {};
    
    // 1. EXTRAÇÃO DO ASSET_ID (CRUCIAL)
    // O Asset ID numérico (hash) será usado como o ID do Asset para carregar o modelo.
    // 1. EXTRAÇÃO DO ASSET_ID (CRUCIAL) - USANDO O CAMPO 'unit_db_id'
    std::string asset_path = get_string_metadata(node, "ASSET_REF_ID");    
    
    if (!asset_path.empty()) {
        std::hash<std::string> hasher;
        // O unit_db_id será usado para armazenar o ID do Asset para carregar o visual
        spawn_data.unit_db_id = static_cast<uint32_t>(hasher(asset_path));
         
    } else {
        std::cerr << "AVISO: Nó SPAWN '" << node->name << "' sem ASSET_REF_ID. Usando ID 0." << std::endl;
        spawn_data.unit_db_id = 0;
    }

    // 2. EXTRAÇÃO DE DADOS DE GAMEPLAY (Custom Properties e Transformação)
    
    // Posição: Copia a transformação do nó GLTF
    if (node->has_translation) {
        memcpy(spawn_data.position, node->translation, sizeof(float) * 3);
    }    

    // Tempo de Respawn
    spawn_data.respawn_time_sec = get_float_metadata(node, "RESPAWN_TIME", 30.0f);

    // Loot Table ID
    spawn_data.loot_table_id = static_cast<uint32_t>(get_float_metadata(node, "LOOT_TABLE_ID", 0.0f)); 

    // O campo patrol_route_id (uint16_t) e max_mobs_in_area (uint16_t) assumem 0 se não especificados.
    // Para preencher, precisaríamos de uma função get_uint16_metadata.

    return spawn_data;
}


// =========================================================================
// 2. ESCRITA BINÁRIA DA SEÇÃO DE NPC
// =========================================================================

// Implementação da escrita da seção de NPC (movida do mmap_writer.cpp)
void write_npc_section(const std::vector<NPCSpawnData>& npc_spawns, 
                       SceneFileHeader& header, 
                       std::ofstream& outfile, 
                       uint64_t& current_offset) 
{
    // 1. Atualiza o Header
    // SCENE_SECTION_NPC_SPAWNS é o índice 2 na enum SceneSectionType
    const int SECTION_INDEX = (int)SceneSectionType::SCENE_SECTION_NPC_SPAWNS;

    header.sections[SECTION_INDEX].count = static_cast<uint32_t>(npc_spawns.size());
    header.sections[SECTION_INDEX].offset = current_offset;

    // 2. Escreve a Seção Binária
    if (!npc_spawns.empty()) {
        outfile.write(reinterpret_cast<const char*>(npc_spawns.data()), 
                      npc_spawns.size() * sizeof(NPCSpawnData));
        
        // Atualiza o offset para o início da próxima seção
        current_offset += npc_spawns.size() * sizeof(NPCSpawnData);
    }
}


} // namespace Compiler