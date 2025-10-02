// // engine/core/SceneLoader.h

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include "path_utils.h"
#include <filesystem>
// Inclui a API de formato binário que criamos:
#include "../../shared/mmap_format/SceneFileFormat.h"

namespace Engine
{

    class SceneLoader
    {
    public:
        // Lista onde todos os SceneNodes do mapa serão armazenados (a sua 'cena' em memória)
        std::vector<SceneNode> nodes;

        bool loadMapBinary(const std::string &mmap_file_path)
        {
            std::filesystem::path fullPath = Engine::resolveEnginePath(mmap_file_path);
            std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

            if (!file.is_open())
            {
                std::cerr << "ERRO: Nao foi possivel abrir o arquivo .mmap: " << mmap_file_path << std::endl;
                return false;
            }

            // 1. LEITURA RAPIDA DO CABEÇALHO (HEADER)
            SceneFileHeader header;

            // Retorna o ponteiro para o inicio do arquivo
            file.seekg(0, std::ios::beg);

            // Le o cabecalho completo de uma vez (leitura binária otimizada)
            file.read(reinterpret_cast<char *>(&header), sizeof(SceneFileHeader));

            // Verifica a integridade (Magic Number)
            if (header.magic != 0x4D4D4150)
            { // 'MMAP'
                std::cerr << "ERRO: Assinatura do arquivo MMAP invalida." << std::endl;
                return false;
            }

            // 2. ACESSO DIRETO AOS NODES (Otimização do MMAP)
            const auto &node_section = header.sections[(int)SceneSectionType::SCENE_SECTION_NODES];

            if (node_section.count > 0)
            {
                nodes.resize(node_section.count);

                // Salta diretamente para o bloco de dados de NODES
                file.seekg(node_section.offset, std::ios::beg);

                // Le o bloco de dados de todos os SceneNodes de uma vez
                file.read(reinterpret_cast<char *>(nodes.data()), node_section.count * sizeof(SceneNode));
            }

            std::cout << "\n[SceneLoader] Mapa carregado com sucesso!" << std::endl;
            std::cout << "  -> Nodes carregados (MMAP): " << nodes.size() << std::endl;
            std::cout << "  -> ID do Mapa: " << header.map_id << std::endl;

            // 3. (Opcional) DEBUG: Imprime o primeiro node lido
            if (!nodes.empty())
            {
                std::cout << "  -> Posicao do Node [1]: (" << nodes[0].position[0] << ", "
                          << nodes[0].position[1] << ", " << nodes[0].position[2] << ")" << std::endl;
            }

            return true;
        }
    };

} // namespace Engine