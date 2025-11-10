// // engine/asset/asset_manager.cpp

#include "asset_manager.h"
#include "model.h"       // Precisamos da definição completa de Model
#include "gltf_loader.h" // Precisamos do loader para carregar o GLB
#include "../core/log.h"
#include "../core/path_utils.h"
#include "../../shared/mmap_format/SceneFileFormat.h" // AssetEntry, AnimationMapping, Header

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
            std::string fullPath = Engine::resolveEnginePath(ASSET_DICTIONARY_BIN_PATH).string();

            std::ifstream file(fullPath, std::ios::binary | std::ios::in);
            if (!file.is_open())
            {
                Engine::Core::Log::Error(std::format("AssetManager: Falha ao abrir o dicionario binario: {}. Usando Fallback.", fullPath));
                return false;
            }

            // 2. NOVO v101: Ler AssetDictionaryHeader
            AssetDictionaryHeader header = {};
            file.read(reinterpret_cast<char*>(&header), sizeof(AssetDictionaryHeader));

            // Validar magic number
            if (header.magic != 0x41535444) // "ASTD"
            {
                Engine::Core::Log::Error(std::format("AssetManager: Magic number invalido! Esperado 0x41535444, recebido 0x{:08X}", header.magic));
                return false;
            }

            // Validar versão
            if (header.version != 101)
            {
                Engine::Core::Log::Warn(std::format("AssetManager: Versao do dicionario {} nao suportada! Esperado 101.", header.version));
                // Poderia tentar ler v100 aqui no futuro (backward compatibility)
                return false;
            }

            Engine::Core::Log::Info(std::format("AssetManager: Carregando dicionario v{} ({} assets)...", 
                                                header.version, header.asset_count));

            if (header.asset_count == 0)
            {
                Engine::Core::Log::Warn("AssetManager: Dicionario binario vazio.");
                return true;
            }

            // 3. Ler AssetEntry[] (v101: 152 bytes cada)
            std::vector<AssetEntry> entries(header.asset_count);
            file.read(reinterpret_cast<char*>(entries.data()), header.asset_count * sizeof(AssetEntry));

            for (const auto &entry : entries)
            {
                // Mapeamento: ID -> Caminho
                m_assetIDToPathMap[entry.asset_id] = std::string(entry.asset_path);
            }

            // 4. NOVO v101: Ler AnimationMapping[] section (se existir)
            if (header.animation_section_offset > 0)
            {
                file.seekg(header.animation_section_offset, std::ios::beg);

                // Calcular quantas AnimationMapping existem (total de todos os assets)
                size_t total_anim_mappings = 0;
                for (const auto& entry : entries)
                {
                    total_anim_mappings += entry.animation_count;
                }

                if (total_anim_mappings > 0)
                {
                    std::vector<AnimationMapping> all_animations(total_anim_mappings);
                    file.read(reinterpret_cast<char*>(all_animations.data()), 
                             total_anim_mappings * sizeof(AnimationMapping));

                    // Popular m_animationMappings (hash -> AnimationMapping)
                    for (const auto& anim : all_animations)
                    {
                        m_animationMappings[anim.engine_name_hash] = anim;
                    }

                    Engine::Core::Log::Info(std::format("  -> {} animation mappings carregados", total_anim_mappings));
                }
            }

            Engine::Core::Log::Info(std::format("AssetManager: Dicionario v101 carregado com sucesso!"));
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
                m_assetIDToPathMap[614879287] = "character_test.glb";
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
                // Engine::Core::Log::Trace(std::format("AssetManager: Servindo '{}' (ID: {}) do cache.", assetName, assetID));
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

        // =========================================================================
        // NOVO v101: ANIMATION MAPPING LOOKUP
        // =========================================================================
        
        const AnimationMapping* AssetManager::getAnimationMapping(uint32_t engineNameHash) const
        {
            auto it = m_animationMappings.find(engineNameHash);
            if (it != m_animationMappings.end())
            {
                return &it->second;
            }
            return nullptr; // Não encontrado
        }

    } // namespace Asset
} // namespace Engine