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

        asset_entries.push_back(entry);

        std::cout << std::format(" -> ID: {} | Path: {}", asset_id, asset_path) << std::endl;
    }

    // 3. Escrita do Arquivo Binário (asset_dictionary.bin)

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

    // Escreve o Header (Contagem total de entradas)
    uint32_t total_count = static_cast<uint32_t>(asset_entries.size());
    outfile.write(reinterpret_cast<const char *>(&total_count), sizeof(uint32_t));

    // Escreve o array de AssetEntry structs
    outfile.write(reinterpret_cast<const char *>(asset_entries.data()),
                  asset_entries.size() * sizeof(AssetEntry));

    outfile.close();

    std::cout << std::format("\nCompilacao de Dicionario concluida! {} assets compilados em {}.",
                             total_count, LOGICAL_FILENAME)
              << std::endl;

    return 0;
}
