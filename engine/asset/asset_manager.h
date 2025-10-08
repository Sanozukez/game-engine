// // engine/asset/asset_manager.h (CORRIGIDO)

#pragma once

#include <memory>
#include <string>
#include <map>
#include <cstdint>

// Forward Declaration: Para evitar dependência circular e include pesado
namespace Engine::Asset
{
    class Model;
}
namespace Engine::Render
{
    class Texture;
}

namespace Engine
{
    namespace Asset
    {

        class AssetManager
        {
        private:
            // A. Cache central de modelos (Chave: Nome do Arquivo)
            std::map<std::string, std::shared_ptr<Model>> m_modelCache;

            // B. TRADUÇÃO (O Mapa de Tradução de ID -> Path)
            // Usado para traduzir o ID numérico do MMAP para a string real do GLB.
            std::map<uint32_t, std::string> m_assetIDToPathMap;

            // Métodos Singleton e de utilidade
            AssetManager();
            ~AssetManager() = default;

            // Remove cópia para garantir que só haja uma instância
            AssetManager(const AssetManager &) = delete;
            AssetManager &operator=(const AssetManager &) = delete;

            // NOVO: Função que carrega o dicionário binário na inicialização
            bool loadAssetDictionary(); // <-- DECLARAÇÃO ESSENCIAL

        public:
            // Ponto de acesso global ao Singleton
            static AssetManager &Get();

            // 1. MÉTODO PRINCIPAL: Recebe o ID Otimizado (uint32_t)
            std::shared_ptr<Model> getModel(uint32_t assetID);

            // 2. FUNÇÃO CORE: Traduz ID numérico -> Nome do arquivo
            std::string getAssetPathByID(uint32_t assetID);

            // 3. Helper de Dev (usado pelo WorldInitializer)
            uint32_t getAssetIDByName(const std::string &assetName);

            // (Futuro: Adicionar getTexture e getAudio)
        };

    } // namespace Asset
} // namespace Engine