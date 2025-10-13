// // engine/core/config_manager.h

#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <regex> // Adicionado para uso dentro do header
#include "log.h" // Adicionado para uso dentro do header

namespace Engine
{

    class ConfigManager
    {
    public:
        static ConfigManager &Get();

        bool load(const std::string &filepath);
        bool save();

        // --- MUDANÇA: A IMPLEMENTAÇÃO DO TEMPLATE AGORA ESTÁ NO .h ---
        template <typename T>
        T getValue(const std::string &key, const T &defaultValue) const
        {
            try
            {
                std::string json_ptr_key = "/" + std::regex_replace(key, std::regex("\\."), "/");
                return m_configData.at(nlohmann::json::json_pointer(json_ptr_key)).get<T>();
            }
            catch (const nlohmann::json::exception &e)
            {
                Log::Warn(std::format("ConfigManager: Chave '{}' não encontrada ou tipo incorreto. Usando valor padrão. Erro: {}", key, e.what()));
                return defaultValue;
            }
        }

        template <typename T>
        void setValue(const std::string &key, const T &value)
        {
            try
            {
                std::string json_ptr_key = "/" + std::regex_replace(key, std::regex("\\."), "/");
                m_configData[nlohmann::json::json_pointer(json_ptr_key)] = value;
            }
            catch (const nlohmann::json::exception &e)
            {
                Log::Error(std::format("ConfigManager: Erro ao definir a chave '{}'. Erro: {}", key, e.what()));
            }
        }

        // NOVO: Retorna a referência ao nó de configuração base para navegação (API de baixo nível)
        const nlohmann::json &getRootNode() const { return m_configData; }

    private:
        ConfigManager() = default;
        ~ConfigManager() = default;
        ConfigManager(const ConfigManager &) = delete;
        ConfigManager &operator=(const ConfigManager &) = delete;

        nlohmann::json m_configData;
        std::string m_filepath;
    };

} // namespace Engine