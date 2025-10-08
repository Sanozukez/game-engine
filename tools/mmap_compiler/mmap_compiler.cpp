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
#include "spawn_processor.h"
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
    // Contém os nós lidos do GLB (SceneNode + cgltf_node*)
    std::vector<CompilerNode> initial_nodes;

    // A lista principal que contém os nós originais + as instâncias geradas pelo array
    std::vector<SceneNode> final_scene_nodes;

    // A lista principal que contém os nós originais + as instâncias geradas pelo array
    std::vector<NPCSpawnData> npc_spawn_nodes;

    // Container para armazenar os dados de Metadados do Terreno
    std::vector<TerrainMetaData> terrain_meta_data;

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

        // Verifica se é um node que a nossa Engine precisa processar
        if (Compiler::is_scene_asset(node->name))
        {
            // 1. Cria o SceneNode base a partir do nó GLTF
            SceneNode scene_node = Compiler::create_scene_node(next_entity_id, node);
            next_entity_id++;

            // 2. EXTRAI METADADOS ESSENCIAIS: ASSET_REF_ID (Necessário para o Hashing)
            std::string asset_ref_id_str = Compiler::get_string_metadata(node, "ASSET_REF_ID");
            uint32_t asset_id = 0;

            if (!asset_ref_id_str.empty())
            {
                std::hash<std::string> hasher;
                asset_id = static_cast<uint32_t>(hasher(asset_ref_id_str));
            }
            scene_node.asset_reference_id = asset_id; // Salva o ID numérico

            // 3. ATRIBUIÇÃO DE TIPO E COLETA DE DADOS DE GAMEPLAY
            if (strncmp(node->name, "TER_", 4) == 0)
            {
                scene_node.type = EntityType::TYPE_TERRAIN_BASE;

                // 1. Cria a struct de Metadados
                TerrainMetaData meta = {};

                // 2. Extrai a propriedade customizada para o nome da Mesh
                std::string mesh_name = Compiler::get_string_metadata(node, "INTERNAL_MESH_REF");

                // 3. Copia a string para o buffer de tamanho fixo
                strncpy(meta.internal_mesh_name, mesh_name.c_str(), MAX_MESH_NAME_LENGTH - 1);

                // Por enquanto, os outros campos (uv_scale, collision_mesh_id) são zero.

                // Armazena a struct
                terrain_meta_data.push_back(meta);

                // O offset aponta para a posição FINAL desta struct no MMAP
                scene_node.specific_data_offset = (uint64_t)terrain_meta_data.size() - 1;
            }
            else if (strncmp(node->name, "ARRAY_", 6) == 0)
            {
                scene_node.type = EntityType::TYPE_ARRAY_START;
            }
            else if (strncmp(node->name, "SPAWN_", 6) == 0)
            {
                // -- PROCESSAMENTO DE SPAWN DATA --

                // Tipo de Node: STATIC_MESH para visualização (temporário)
                scene_node.type = EntityType::TYPE_STATIC_MESH;

                // 1. Processa as Custom Properties para o bloco de NPC
                // (Função process_npc_spawn usa o ASSET_REF_ID/Hash para preencher unit_db_id)
                NPCSpawnData spawn_data = Compiler::process_npc_spawn(node);

                // 2. Armazena o offset (índice na lista npc_spawn_nodes) e o dado
                // O offset é o índice na lista, que será o tamanho ATUAL da lista ANTES de adicionar.
                scene_node.specific_data_offset = (uint64_t)npc_spawn_nodes.size();

                // 3. Adiciona o dado à lista de spawns
                npc_spawn_nodes.push_back(spawn_data);
            }
            
            else
            {
                scene_node.type = EntityType::TYPE_STATIC_MESH; // Default para objetos SM_
            }

            // 4. Armazena o node para processamento posterior (Arrays) e lista final
            CompilerNode temp = {scene_node, node};
            initial_nodes.push_back(temp);
            final_scene_nodes.push_back(scene_node); // Adiciona o nó original à lista final

            // Log de debug
            std::cout << "  - Node: " << node->name
                      << " | Type: " << (int)scene_node.type
                      << " | ID: " << scene_node.entity_id
                      << " | Asset ID Num: " << scene_node.asset_reference_id << std::endl;
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
    if (!Compiler::write_mmap_file(final_scene_nodes,
                                   npc_spawn_nodes,
                                   terrain_meta_data, // <--- NOVO ARGUMENTO ADICIONADO
                                   glb_file_path))
    {
        return 1;
    }

    return 0;
}