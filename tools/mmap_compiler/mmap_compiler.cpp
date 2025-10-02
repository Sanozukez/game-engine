// // tools/mmap_compiler/mmap_compiler.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring> // Para memcpy

// Inclui o formato de arquivo que acabamos de criar (a ponte entre engine e tool)
#include "../../shared/mmap_format/SceneFileFormat.h"

// Inclui a biblioteca cgltf (assumindo que o header está em deps/cgltf/)
#define CGLTF_IMPLEMENTATION
#include "../../engine/deps/cgltf/cgltf.h"

// Função que verifica se o nome de um node é um 'SPAWN' ou 'STATIC'
bool is_scene_asset(const char *name)
{
    if (!name)
        return false;
    // Lógica para identificação de assets que precisam ser processados
    // Ex: STATIC_ para modularidade, SPAWN_ para itens instanciados
    return strncmp(name, "SPAWN_", 6) == 0 || strncmp(name, "STATIC_", 7) == 0 || strncmp(name, "ARRAY_", 6) == 0;
}

int main(int argc, char *argv[])
{
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

    // 3. Iteracao e Processamento de Nodes
    std::cout << "\nNodes de Cena Encontrados:" << std::endl;
    std::cout << "---------------------------" << std::endl;

    std::vector<SceneNode> scene_nodes;
    SceneNodeID next_entity_id = 1;

    for (cgltf_size i = 0; i < data->nodes_count; ++i)
    {
        cgltf_node *node = &data->nodes[i];

        // Verifica se é um node que a nossa Engine precisa processar (e não um node interno do GLB)
        if (is_scene_asset(node->name))
        {
            SceneNode scene_node = {}; // Inicializa a struct com 0

            // Define o ID de Entidade (para a Rede)
            scene_node.entity_id = next_entity_id++;

            // Define o Tipo (Simplificado por enquanto)
            scene_node.type = EntityType::TYPE_STATIC_MESH;

            // Extrai a Transformação
            if (node->has_matrix)
            {
                // Se tiver matriz, decompõe para posição/rotação/escala (tarefa mais complexa)
                // Vamos usar os componentes de transformação diretos, se existirem:
            }
            else
            {
                // Se o cgltf fornecer diretamente P/R/S, é mais fácil:
                if (node->has_translation)
                {
                    memcpy(scene_node.position, node->translation, sizeof(float) * 3);
                }
                if (node->has_rotation)
                {
                    // Copia o Quatérnio (W, X, Y, Z)
                    scene_node.rotation_quat[0] = node->rotation[3]; // W
                    scene_node.rotation_quat[1] = node->rotation[0]; // X
                    scene_node.rotation_quat[2] = node->rotation[1]; // Y
                    scene_node.rotation_quat[3] = node->rotation[2]; // Z
                    // cgltf armazena como x, y, z, w. Nossa struct usa W, X, Y, Z para consistência.
                }
                else
                {
                    // Rotação padrão (identidade) se não for especificada
                    scene_node.rotation_quat[0] = 1.0f;
                }
                if (node->has_scale)
                {
                    memcpy(scene_node.scale, node->scale, sizeof(float) * 3);
                }
                else
                {
                    scene_node.scale[0] = scene_node.scale[1] = scene_node.scale[2] = 1.0f; // Escala padrão
                }
            }

            // Exemplo de Saida (Debug)
            std::cout << "ID: " << scene_node.entity_id
                      << " | Nome: " << node->name
                      << " | Posicao: (" << scene_node.position[0]
                      << ", " << scene_node.position[1] << ", "
                      << scene_node.position[2] << ")" << std::endl;

            scene_nodes.push_back(scene_node);
        }
    }

    // 4. Libera a memoria do cgltf
    cgltf_free(data);

    // ====================================================================
    // 5. ETAPA DE ESCRITA BINÁRIA (WRITE MMAP)
    // ====================================================================

    if (scene_nodes.empty())
    {
        std::cout << "\nAVISO: Nenhuma SceneNode para escrever. Saindo." << std::endl;
        return 0;
    }

    // A) Definir o caminho de saída (.glb -> .mmap)
    std::string glb_path_str = glb_file_path;
    size_t last_dot = glb_path_str.find_last_of('.');
    std::string mmap_file_path = (last_dot == std::string::npos)
                                     ? glb_path_str + ".mmap"
                                     : glb_path_str.substr(0, last_dot) + ".mmap";

    std::cout << "\nEscrevendo arquivo MMAP binario: " << mmap_file_path << std::endl;

    // B) Inicializar o Cabeçalho
    SceneFileHeader header = {};

    // C) Calcular e preencher a Tabela de Offsets
    uint64_t current_offset = sizeof(SceneFileHeader); // O primeiro bloco começa logo após o cabeçalho

    // Seção 1: SCENE_SECTION_NODES
    header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].count = 
    static_cast<uint32_t>(scene_nodes.size()); 

    header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].offset = current_offset;
    current_offset += scene_nodes.size() * sizeof(SceneNode);

    // Seções Futuras (Deixamos como 0 por enquanto)
    // header.sections[(int)SceneSectionType::SCENE_SECTION_STATIC_MESHES].count = 0;
    // ...

    // D) Preencher Metadados Finais do Cabeçalho
    header.map_id = 1;                // ID Fixo por enquanto
    header.fileSize = current_offset; // O tamanho final do arquivo binário

    // E) Escrita no Arquivo
    std::ofstream outfile(mmap_file_path, std::ios::binary);
    if (!outfile.is_open())
    {
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo para escrita: " << mmap_file_path << std::endl;
        return 1;
    }

    // 1. Escreve o Cabeçalho (Header)
    outfile.write((char *)&header, sizeof(SceneFileHeader));

    // 2. Escreve a Seção de Nodes
    if (!scene_nodes.empty())
    {
        std::cout << "  -> Escrevendo " << scene_nodes.size() << " SceneNodes em offset "
                  << header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].offset << "..." << std::endl;
        outfile.write((char *)scene_nodes.data(), scene_nodes.size() * sizeof(SceneNode));
    }

    outfile.close();

    std::cout << "\nCompilacao MMAP concluida! Tamanho total: " << header.fileSize << " bytes." << std::endl;

    return 0;
}