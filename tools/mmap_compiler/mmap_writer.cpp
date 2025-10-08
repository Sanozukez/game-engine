// // tools/mmap_compiler/mmap_writer.cpp

#include "mmap_writer.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

namespace Compiler
{

    bool write_mmap_file(const std::vector<SceneNode> &final_nodes,
                         const std::vector<NPCSpawnData> &npc_spawns,
                         const std::vector<TerrainMetaData>& terrain_meta,
                         const char *glb_file_path)
    {
        if (final_nodes.empty())
        {
            std::cout << "\nAVISO: Nenhuma SceneNode para escrever. Saindo." << std::endl;
            return true; // Considerado sucesso, pois não há o que escrever
        }

        // A) Definir o caminho de saída (.glb -> .mmap)
        std::string glb_path_str = glb_file_path;
        size_t last_dot = glb_path_str.find_last_of('.');
        std::string mmap_file_path = (last_dot == std::string::npos)
                                         ? glb_path_str + ".mmap"
                                         : glb_path_str.substr(0, last_dot) + ".mmap";

        // B) Inicializar e Preencher o Cabeçalho
        SceneFileHeader header = {};

        // C) Calcular e preencher a Tabela de Offsets
        uint64_t current_offset = sizeof(SceneFileHeader);

        // D. SEÇÃO 1: SceneNodes
        header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].count = static_cast<uint32_t>(final_nodes.size());
        header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].offset = current_offset;
        current_offset += final_nodes.size() * sizeof(SceneNode);

        // D. SEÇÃO 2: NPC SPAWNS (NOVO CÁLCULO)
        const int NPC_SECTION_INDEX = (int)SceneSectionType::SCENE_SECTION_NPC_SPAWNS;
        header.sections[NPC_SECTION_INDEX].count = static_cast<uint32_t>(npc_spawns.size());
        header.sections[NPC_SECTION_INDEX].offset = current_offset;
        current_offset += npc_spawns.size() * sizeof(NPCSpawnData); // Atualiza o offset com o tamanho do bloco

        // D. SEÇÃO 3: TERRAIN METADATA (NOVO CÁLCULO)
        const int TERRAIN_META_SECTION = (int)SceneSectionType::SCENE_SECTION_TERRAIN_METADATA;
        header.sections[TERRAIN_META_SECTION].count = static_cast<uint32_t>(terrain_meta.size());
        header.sections[TERRAIN_META_SECTION].offset = current_offset;
        current_offset += terrain_meta.size() * sizeof(TerrainMetaData); // Atualiza o offset

        // D. FINALIZAÇÃO DO HEADER
        header.map_id = 1;
        header.fileSize = current_offset;

        std::cout << "\nEscrevendo arquivo MMAP binario: " << mmap_file_path << std::endl;

        // E) Escrita no Arquivo
        std::ofstream outfile(mmap_file_path, std::ios::binary);
        if (!outfile.is_open())
        {
            std::cerr << "ERRO: Nao foi possivel abrir o arquivo para escrita: " << mmap_file_path << std::endl;
            return false;
        }

        // 1. Escreve o Cabeçalho (Header)
        outfile.write(reinterpret_cast<const char *>(&header), sizeof(SceneFileHeader));

        // 2. Escreve a Seção A: SceneNodes
        if (!final_nodes.empty())
        {
            outfile.write(reinterpret_cast<const char *>(final_nodes.data()), final_nodes.size() * sizeof(SceneNode));
        }

        // 3. Escreve a Seção B: NPC Spawns
        if (!npc_spawns.empty())
        {
            outfile.write(reinterpret_cast<const char *>(npc_spawns.data()), npc_spawns.size() * sizeof(NPCSpawnData));
        }

        // Escreve a Seção C: Terrain Metadados 
        if (!terrain_meta.empty()) {
            outfile.write(reinterpret_cast<const char *>(terrain_meta.data()), terrain_meta.size() * sizeof(TerrainMetaData));
        }

        outfile.close();

        std::cout << "\nCompilacao MMAP concluida! Tamanho total: " << header.fileSize << " bytes." << std::endl;

        return true;
    }

} // namespace Compiler