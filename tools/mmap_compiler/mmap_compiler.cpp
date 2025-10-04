// // tools/mmap_compiler/mmap_compiler.cpp

#include <iostream>
#include <fstream>
#include <cstring>   // Para memcpy
#include <cmath>     // Para std::floor
#include <algorithm> // Necessário
#include <string>
#include <vector>

// 1. Definições de Estrutura (Nossa API Binária e Structs Temporárias)
#include "mmap_compiler_data.h"

// 2. Módulos Funcionais (APIs que o main chama)
#include "asset_processor.h"
#include "array_processor.h"
// Inclua o header do escritor (que estava faltando)
#include "mmap_writer.h"
// Use o namespace onde as funções estão definidas
using namespace Compiler;

// 3. Dependência GLTF (Tipos e Implementação)
// Necessário para usar cgltf_data* e cgltf_node*
#define CGLTF_IMPLEMENTATION
#include "../../engine/deps/cgltf/cgltf.h"

// NOTE: A estrutura 'TempSceneNode' não existe mais. Foi substituída por 'CompilerNode'.
// A estrutura 'ProcessedNode' também foi removida.

int main(int argc, char *argv[])
{
    // A lista temporária (CORRIGIDA para usar o nome FINAL)
    // Contém os nós lidos do GLB (SceneNode + cgltf_node*)
    std::vector<CompilerNode> initial_nodes;

    // A lista principal que contém os nós originais + as instâncias geradas pelo array
    std::vector<SceneNode> final_scene_nodes;

    SceneNodeID next_entity_id = 1;

    // 1. Verificação de Argumentos
    if (argc < 2)
    {
        std::cerr << "ERRO: Caminho do arquivo GLB nao fornecido." << std::endl;
        std::cerr << "Uso: " << argv[0] << " <caminho/para/arquivo.glb>" << std::endl;
        return 1;
    }

    const char *glb_file_path = argv[1];
    std::cout << "--- MMAP Compiler ---" << std::endl;
    std::cout << "Processando GLB: " << glb_file_path << std::endl;

    // 2. Leitura do GLB via cgltf
    cgltf_options options = {};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, glb_file_path, &data);

    if (result != cgltf_result_success)
    {
        std::cerr << "ERRO: Falha ao carregar ou analisar o GLB. Codigo: " << result << std::endl;
        return 1;
    }

    cgltf_load_buffers(&options, data, glb_file_path);

    // 3. Iteracao e Processamento de Nodes (O AssetProcessor cria os nós)
    std::cout << "\nNodes de Cena Encontrados:" << std::endl;
    std::cout << "---------------------------" << std::endl;

    for (cgltf_size i = 0; i < data->nodes_count; ++i)
    {
        cgltf_node *node = &data->nodes[i];

        // Verifica se é um node que a nossa Engine precisa processar (via asset_processor.h)
        if (Compiler::is_scene_asset(node->name))
        {

            // 1. Cria o SceneNode base a partir do nó GLTF
            // (Função movida para asset_processor.cpp)
            SceneNode scene_node = Compiler::create_scene_node(next_entity_id, node);
            next_entity_id++;

            // 2. Atribuição de Tipo
            if (strncmp(node->name, "TER_", 4) == 0)
            {
                scene_node.type = EntityType::TYPE_TERRAIN_BASE;
            }
            else if (strncmp(node->name, "ARRAY_", 6) == 0)
            {
                scene_node.type = EntityType::TYPE_ARRAY_START;
            }
            else
            {
                scene_node.type = EntityType::TYPE_STATIC_MESH;
            }

            // 3. EXTRAI METADADOS ESSENCIAIS: ASSET_REF_ID
            std::string asset_ref_id_str = Compiler::get_string_metadata(node, "ASSET_REF_ID");
            uint32_t asset_id = 0; // O ID numérico a ser gravado no MMAP

            if (asset_ref_id_str.empty())
            {
                std::cerr << "AVISO: Node '" << node->name << "' nao tem a propriedade ASSET_REF_ID. Usando ID 0." << std::endl;
            }
            else
            {
                // CRUCIAL: Cria um ID numérico a partir do nome do arquivo (Hash simples para teste)
                std::hash<std::string> hasher;
                // O ID final é o hash do nome do asset (string)
                asset_id = static_cast<uint32_t>(hasher(asset_ref_id_str));
            }

            // 4. Salvar o ID no SceneNode
            scene_node.asset_reference_id = asset_id; // <--- AGORA O MMAP TEM O ID NUMÉRICO

            // Log de debug para ver o Asset ID sendo lido
            std::cout << "  - Asset Ref: " << (asset_ref_id_str.empty() ? "(Nenhum)" : asset_ref_id_str)
                      << " | ID Num: " << asset_id << std::endl;

            // 5. Armazena o node temporário (para uso na expansão de Arrays)
            CompilerNode temp = {scene_node, node};
            initial_nodes.push_back(temp);
            final_scene_nodes.push_back(scene_node); // Adiciona o nó original

            std::cout << "  - Node: " << node->name << " | ID: " << scene_node.entity_id << std::endl;
        }
    }

    // 4. Lógica de Expansão do ARRAY (Processa a lista inicial e adiciona instâncias na final)
    // Chamada correta: Passa a lista inicial para a função que gera as instâncias e
    // as adiciona à lista final. (Função movida para array_processor.cpp)
    expand_arrays(data, initial_nodes, final_scene_nodes, next_entity_id);

    // 5. Libera a memoria do cgltf
    cgltf_free(data);

    // 6. ETAPA DE ESCRITA BINÁRIA (Chamada ao Módulo Writer)
    // Chama a função de escrita binária (Função movida para mmap_writer.cpp)
    if (!Compiler::write_mmap_file(final_scene_nodes, glb_file_path))
    {
        return 1;
    }

    return 0;
}