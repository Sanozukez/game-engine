// engine/core/path_utils.cpp
#include "path_utils.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
// Inclua o log aqui também para usar Engine::Log
#include "./log.h" // caminho relativo correto para o log

namespace Engine { // **** NOVO: NAMESPACE ENGINE ****

std::filesystem::path resolveEnginePath(const std::string& relativePath) {
    // Caminho do executável
    auto execPath = std::filesystem::current_path();

    // Sobe até o root do projeto (assumindo estrutura de build em build/src/Debug)
    // Cuidado: Isso ainda é frágil se a estrutura de build mudar.
    // Uma variável de ambiente ou parâmetro de linha de comando seria mais robusto.
    auto projectRoot = execPath.parent_path().parent_path().parent_path();

    auto fullPath = projectRoot / relativePath;

    if (!std::filesystem::exists(fullPath)) {
        Engine::Log::Error(std::format("PathUtils: Arquivo não encontrado: {}", fullPath.string()));
        throw std::runtime_error("Arquivo não encontrado: " + fullPath.string());
    }

    return fullPath;
}

std::string loadFileFromEngineAssets(const std::string& relativePath) {
    auto fullPath = resolveEnginePath(relativePath); // resolveEnginePath também está em Engine::

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        Engine::Log::Error(std::format("PathUtils: Erro ao abrir arquivo: {}", fullPath.string()));
        throw std::runtime_error("Erro ao abrir arquivo: " + fullPath.string());
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    Engine::Log::Info(std::format("PathUtils: Arquivo carregado de: {} ({} bytes)", fullPath.string(), buffer.str().size()));

    return buffer.str();
}

} // namespace Engine