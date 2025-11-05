// // engine/render/shader_manager.h
#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "shader.h" // Depende da interface do Shader
// NOVO: Incluir ConfigManager aqui para que o construtor possa usar o tipo
#include "../core/config_manager.h" 

namespace Engine
{
namespace Render
{
    class ShaderManager
    {
    public:
        // Singleton (Acesso Global)
        static ShaderManager& Get();
        
        // NOVO: Inicializa o manager (lê do JSON de configuração)
        void initialize(Engine::Core::ConfigManager& config);
        
        // Carrega um shader a partir de caminhos e retorna seu ID numérico (hash)
        uint32_t loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);

        // Retorna uma referência ao Shader (Interface DIP)
        Shader& getShader(uint32_t shaderID);
        
        // Retorna o ID do shader pelo nome (utility)
        uint32_t getShaderIDByName(const std::string& name) const;
        
        // NOVO: Retorna o shader ativo (por ID ou nome)
        Shader& getActiveShader(); // <--- DECLARAÇÃO CORRIGIDA

    private:
        ShaderManager();
        ~ShaderManager() = default;

        std::unordered_map<uint32_t, std::unique_ptr<Shader>> m_shaders;
        std::unordered_map<std::string, uint32_t> m_nameToIDMap;
        
        uint32_t m_activeShaderID = 0; // <--- NOVO MEMBRO
    };
}
}