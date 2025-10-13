// // engine/asset/asset_manager.cpp

#include "asset_manager.h"
#include "model.h"       // Precisamos da definição completa de Model
#include "gltf_loader.h" // Precisamos do loader para carregar o GLB
#include "../core/log.h"
#include "../core/path_utils.h"
#include "../../shared/mmap_format/SceneFileFormat.h"

#include <fstream>

namespace Engine
{
    namespace Asset
    {
        // Define o nome do arquivo binário que o compilador gerou
        const char *ASSET_DICTIONARY_BIN_PATH = "data/assets_dictionary.bin";

        // ------------------------------------------------------------------------
        // FUNÇÃO QUE CARREGA O DICIONÁRIO BINÁRIO (Substitui o hardcode)
        // ------------------------------------------------------------------------

        bool AssetManager::loadAssetDictionary()
        {
            // 1. Resolve o caminho para o arquivo binário
            // Usa path_utils para resolver o caminho da Engine
            std::string fullPath = Engine::resolveEnginePath(ASSET_DICTIONARY_BIN_PATH).string();

            std::ifstream file(fullPath, std::ios::binary | std::ios::in);
            if (!file.is_open())
            {
                Engine::Core::Log::Error(std::format("AssetManager: Falha ao abrir o dicionario binario: {}. Usando Fallback.", fullPath));
                return false;
            }

            // 2. Leitura do Header (Contagem total de entradas)
            uint32_t total_count = 0;
            file.read(reinterpret_cast<char *>(&total_count), sizeof(uint32_t));

            if (total_count == 0)
            {
                Engine::Core::Log::Warn("AssetManager: Dicionario binario vazio.");
                return true;
            }

            // 3. Leitura e Mapeamento das Entradas
            // Nota: O tamanho da AssetEntry é crucial.
            // Assumimos que o compilador escreveu o tamanho correto de AssetEntry.
            std::vector<AssetEntry> entries(total_count);

            file.read(reinterpret_cast<char *>(entries.data()), total_count * sizeof(AssetEntry));

            for (const auto &entry : entries)
            {
                // Mapeamento: ID -> Caminho (string)
                m_assetIDToPathMap[entry.asset_id] = std::string(entry.asset_path);
            }

            Engine::Core::Log::Info(std::format("AssetManager: Dicionario binario carregado com sucesso ({} assets).", total_count));
            return true;
        }

        // =========================================================================
        // 1. CONSTRUTOR (Singleton Inicialização da Tabela de Tradução)
        // =========================================================================

        AssetManager::AssetManager()
        {
            // Tenta carregar o dicionário do arquivo binário
            if (!loadAssetDictionary())
            {
                // Fallback de DEBUG: Se o binário não for encontrado, usa o hardcode como último recurso.
                Engine::Core::Log::Warn("AssetManager: Usando dicionario de assets de fallback (hardcoded).");

                // Coloque aqui os IDs mínimos essenciais para que o jogo não quebre em DEV.
                m_assetIDToPathMap[614879287] = "character_placeholder.glb";
                m_assetIDToPathMap[3665308213] = "wall_module_placeholder.glb";
                m_assetIDToPathMap[3574192723] = "test_scene_1.glb";
                m_assetIDToPathMap[0] = "fallback_asset.glb";
            }
        }

        uint32_t AssetManager::getAssetIDByName(const std::string &assetName)
        {
            // Esta função simula o que o mmap_compiler faz para obter o ID numérico
            std::hash<std::string> hasher;
            return static_cast<uint32_t>(hasher(assetName));
        }

        // 2. Implementação do Singleton (Ponto de Acesso Global)
        AssetManager &AssetManager::Get()
        {
            static AssetManager instance;
            return instance;
        }

        // =========================================================================
        // 3. CORE LOGIC: TRADUÇÃO DE ID
        // =========================================================================

        // Função que traduz ID numérico -> Caminho do Arquivo (Fonte da Verdade)
        std::string AssetManager::getAssetPathByID(uint32_t assetID)
        {
            if (m_assetIDToPathMap.count(assetID))
            {
                return m_assetIDToPathMap.at(assetID);
            }

            Engine::Core::Log::Error(std::format("AssetManager: ID de Asset {} nao encontrado. Usando ID 0.", assetID));

            // Fallback para ID 0 (que deve ser o fallback_asset.glb)
            if (m_assetIDToPathMap.count(0))
            {
                return m_assetIDToPathMap.at(0);
            }

            return ""; // Retorna vazio, que será tratado como erro no getModel
        }

        // Implementação de Verificação de Existência (API Simples)
        bool AssetManager::isAssetAvailable(uint32_t assetID)
        {
            // Verifica se o ID está presente no dicionário de tradução.
            // Se o ID tem um caminho, ele é considerado "disponível" para carregamento.
            return m_assetIDToPathMap.count(assetID);
        }

        // =========================================================================
        // 4. CORE LOGIC: CARREGAMENTO DE MODELOS (Recebe o ID)
        // =========================================================================

        std::shared_ptr<Model> AssetManager::getModel(uint32_t assetID)
        {
            // 1. Traduz o ID numérico para o nome do arquivo
            std::string assetName = getAssetPathByID(assetID);

            if (assetName.empty())
            {
                Engine::Core::Log::Error(std::format("AssetManager: Falha ao obter caminho para ID {}.", assetID));
                return nullptr;
            }

            // 2. Checar Cache (A chave de busca no cache ainda é o nome do arquivo)
            if (m_modelCache.count(assetName))
            {
                Engine::Core::Log::Trace(std::format("AssetManager: Servindo '{}' (ID: {}) do cache.", assetName, assetID));
                return m_modelCache.at(assetName);
            }

            // 3. Carregar do Disco (Lógica de disco/path)
            std::string fullPath = "assets/models/" + assetName;
            auto loadedModel = GLTFLoader::loadGLTF(fullPath);

            if (loadedModel)
            {
                Engine::Core::Log::Info(std::format("AssetManager: Carregando '{}' do disco. Adicionado ao cache.", assetName));

                // Armazenar no Cache (shared_ptr assume o unique_ptr)
                std::shared_ptr<Model> sharedModel(loadedModel.release());
                m_modelCache[assetName] = sharedModel;

                return sharedModel;
            }

            Engine::Core::Log::Error(std::format("AssetManager: Falha ao carregar asset '{}' (ID: {}).", assetName, assetID));
            return nullptr;
        }

    } // namespace Asset
} // namespace Engine