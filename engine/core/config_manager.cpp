// // engine/core/config_manager.cpp

#include "config_manager.h"
#include "log.h"
#include "path_utils.h"
#include <fstream>
#include <sstream>

namespace Engine
{
    namespace Core
    {

        ConfigManager &ConfigManager::Get()
        {
            static ConfigManager instance;
            return instance;
        }

        // --- MÉTODO CORRIGIDO ---
        bool ConfigManager::load(const std::string &relativeFilepath)
        {
            try
            {
                // Usa a sua função utilitária para encontrar o caminho absoluto do arquivo
                auto fullPath = resolveEnginePath(relativeFilepath);
                m_filepath = fullPath.string();

                std::ifstream file(m_filepath);
                if (!file.is_open())
                {
                    Log::Error(std::format("ConfigManager: Não foi possível abrir o arquivo de configuração: {}", m_filepath));
                    m_configData = nlohmann::json::object();
                    return false;
                }

                file >> m_configData;
                Log::Info(std::format("ConfigManager: Configurações carregadas de {}", m_filepath));
            }
            catch (const std::exception &e)
            {
                Log::Error(std::format("ConfigManager: Erro ao carregar ou parsear JSON em {}: {}", m_filepath, e.what()));
                m_configData = nlohmann::json::object();
                return false;
            }
            return true;
        }

        bool ConfigManager::save()
        {
            std::ofstream file(m_filepath);
            if (!file.is_open())
            {
                Log::Error(std::format("ConfigManager: Não foi possível salvar no arquivo de configuração: {}", m_filepath));
                return false;
            }
            file << m_configData.dump(4);
            Log::Info(std::format("ConfigManager: Configurações salvas em {}", m_filepath));
            return true;
        }
    } // namespace Core
} // namespace Engine