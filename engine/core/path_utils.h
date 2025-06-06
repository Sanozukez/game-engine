// engine/core/path_utils.h
#pragma once

#include <string>
#include <filesystem>

namespace Engine { // **** NOVO: NAMESPACE ENGINE ****

std::filesystem::path resolveEnginePath(const std::string& relativePath);
std::string loadFileFromEngineAssets(const std::string& relativePath);

} // namespace Engine