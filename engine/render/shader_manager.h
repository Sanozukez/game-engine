// // engine/render/shader_manager.h
#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "shader.h" // Depende da interface do Shader

namespace Engine
{
namespace Render
{
    // O ShaderManager tem a responsabilidade única (SRP) de carregar, compilar e armazenar Shaders.
    class ShaderManager
    {
    public:
        // Singleton (Acesso Global)
        static ShaderManager& Get();
        
        // Carrega um shader a partir de caminhos e retorna seu ID numérico (hash)
        uint32_t loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);

        // Retorna uma referência ao Shader (Interface DIP)
        Shader& getShader(uint32_t shaderID);
        
        // Retorna o ID do shader pelo nome (utility)
        uint32_t getShaderIDByName(const std::string& name) const;

    private:
        ShaderManager();
        ~ShaderManager() = default;

        // Armazena os objetos Shader (o ID aqui é o Hash do nome do shader)
        std::unordered_map<uint32_t, std::unique_ptr<Shader>> m_shaders;
        
        // Mapeia nome legível do shader para o ID Hash (ex: "Animated" -> 12345)
        std::unordered_map<std::string, uint32_t> m_nameToIDMap;
    };
}
}