// // engine/asset/asset_manager.cpp

#include "asset_manager.h"
#include "../core/log.h"
#include "gltf_loader.h" // Precisamos do loader para carregar o GLB
#include "model.h"       // Precisamos da definição completa de Model

namespace Engine
{
namespace Asset
{

// =========================================================================
// 1. CONSTRUTOR (Singleton Inicialização da Tabela de Tradução)
// =========================================================================

AssetManager::AssetManager()
{
    // Inicialização Temporária do Dicionário (Hardcode para o teste)
    // Este bloco será substituído pelo carregamento binário do dicionário futuramente.
    m_assetIDToPathMap[3665308213] = "wall_module_placeholder.glb"; // ID do muro
   // 2. TERRENO/MAPA (ID do 'map_test.glb' ou 'test_scene_1.glb')
    m_assetIDToPathMap[2727254143] = "test_scene_1.glb"; // Ou o nome GLB correto do seu mapa
    
    // 3. PERSONAGEM (ID do 'character_placeholder.glb')
    m_assetIDToPathMap[614879287] = "character_placeholder.glb"; 
    
    // 4. FALLBACK
    m_assetIDToPathMap[0] = "fallback_asset.glb"; 

    Engine::Log::Info("AssetManager: Dicionario de Assets temporariamente carregado.");
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

    Engine::Log::Error(std::format("AssetManager: ID de Asset {} nao encontrado. Usando ID 0.", assetID));
    
    // Fallback para ID 0 (que deve ser o fallback_asset.glb)
    if (m_assetIDToPathMap.count(0))
    {
        return m_assetIDToPathMap.at(0);
    }

    return ""; // Retorna vazio, que será tratado como erro no getModel
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
        Engine::Log::Error(std::format("AssetManager: Falha ao obter caminho para ID {}.", assetID));
        return nullptr;
    }

    // 2. Checar Cache (A chave de busca no cache ainda é o nome do arquivo)
    if (m_modelCache.count(assetName))
    {
        Engine::Log::Trace(std::format("AssetManager: Servindo '{}' (ID: {}) do cache.", assetName, assetID));
        return m_modelCache.at(assetName);
    }

    // 3. Carregar do Disco (Lógica de disco/path)
    std::string fullPath = "assets/models/" + assetName; 
    auto loadedModel = GLTFLoader::loadGLTF(fullPath);

    if (loadedModel)
    {
        Engine::Log::Info(std::format("AssetManager: Carregando '{}' do disco. Adicionado ao cache.", assetName));

        // Armazenar no Cache (shared_ptr assume o unique_ptr)
        std::shared_ptr<Model> sharedModel(loadedModel.release());
        m_modelCache[assetName] = sharedModel;

        return sharedModel;
    }

    Engine::Log::Error(std::format("AssetManager: Falha ao carregar asset '{}' (ID: {}).", assetName, assetID));
    return nullptr;
}


} // namespace Asset
} // namespace Engine