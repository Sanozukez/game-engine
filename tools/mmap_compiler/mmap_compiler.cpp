// // tools/mmap_compiler/mmap_compiler.cpp (Parte 1: Includes e Utilitários)

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>   // Para memcpy
#include <cmath>     // Para std::floor
#include <algorithm> // Necessário
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Inclui o formato de arquivo (A API Binária)
#include "../../shared/mmap_format/SceneFileFormat.h"

// Inclui a biblioteca cgltf (Assumimos que o caminho está correto)
#define CGLTF_IMPLEMENTATION
#include "../../engine/deps/cgltf/cgltf.h"

// Constante para o comprimento base do módulo
const float ARRAY_BASE_MODULE_LENGTH = 4.0f;

// Estrutura temporária para armazenar o link entre o SceneNode e o cgltf_node
struct TempSceneNode
{
    SceneNode scene_node;
    cgltf_node *source_node_ptr; // Ponteiro para o nó GLB original
};

// Estrutura temporaria para armazenar temporariamente todos os dados do nó GLTF que precisamos.
struct ProcessedNode
{
    SceneNode scene_node;
    cgltf_node *source_node_ptr;
};

// =========================================================================
// 1. FUNÇÕES UTILITÁRIAS DE METADADOS (JSON)
// =========================================================================

// Extrai metadados de string
std::string get_string_metadata(cgltf_node *node, const std::string &key)
{
    if (node->extras.data)
    {
        try
        {
            json extras = json::parse(node->extras.data);

            if (extras.contains(key))
            {
                return extras[key].get<std::string>();
            }
        }
        catch (const json::parse_error &e)
        {
            std::cerr << "AVISO: Falha ao analisar extras JSON para o node '"
                      << node->name << "'. Erro: " << e.what() << std::endl;
        }
    }
    return "";
}

// Extrai metadados float
float get_float_metadata(cgltf_node *node, const std::string &key, float default_value)
{
    if (node->extras.data)
    {
        try
        {
            json extras = json::parse(node->extras.data);

            if (extras.contains(key) && extras[key].is_number())
            {
                return extras[key].get<float>();
            }
        }
        catch (const json::parse_error &e)
        {
            // Ignora o erro de parsing para o valor padrão, mas mostra aviso se a sintaxe quebrou
        }
    }
    return default_value;
}

// =========================================================================
// 2. FUNÇÕES DE CRIAÇÃO E PROCESSAMENTO DE NODES
// =========================================================================

// Função que verifica se o nome de um node é relevante para a cena
bool is_scene_asset(const char *name)
{
    if (!name)
        return false;
    return strncmp(name, "SPAWN_", 6) == 0 ||
           strncmp(name, "STATIC_", 7) == 0 ||
           strncmp(name, "ARRAY_", 6) == 0 ||
           strncmp(name, "TER_", 4) == 0;
}

// Cria um SceneNode a partir de um cgltf_node, extraindo transforms
SceneNode create_scene_node(SceneNodeID id, cgltf_node *node)
{
    SceneNode scene_node = {};
    scene_node.entity_id = id;

    // Extrai a Transformação (Posição)
    if (node->has_translation)
    {
        memcpy(scene_node.position, node->translation, sizeof(float) * 3);
    }
    // Rotação Quatérnio (Assume glTF: x, y, z, w. Converte para nossa convenção: w, x, y, z)
    if (node->has_rotation)
    {
        scene_node.rotation_quat[0] = node->rotation[3]; // W
        scene_node.rotation_quat[1] = node->rotation[0]; // X
        scene_node.rotation_quat[2] = node->rotation[1]; // Y
        scene_node.rotation_quat[3] = node->rotation[2]; // Z
    }
    else
    {
        scene_node.rotation_quat[0] = 1.0f; // W=1 (Identidade)
    }
    // Escala
    if (node->has_scale)
    {
        memcpy(scene_node.scale, node->scale, sizeof(float) * 3);
    }
    else
    {
        scene_node.scale[0] = scene_node.scale[1] = scene_node.scale[2] = 1.0f;
    }

    // O tipo será definido no loop principal
    scene_node.type = EntityType::TYPE_UNKNOWN;

    return scene_node;
}

