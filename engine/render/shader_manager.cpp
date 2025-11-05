// engine/render/shader_manager.cpp
#include "shader_manager.h"
#include "shader.h" // Assumindo que a classe Shader está definida aqui
#include "../core/log.h"
#include "../core/path_utils.h"
#include "../../asset/asset_manager.h" // NOVO: Para obter o AssetID (Hash)
#include <nlohmann/json.hpp>           // NOVO: Para leitura do JSON
#include <format>
#include <functional> // Para std::hash

using nlohmann::json;

namespace Engine
{
    namespace Render
    {
        ShaderManager::ShaderManager() = default;

        ShaderManager &ShaderManager::Get()
        {
            static ShaderManager instance;
            return instance;
        }

        void ShaderManager::initialize(Engine::Core::ConfigManager &config)
        {
            // Usa o nó raiz para acessar o array 'utility_shaders'
            const json &rootNode = config.getRootNode();

            if (!rootNode.contains("rendering"))
            {
                Engine::Core::Log::Critical("[ShaderManager] Seção 'rendering' não encontrada no JSON de configuração.");
                return;
            }
            const json &rendering = rootNode["rendering"];

            // -------------------------------------------------------------
            // 1. CARREGAMENTO DO SHADER PADRÃO
            // -------------------------------------------------------------
            std::string vertPath = config.getValue<std::string>("rendering.default_shader.vertex_path", "engine/shaders/basic.vert");
            std::string fragPath = config.getValue<std::string>("rendering.default_shader.fragment_path", "engine/shaders/basic.frag");
            std::string name = config.getValue<std::string>("rendering.default_shader.name", "AnimatedPBR"); // Usar o nome do JSON, com fallback

            uint32_t shaderID = loadShader(name, vertPath, fragPath);

            if (shaderID != 0)
            {
                m_activeShaderID = shaderID;
                Engine::Core::Log::Info(std::format("[ShaderManager] Shader Padrão '{}' (ID: {}) carregado e ativado.", name, shaderID));
            }
            else
            {
                Engine::Core::Log::Critical("[ShaderManager] Falha ao carregar shader padrão do JSON.");
            }

            // -------------------------------------------------------------
            // NOVO: 2. CARREGAMENTO DE SHADERS DE UTILIDADE/DEBUG (Armature)
            // -------------------------------------------------------------
            if (rendering.contains("utility_shaders") && rendering["utility_shaders"].is_array())
            {
                for (const auto &shaderConfig : rendering["utility_shaders"])
                {
                    try
                    {
                        // Leitura dos dados do array
                        std::string util_name = shaderConfig.at("name").get<std::string>();
                        std::string util_vs_path = shaderConfig.at("vertex_path").get<std::string>();
                        std::string util_fs_path = shaderConfig.at("fragment_path").get<std::string>();

                        uint32_t util_shaderID = loadShader(util_name, util_vs_path, util_fs_path);

                        if (util_shaderID != 0)
                        {
                            Engine::Core::Log::Info(std::format("[ShaderManager] Shader de utilidade '{}' (ID: {}) carregado.", util_name, util_shaderID));
                        }
                        else
                        {
                            Engine::Core::Log::Error(std::format("[ShaderManager] Falha ao compilar shader de utilidade: {}", util_name));
                        }
                    }
                    catch (const nlohmann::json::exception &e)
                    {
                        // Captura erros se o formato do JSON estiver errado (ex: faltando 'name')
                        Engine::Core::Log::Critical(std::format("[ShaderManager] Erro de parse no JSON (utility_shaders): {}", e.what()));
                    }
                }
            }
        }

        uint32_t ShaderManager::loadShader(const std::string &name, const std::string &vertPath, const std::string &fragPath)
        {
            if (m_nameToIDMap.count(name))
                return m_nameToIDMap.at(name);

            try
            {
                // Usamos PathUtils para resolver caminhos corretamente (o que já estava em seu código)
                std::string vs_path = Engine::resolveEnginePath(vertPath).string();
                std::string fs_path = Engine::resolveEnginePath(fragPath).string();

                auto shader = std::make_unique<Shader>(vs_path.c_str(), fs_path.c_str());
                uint32_t id = static_cast<uint32_t>(std::hash<std::string>{}(name));

                m_shaders[id] = std::move(shader);
                m_nameToIDMap[name] = id;
                return id;
            }
            catch (const std::exception &e)
            {
                Engine::Core::Log::Error(std::format("[ShaderManager] Erro ao carregar shader '{}': {}", name, e.what()));
                return 0;
            }
        }

        Shader &ShaderManager::getShader(uint32_t shaderID)
        {
            if (m_shaders.count(shaderID))
            {
                return *m_shaders.at(shaderID);
            }
            // ESTE É O PONTO DE CRASH: Tentar acessar um shader inexistente.
            throw std::runtime_error(std::format("Shader com ID {} não encontrado. Possível falha de carregamento.", shaderID));
        }

        uint32_t ShaderManager::getShaderIDByName(const std::string &name) const
        {
            if (m_nameToIDMap.count(name))
            {
                return m_nameToIDMap.at(name);
            }
            return 0; // Retorna 0 (ID de shader inválido) se não encontrado
        }

        Shader &ShaderManager::getActiveShader()
        {
            return getShader(m_activeShaderID);
        }
    }
}