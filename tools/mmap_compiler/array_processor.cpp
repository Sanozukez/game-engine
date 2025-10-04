// // tools/mmap_compiler/array_processor.cpp

#include "array_processor.h"
#include "asset_processor.h" // Necessário para get_float_metadata
#include "mmap_compiler_data.h"

#include <iostream>
#include <cmath>
#include <string>
#include <vector>

namespace Compiler
{

    // Utilitário simples para calcular a distância euclidiana 3D
    float calculate_distance(float x1, float y1, float z1, float x2, float y2, float z2)
    {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dz = z2 - z1;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // Funcao utilitaria para converter um angulo (Yaw) em Quaternio.
    // O angulo é dado em radianos.
    void euler_to_quaternion(float yaw_radians, float quat[4])
    {
        // Para simplificar, assumimos que a rotação é APENAS no eixo Y (Yaw).
        float half_yaw = yaw_radians * 0.5f;
        float sin_half_yaw = std::sin(half_yaw);
        float cos_half_yaw = std::cos(half_yaw);

        // Quaternio Q = [w, x, y, z]
        quat[0] = cos_half_yaw; // W
        quat[1] = 0.0f;         // X (Pitch)
        quat[2] = sin_half_yaw; // Y (Yaw)
        quat[3] = 0.0f;         // Z (Roll)
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
        // Ação: O asset_reference_id do nó gerado deve ser o mesmo do nó ARRAY_START
        instance.asset_reference_id = base_node.asset_reference_id;
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
            if (pos_start != std::string::npos)
            {
                start_name_prefix.erase(pos_start);
            }
            else
            {
                continue;
            }

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
            // Aqui usamos a função do asset_processor.cpp para ler os metadados.
            float module_length = get_float_metadata(start_source, "ARRAY_MODULE_LENGTH", ARRAY_BASE_MODULE_LENGTH);

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

            float step_factor = module_length / total_distance;

            // 1. Calcular o ângulo YAW (em radianos)
            // Usamos o padrão atan2(X, Z) para o Yaw, pois o eixo Y é vertical (rotação em torno de Y).
            // A ordem dx, dz é a mais comum.
            float yaw_radians = std::atan2(dx, dz);

            // 2. Aplicar o Offset de 180 Graus (PI) ou 90 Graus
            // Se o "frente" do seu módulo é -Y, e o sistema calcula a partir de +X ou +Z,
            // o ajuste é geralmente PI (180 graus) ou -PI/2.
            // Vamos assumir que o sistema GLTF (que espera Z ou -Z como frente) precisa de PI.

            const float PI = 3.14159265359f;
            // Ajuste de 180 graus (ou PI) é necessário para virar o objeto, pois o vetor 'd' aponta
            // na direção de ARRAY_END, mas o seu modelo pode estar virado 180 graus na origem.
            yaw_radians += (PI / 2.0f);

            // 3. Converter o ângulo para Quatérnio
            float rotation_q[4];
            euler_to_quaternion(yaw_radians, rotation_q);

            for (int i = 0; i < num_modules; ++i)
            {
                // Calcula a posição (Corrigido e já existente)
                float current_x = sx + (dx * step_factor * i);
                float current_y = sy + (dy * step_factor * i);
                float current_z = sz + (dz * step_factor * i);

                SceneNode instance = create_instance_node(
                    temp_start_node.scene_node,
                    next_entity_id++,
                    current_x, current_y, current_z);

                // 3. Aplicar a Rotação CALCULADA para a instância
                // Copia os 4 componentes (W, X, Y, Z) para o array de rotação da instância.
                memcpy(instance.rotation_quat, rotation_q, sizeof(float) * 4);

                generated_instances.push_back(instance);
            }
        }

        final_scene_nodes.insert(final_scene_nodes.end(), generated_instances.begin(), generated_instances.end());
        std::cout << "  --- Total de " << generated_instances.size() << " Modulos Gerados. ---" << std::endl;
    }

} // namespace Compiler