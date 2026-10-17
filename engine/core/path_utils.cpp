// engine/core/path_utils.cpp
#include "path_utils.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
// Inclua o log aqui também para usar Engine::Log
#include "./log.h" // caminho relativo correto para o log

namespace Engine
{ // **** NOVO: NAMESPACE ENGINE ****

    namespace {

    std::filesystem::path findExistingPathFromCurrent(const std::string &relativePath)
    {
        std::filesystem::path probe = std::filesystem::current_path();
        for (int depth = 0; depth < 8; ++depth)
        {
            const std::filesystem::path candidate = probe / relativePath;
            if (std::filesystem::exists(candidate))
            {
                return candidate;
            }

            const std::filesystem::path parent = probe.parent_path();
            if (parent == probe || parent.empty())
            {
                break;
            }
            probe = parent;
        }

        return {};
    }

    } // namespace

    std::filesystem::path resolveEnginePath(const std::string &relativePath)
    {
        const std::filesystem::path fullPath = findExistingPathFromCurrent(relativePath);

        if (fullPath.empty())
        {
            Engine::Core::Log::Error(std::format("PathUtils: Arquivo não encontrado para '{}', cwd='{}'", relativePath, std::filesystem::current_path().string()));
            throw std::runtime_error("Arquivo não encontrado: " + relativePath);
        }

        return fullPath;
    }

    std::string loadFileFromEngineAssets(const std::string &relativePath)
    {
        auto fullPath = resolveEnginePath(relativePath); // resolveEnginePath também está em Engine::

        std::ifstream file(fullPath);
        if (!file.is_open())
        {
            Engine::Core::Log::Error(std::format("PathUtils: Erro ao abrir arquivo: {}", fullPath.string()));
            throw std::runtime_error("Erro ao abrir arquivo: " + fullPath.string());
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();

        Engine::Core::Log::Info(std::format("PathUtils: Arquivo carregado de: {} ({} bytes)", fullPath.string(), buffer.str().size()));

        return buffer.str();
    }

    std::filesystem::path resolveEnginePathForWrite(const std::string &relativePath)
    {
        std::filesystem::path probe = std::filesystem::current_path();
        for (int depth = 0; depth < 8; ++depth)
        {
            const std::filesystem::path configCandidate = probe / "config" / "engine_settings.json";
            if (std::filesystem::exists(configCandidate))
            {
                return probe / relativePath;
            }

            const std::filesystem::path parent = probe.parent_path();
            if (parent == probe || parent.empty())
            {
                break;
            }
            probe = parent;
        }

        return std::filesystem::current_path() / relativePath;
    }

} // namespace Engine