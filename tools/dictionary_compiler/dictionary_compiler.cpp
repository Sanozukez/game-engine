// // tools/dictionary_compiler/dictionary_compiler.cpp

#include <iostream>
#include <fstream>
#include <format>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

#include "../../engine/core/path_utils.h"

// Inclui o formato de arquivo (onde AssetEntry e MAX_ASSET_PATH_LENGTH estão definidos)
#include "../../shared/mmap_format/SceneFileFormat.h"

using json = nlohmann::json;
// using namespace Compiler; // Para SceneNodeID, etc.

// Define o nome de saída binária
// const char *OUTPUT_FILENAME = "assets_dictionary.bin";
const char *TARGET_RUNTIME_PATH = "data/assets_dictionary.bin";

// Define o caminho para o JSON de origem (assumindo que ele está na pasta config/data/)
const char *JSON_SOURCE_PATH = "../../../data/asset_dictionary.json";

// -------------------------------------------------------------------------
// FUNÇÃO PRINCIPAL
// -------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    std::cout << "--- Asset Dictionary Compiler ---" << std::endl;

    // 1. Carregamento do JSON Source of Truth
    std::ifstream json_file(JSON_SOURCE_PATH);
    if (!json_file.is_open())
    {
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo JSON de origem: " << JSON_SOURCE_PATH << std::endl;
        std::cerr << "Certifique-se de que o arquivo JSON existe." << std::endl;
        return 1;
    }

    json data;
    try
    {
        json_file >> data;
    }
    catch (const json::parse_error &e)
    {
        std::cerr << "ERRO: Falha ao analisar o JSON de assets: " << e.what() << std::endl;
        return 1;
    }

    if (!data.contains("assets") || !data["assets"].is_array())
    {
        std::cerr << "ERRO: O arquivo JSON nao contem o array 'assets' no formato esperado." << std::endl;
        return 1;
    }

    // 2. Processamento e Geração das Entradas Binárias
    std::vector<AssetEntry> asset_entries;
    std::vector<AnimationMapping> all_animation_mappings; // NOVO v101: Todas as animations de todos os assets
    std::hash<std::string> hasher;

    std::cout << "Processando " << data["assets"].size() << " entradas de asset..." << std::endl;

    for (const auto &asset_json : data["assets"])
    {
        std::string asset_path = asset_json.value("path", "");
        uint32_t type_id = asset_json.value("type_id", 1); // 1 = Model (Fallback)

        if (asset_path.empty())
        {
            std::cerr << "AVISO: Entrada de asset sem 'path'. Pulando." << std::endl;
            continue;
        }

        // CRUCIAL: Calcula o ID numérico a partir do nome do arquivo
        uint32_t asset_id = static_cast<uint32_t>(hasher(asset_path));

        AssetEntry entry = {};
        entry.asset_id = asset_id;
        entry.asset_type = static_cast<uint8_t>(type_id);

        // Copia o caminho para o buffer binário (Tamanho Fixo)
        strncpy(entry.asset_path, asset_path.c_str(), MAX_ASSET_PATH_LENGTH - 1);
        entry.asset_path[MAX_ASSET_PATH_LENGTH - 1] = '\0'; // Garantir terminação nula

        // NOVO v101: Processar animations (se existir)
        if (asset_json.contains("animations") && asset_json["animations"].is_array())
        {
            size_t anim_count = asset_json["animations"].size();
            if (anim_count > 0)
            {
                entry.animation_count = static_cast<uint32_t>(anim_count);
                // IMPORTANTE: Salvar ÍNDICE no vetor global (será convertido para byte offset depois)
                entry.animation_data_offset = all_animation_mappings.size();
                
                std::cout << std::format(" -> ID: {} | Path: {} | Animations: {}", 
                                        asset_id, asset_path, entry.animation_count) << std::endl;

            // Processar cada animação
            for (const auto &anim_json : asset_json["animations"])
            {
                AnimationMapping anim = {};
                
                std::string engine_name = anim_json.value("engine_name", "");
                std::string source_name = anim_json.value("source_name", "");
                
                if (engine_name.empty() || source_name.empty())
                {
                    std::cerr << "AVISO: Animacao sem engine_name ou source_name. Pulando." << std::endl;
                    continue;
                }
                
                // Hash do engine_name (usado para lookup rápido)
                anim.engine_name_hash = static_cast<uint32_t>(hasher(engine_name));
                
                // Copiar source_name (nome no GLTF)
                strncpy(anim.source_name, source_name.c_str(), MAX_ANIM_NAME_LENGTH - 1);
                anim.source_name[MAX_ANIM_NAME_LENGTH - 1] = '\0';
                
                // Metadata
                anim.duration = anim_json.value("duration", 0.0f);
                anim.blend_in_time = anim_json.value("blend_in_time", 0.2f);
                anim.blend_out_time = anim_json.value("blend_out_time", 0.2f);
                anim.default_playback_speed = anim_json.value("playback_speed", 1.0f);
                anim.movement_speed = anim_json.value("movement_speed", 0.0f);
                anim.looping = anim_json.value("looping", true) ? 1 : 0;
                anim.priority = static_cast<uint8_t>(anim_json.value("priority", 100));
                
                all_animation_mappings.push_back(anim);
                
                std::cout << std::format("    - {} -> {} (speed: {:.2f}, loop: {})",
                                        engine_name, source_name, 
                                        anim.default_playback_speed, anim.looping) << std::endl;
            }
            }
        }
        else
        {
            // Asset sem animações
            entry.animation_count = 0;
            entry.animation_data_offset = 0;
            
            std::cout << std::format(" -> ID: {} | Path: {}", asset_id, asset_path) << std::endl;
        }

        asset_entries.push_back(entry);
    }

    // 3. Escrita do Arquivo Binário (asset_dictionary.bin) - FORMATO v101

    // O nome lógico do arquivo
    const char *LOGICAL_FILENAME = "assets_dictionary.bin";
    const char *TARGET_RUNTIME_PATH = "data/assets_dictionary.bin";

    std::string output_file_path = Engine::resolveEnginePathForWrite(TARGET_RUNTIME_PATH).string();

    std::ofstream outfile(output_file_path, std::ios::binary);

    if (!outfile.is_open())
    {
        std::cerr << "ERRO FATAL: Nao foi possivel abrir o arquivo para escrita: " << TARGET_RUNTIME_PATH << std::endl;
        return 1;
    }

    // NOVO v101: Calcular offsets
    size_t header_size = sizeof(AssetDictionaryHeader);
    size_t entries_size = asset_entries.size() * sizeof(AssetEntry);
    uint64_t animation_section_offset = header_size + entries_size;

    // CRÍTICO: Converter índices de animação para byte offsets
    for (auto& entry : asset_entries)
    {
        if (entry.animation_count > 0)
        {
            // animation_data_offset estava guardando o ÍNDICE, agora convertemos para BYTE OFFSET
            size_t anim_index = entry.animation_data_offset;
            entry.animation_data_offset = animation_section_offset + (anim_index * sizeof(AnimationMapping));
        }
    }

    // Escreve o Header v101
    AssetDictionaryHeader header = {};
    header.magic = 0x41535444;  // "ASTD"
    header.version = 101;
    header.padding = 0;
    header.asset_count = static_cast<uint32_t>(asset_entries.size());
    header.animation_section_offset = all_animation_mappings.empty() ? 0 : animation_section_offset;
    header.reserved[0] = 0;
    header.reserved[1] = 0;

    outfile.write(reinterpret_cast<const char *>(&header), sizeof(AssetDictionaryHeader));

    // Escreve o array de AssetEntry structs (v101: 152 bytes cada)
    outfile.write(reinterpret_cast<const char *>(asset_entries.data()),
                  asset_entries.size() * sizeof(AssetEntry));

    // NOVO v101: Escrever AnimationMapping[] section
    if (!all_animation_mappings.empty())
    {
        outfile.write(reinterpret_cast<const char *>(all_animation_mappings.data()),
                      all_animation_mappings.size() * sizeof(AnimationMapping));
    }

    outfile.close();

    std::cout << std::format("\nCompilacao de Dicionario concluida! (v101)")
              << std::format("\n  {} assets compilados", header.asset_count)
              << std::format("\n  {} animation mappings", all_animation_mappings.size())
              << std::format("\n  Salvo em: {}", LOGICAL_FILENAME)
              << std::endl;

    return 0;
}
