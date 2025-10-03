// // tools/mmap_compiler/mmap_writer.cpp

#include "mmap_writer.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

namespace Compiler {

bool write_mmap_file(const std::vector<SceneNode>& final_nodes, const char* glb_file_path) {
    if (final_nodes.empty()) {
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

    // Seção 1: SCENE_SECTION_NODES
    header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].count = 
        static_cast<uint32_t>(final_nodes.size());

    header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].offset = current_offset;
    header.fileSize = current_offset + final_nodes.size() * sizeof(SceneNode); 
    
    // D) Preencher Metadados Finais do Cabeçalho
    header.map_id = 1; 
    
    std::cout << "\nEscrevendo arquivo MMAP binario: " << mmap_file_path << std::endl;

    // E) Escrita no Arquivo
    std::ofstream outfile(mmap_file_path, std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo para escrita: " << mmap_file_path << std::endl;
        return false;
    }

    // 1. Escreve o Cabeçalho (Header)
    outfile.write(reinterpret_cast<const char *>(&header), sizeof(SceneFileHeader)); 

    // 2. Escreve a Seção de Nodes
    if (!final_nodes.empty()) {
        std::cout << "  -> Escrevendo " << final_nodes.size() << " SceneNodes em offset "
                  << header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].offset << "..." << std::endl;
        outfile.write(reinterpret_cast<const char *>(final_nodes.data()), final_nodes.size() * sizeof(SceneNode));
    }

    outfile.close();
    
    std::cout << "\nCompilacao MMAP concluida! Tamanho total: " << header.fileSize << " bytes." << std::endl;
    
    return true;
}

} // namespace Compiler