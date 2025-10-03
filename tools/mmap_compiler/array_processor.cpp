// // tools/mmap_compiler/array_processor.cpp

#include "array_processor.h"
#include "asset_processor.h" // Necessário para get_float_metadata
#include "mmap_compiler_data.h"

#include <iostream>
#include <cmath>
#include <string>
#include <vector>

namespace Compiler {

// Utilitário simples para calcular a distância euclidiana 3D
float calculate_distance(float x1, float y1, float z1, float x2, float y2, float z2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Função que cria uma cópia de um SceneNode e define a posição (do asset_processor.cpp original)
SceneNode create_instance_node(SceneNode base_node, SceneNodeID id, float x, float y, float z)
{
    SceneNode instance = base_node;
    instance.entity_id = id;
    instance.type = EntityType::TYPE_STATIC_MESH; 
    instance.position[0] = x;
    instance.position[1] = y;
    instance.position[2] = z;
    return instance;
}

// =========================================================================
// FUNÇÃO DE EXPANSÃO DO ARRAY
// =========================================================================

void expand_arrays(cgltf_data *data,
                   const std::vector<CompilerNode> &initial_nodes, // <-- CORRIGIDO
                   std::vector<SceneNode> &final_scene_nodes,
                   SceneNodeID &next_entity_id)
{

    std::vector<SceneNode> generated_instances;

    // NOTA: Troquei 'TempSceneNode' por 'CompilerNode' para consistência.
    for (const auto &temp_start_node : initial_nodes)
    {
        if (temp_start_node.scene_node.type != EntityType::TYPE_ARRAY_START)
        {
            continue;
        }

        cgltf_node *start_source = temp_start_node.source_node_ptr;
        
        // 1. Encontrar o nó ARRAY_END correspondente (Lógica de Mapeamento)
        cgltf_node *end_source = nullptr;
        std::string start_name_prefix(start_source->name);
        
        size_t pos_start = start_name_prefix.find("_START");
        if (pos_start != std::string::npos) {
            start_name_prefix.erase(pos_start);
        } else {
            continue; 
        }
        
        std::string end_name_match = start_name_prefix + "_END";

        for (cgltf_size i = 0; i < data->nodes_count; ++i) {
            if (data->nodes[i].name && data->nodes[i].name == end_name_match) {
                end_source = &data->nodes[i];
                break;
            }
        }

        if (!end_source) {
            std::cerr << "ERRO: Node ARRAY_START '" << start_source->name << "' nao encontrou o par END (" << end_name_match << ")." << std::endl;
            continue;
        }

        // 2. Leitura de Metadados e Cálculo de Parâmetros
        // Aqui usamos a função do asset_processor.cpp para ler os metadados.
        float module_length = get_float_metadata(start_source, "ARRAY_MODULE_LENGTH", ARRAY_BASE_MODULE_LENGTH);

        float sx = temp_start_node.scene_node.position[0];
        float sy = temp_start_node.scene_node.position[1];
        float sz = temp_start_node.scene_node.position[2];

        float ex = end_source->translation[0];
        float ey = end_source->translation[1];
        float ez = end_source->translation[2];

        float total_distance = calculate_distance(sx, sy, sz, ex, ey, ez);

        if (module_length <= 0.001f || total_distance <= 0.001f) {
            std::cerr << "AVISO: Distancia/Comprimento do modulo invalido. Pulando expansao de " << start_source->name << "." << std::endl;
            continue;
        }

        int num_modules = static_cast<int>(std::floor(total_distance / module_length));

        // --- CÁLCULO DO VETOR DE DIREÇÃO ---
        float dx = ex - sx;
        float dy = ey - sy;
        float dz = ez - sz;

        float step_factor = module_length / total_distance;

        std::cout << "\n  --- Expansao ARRAY: " << start_source->name << " ---" << std::endl;
        std::cout << "  - Distancia Total: " << total_distance << "m" << std::endl;
        std::cout << "  - Modulos Gerados: " << num_modules << " (Comprimento: " << module_length << "m)" << std::endl;

        // 3. Geração das Instâncias
        for (int i = 0; i < num_modules; ++i)
        {
            // Calcula a posição da nova instância
            float current_x = sx + (dx * step_factor * i);
            float current_y = sy + (dy * step_factor * i);
            float current_z = sz + (dz * step_factor * i);

            // NOTA: A rotação é mantida do START node.
            SceneNode instance = create_instance_node(
                temp_start_node.scene_node, // Node base
                next_entity_id++,
                current_x, current_y, current_z);
            generated_instances.push_back(instance);
        }
    }

    final_scene_nodes.insert(final_scene_nodes.end(), generated_instances.begin(), generated_instances.end());
    std::cout << "  --- Total de " << generated_instances.size() << " Modulos Gerados. ---" << std::endl;
}

} // namespace Compiler