// Função que cria uma cópia de um SceneNode e define a posição (usada para Arrays)
SceneNode create_instance_node(SceneNode base_node, SceneNodeID id, float x, float y, float z)
{
    SceneNode instance = base_node;
    instance.entity_id = id;
    instance.type = EntityType::TYPE_STATIC_MESH; // As instâncias geradas são estáticas
    instance.position[0] = x;
    instance.position[1] = y;
    instance.position[2] = z;
    return instance;
}

// Utilitário simples para calcular a distância euclidiana 3D
float calculate_distance(float x1, float y1, float z1, float x2, float y2, float z2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// =========================================================================
// 3. FUNÇÃO DE EXPANSÃO DO ARRAY
// =========================================================================

void expand_arrays(cgltf_data *data,
                   const std::vector<TempSceneNode> &initial_nodes, // Lista de todos os nós originais
                   std::vector<SceneNode> &final_scene_nodes,
                   SceneNodeID &next_entity_id)
{

    std::vector<SceneNode> generated_instances;

    // Mapeamento: Busca por pares ARRAY_START e ARRAY_END
    for (const auto &temp_start_node : initial_nodes)
    {
        if (temp_start_node.scene_node.type != EntityType::TYPE_ARRAY_START)
        {
            continue;
        }

        cgltf_node *start_source = temp_start_node.source_node_ptr;

        // 1. Encontrar o nó ARRAY_END correspondente
        cgltf_node *end_source = nullptr;
        std::string start_name_prefix(start_source->name);

        // Remove o sufixo _START para procurar o _END (ex: ARRAY_Wall_START -> ARRAY_Wall)
        size_t pos_start = start_name_prefix.find("_START");
        if (pos_start != std::string::npos)
        {
            start_name_prefix.erase(pos_start);
        }
        else
        {
            // Se não tem _START, não processamos este nó de array
            continue;
        }

        // Busca o nó END correspondente
        std::string end_name_match = start_name_prefix + "_END";

        for (cgltf_size i = 0; i < data->nodes_count; ++i)
        {
            if (data->nodes[i].name && data->nodes[i].name == end_name_match)
            {
                end_source = &data->nodes[i];
                break;
            }
        }

        if (!end_source)
        {
            std::cerr << "ERRO: Node ARRAY_START '" << start_source->name << "' nao encontrou o par END (" << end_name_match << ")." << std::endl;
            continue;
        }

        // 2. Leitura de Metadados e Cálculo de Parâmetros
        float module_length = get_float_metadata(start_source, "ARRAY_MODULE_LENGTH", ARRAY_BASE_MODULE_LENGTH);

        // Posição: [X, Y, Z]
        float sx = temp_start_node.scene_node.position[0];
        float sy = temp_start_node.scene_node.position[1];
        float sz = temp_start_node.scene_node.position[2];

        float ex = end_source->translation[0];
        float ey = end_source->translation[1];
        float ez = end_source->translation[2];

        float total_distance = calculate_distance(sx, sy, sz, ex, ey, ez);

        if (module_length <= 0.001f || total_distance <= 0.001f)
        {
            std::cerr << "AVISO: Distancia/Comprimento do modulo invalido. Pulando expansao de " << start_source->name << "." << std::endl;
            continue;
        }

        int num_modules = static_cast<int>(std::floor(total_distance / module_length));

        // --- CÁLCULO DO VETOR DE DIREÇÃO ---
        float dx = ex - sx;
        float dy = ey - sy;
        float dz = ez - sz;

        // Fator de passo: A fração da distância total por módulo
        float step_factor = module_length / total_distance;

        std::cout << "\n  --- Expansao ARRAY: " << start_source->name << " ---" << std::endl;
        std::cout << "  - Distancia Total: " << total_distance << "m" << std::endl;
        std::cout << "  - Modulos Gerados: " << num_modules << " (Comprimento: " << module_length << "m)" << std::endl;

        // 3. Geração das Instâncias
        for (int i = 0; i < num_modules; ++i)
        {
            // Calcula a posição da nova instância
            float current_x = sx + (dx * step_factor * i);
            float current_y = sy + (dy * step_factor * i);
            float current_z = sz + (dz * step_factor * i);

            // NOTA: A ROTAÇÃO do módulo deve ser calculada usando atan2(dz, dx) e depois convertida
            // para Quatérnio. Por enquanto, mantemos a rotação do nó START.

            SceneNode instance = create_instance_node(
                temp_start_node.scene_node, // Node base
                next_entity_id++,
                current_x, current_y, current_z);
            generated_instances.push_back(instance);
        }
    }

    // Adiciona todas as instâncias geradas à lista final
    final_scene_nodes.insert(final_scene_nodes.end(), generated_instances.begin(), generated_instances.end());
    std::cout << "  --- Total de " << generated_instances.size() << " Modulos Gerados. ---" << std::endl;
}

// // tools/mmap_compiler/mmap_compiler.cpp (Função main)

int main(int argc, char *argv[])
{
    // A lista principal de SceneNodes para escrita
    std::vector<SceneNode> final_scene_nodes;

    // A lista temporária que armazena o link entre SceneNode e cgltf_node (TempSceneNode)
    std::vector<TempSceneNode> initial_nodes;

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

    // 3. Iteracao e Processamento de Nodes
    std::cout << "\nNodes de Cena Encontrados:" << std::endl;
    std::cout << "---------------------------" << std::endl;

    for (cgltf_size i = 0; i < data->nodes_count; ++i)
    {
        cgltf_node *node = &data->nodes[i];
        if (!is_scene_asset(node->name))
            continue;

        // 1. Cria o SceneNode base a partir do nó GLTF
        SceneNode scene_node = create_scene_node(next_entity_id, node);
        next_entity_id++; // Incrementa o contador de ID

        // 2. Atribuição de Tipo (Lógica do Manual)
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

        // 3. Armazena o node temporário (para uso na expansão de Arrays)
        // O node GLTF (ponteiro) e o SceneNode (dados) são ligados aqui.
        TempSceneNode temp = {scene_node, node};
        initial_nodes.push_back(temp);

        // 4. Adiciona o node original à lista final que será escrita
        final_scene_nodes.push_back(scene_node);

        // --- Log de Debug (Omitindo a lógica complexa de Asset_Ref por enquanto) ---
        std::cout << "  - Node: " << node->name
                  << " | Type: " << (int)scene_node.type
                  << " | ID: " << scene_node.entity_id << std::endl;
    }

    // 4. Lógica de Expansão do ARRAY (AQUI)
    // Chamada correta: Passa a lista inicial para a função que gera as instâncias e
    // as adiciona à lista final.
    expand_arrays(data, initial_nodes, final_scene_nodes, next_entity_id);

    // 5. Libera a memoria do cgltf
    cgltf_free(data);

    // ====================================================================
    // 6. ETAPA DE ESCRITA BINÁRIA (WRITE MMAP)
    // ====================================================================

    if (final_scene_nodes.empty()) // Usa a lista final
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
    uint64_t current_offset = sizeof(SceneFileHeader);

    // Seção 1: SCENE_SECTION_NODES
    header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].count =
        static_cast<uint32_t>(final_scene_nodes.size());

    header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].offset = current_offset;
    current_offset += final_scene_nodes.size() * sizeof(SceneNode);

    // D) Preencher Metadados Finais do Cabeçalho
    header.map_id = 1;
    header.fileSize = current_offset;

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
    if (!final_scene_nodes.empty())
    {
        std::cout << "  -> Escrevendo " << final_scene_nodes.size() << " SceneNodes em offset "
                  << header.sections[(int)SceneSectionType::SCENE_SECTION_NODES].offset << "..." << std::endl;
        outfile.write((char *)final_scene_nodes.data(), final_scene_nodes.size() * sizeof(SceneNode));
    }

    outfile.close();

    std::cout << "\nCompilacao MMAP concluida! Tamanho total: " << header.fileSize << " bytes." << std::endl;

    return 0;
}