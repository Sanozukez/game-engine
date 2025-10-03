// // tools/mmap_compiler/mmap_writer.h

#pragma once

#include <vector>
#include <string>
#include "../../shared/mmap_format/SceneFileFormat.h" // Para SceneNode e Header

namespace Compiler {

/**
 * @brief Escreve todos os SceneNodes processados para o arquivo binário MMAP.
 * @param final_nodes Lista de todos os nós a serem escritos.
 * @param glb_file_path Caminho do arquivo GLB (para determinar o nome de saída .mmap).
 * @return True se a escrita for bem-sucedida.
 */
bool write_mmap_file(const std::vector<SceneNode>& final_nodes, const char* glb_file_path);

} // namespace Compiler