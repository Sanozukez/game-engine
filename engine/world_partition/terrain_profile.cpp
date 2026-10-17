#include "terrain_profile.h"

#include "core/log.h"

#include <fstream>
#include <format>

#include <nlohmann/json.hpp>

namespace Engine::WorldPartition {

namespace {
TerrainProfile BuildDefaultProfile() {
    TerrainProfile profile;
    profile.minHeightMeters = -200.0f;
    profile.maxHeightMeters = 1800.0f;
    profile.zeroLevelMeters = 0.0f;
    profile.bitDepth = 16;
    profile.normalizationMode = "global_range_linear";
    profile.grid = WorldGridConfig{};
    return profile;
}

nlohmann::json ToJson(const TerrainProfile& profile) {
    return {
        {"terrain_profile", {
            {"min_height_m", profile.minHeightMeters},
            {"max_height_m", profile.maxHeightMeters},
            {"zero_level_m", profile.zeroLevelMeters},
            {"bit_depth", profile.bitDepth},
            {"normalization_mode", profile.normalizationMode}
        }},
        {"world_grid", {
            {"macro_columns", profile.grid.macroColumns},
            {"macro_rows", profile.grid.macroRows},
            {"chunks_per_macro_side", profile.grid.chunksPerMacroSide},
            {"chunk_size_m", profile.grid.chunkSizeMeters},
            {"clamp_to_defined_grid", profile.grid.clampToDefinedGrid}
        }}
    };
}
} // namespace

bool EnsureTerrainProfileFileExists(const std::filesystem::path& filePath) {
    if (std::filesystem::exists(filePath)) {
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(filePath.parent_path(), ec);
    if (ec) {
        Engine::Core::Log::Warn(std::format("[TerrainProfile] Falha ao criar diretorio para profile: {}", filePath.parent_path().string()));
        return false;
    }

    std::ofstream out(filePath);
    if (!out.is_open()) {
        Engine::Core::Log::Warn(std::format("[TerrainProfile] Falha ao criar arquivo: {}", filePath.string()));
        return false;
    }

    out << ToJson(BuildDefaultProfile()).dump(4);
    out << "\n";
    return true;
}

TerrainProfile LoadTerrainProfileFromFile(const std::filesystem::path& filePath) {
    TerrainProfile profile = BuildDefaultProfile();

    if (!std::filesystem::exists(filePath)) {
        EnsureTerrainProfileFileExists(filePath);
        return profile;
    }

    std::ifstream in(filePath);
    if (!in.is_open()) {
        Engine::Core::Log::Warn(std::format("[TerrainProfile] Nao foi possivel abrir arquivo: {}", filePath.string()));
        return profile;
    }

    nlohmann::json data;
    try {
        in >> data;
    } catch (const std::exception& e) {
        Engine::Core::Log::Warn(std::format("[TerrainProfile] JSON invalido em {}: {}", filePath.string(), e.what()));
        return profile;
    }

    if (data.contains("terrain_profile")) {
        const auto& node = data["terrain_profile"];
        profile.minHeightMeters = node.value("min_height_m", profile.minHeightMeters);
        profile.maxHeightMeters = node.value("max_height_m", profile.maxHeightMeters);
        profile.zeroLevelMeters = node.value("zero_level_m", profile.zeroLevelMeters);
        profile.bitDepth = node.value("bit_depth", profile.bitDepth);
        profile.normalizationMode = node.value("normalization_mode", profile.normalizationMode);
    }

    if (data.contains("world_grid")) {
        const auto& node = data["world_grid"];
        profile.grid.macroColumns = node.value("macro_columns", profile.grid.macroColumns);
        profile.grid.macroRows = node.value("macro_rows", profile.grid.macroRows);
        profile.grid.chunksPerMacroSide = node.value("chunks_per_macro_side", profile.grid.chunksPerMacroSide);
        profile.grid.chunkSizeMeters = node.value("chunk_size_m", profile.grid.chunkSizeMeters);
        profile.grid.clampToDefinedGrid = node.value("clamp_to_defined_grid", profile.grid.clampToDefinedGrid);
    }

    if (profile.maxHeightMeters <= profile.minHeightMeters) {
        profile.maxHeightMeters = profile.minHeightMeters + 1.0f;
    }
    if (profile.bitDepth <= 0) {
        profile.bitDepth = 16;
    }
    if (profile.grid.macroColumns <= 0) {
        profile.grid.macroColumns = 1;
    }
    if (profile.grid.macroRows <= 0) {
        profile.grid.macroRows = 1;
    }
    if (profile.grid.chunksPerMacroSide <= 0) {
        profile.grid.chunksPerMacroSide = 1;
    }
    if (profile.grid.chunkSizeMeters <= 0.001f) {
        profile.grid.chunkSizeMeters = 250.0f;
    }

    return profile;
}

std::filesystem::path ResolveTerrainProfilePath(const std::filesystem::path& baseDirectory) {
    const std::filesystem::path defaultPath = baseDirectory / "project" / "config" / "terrain_profile.json";
    const std::filesystem::path settingsPath = baseDirectory / "config" / "engine_settings.json";

    if (!std::filesystem::exists(settingsPath)) {
        return defaultPath;
    }

    std::ifstream in(settingsPath);
    if (!in.is_open()) {
        return defaultPath;
    }

    nlohmann::json settings;
    try {
        in >> settings;
    } catch (const std::exception&) {
        return defaultPath;
    }

    if (!settings.contains("world") || !settings["world"].is_object()) {
        return defaultPath;
    }

    const auto& world = settings["world"];
    if (!world.contains("terrain_profile_path") || !world["terrain_profile_path"].is_string()) {
        return defaultPath;
    }

    const std::string configuredPath = world["terrain_profile_path"].get<std::string>();
    if (configuredPath.empty()) {
        return defaultPath;
    }

    const std::filesystem::path cfgPath(configuredPath);
    if (cfgPath.is_absolute()) {
        return cfgPath;
    }

    return (baseDirectory / cfgPath).lexically_normal();
}

} // namespace Engine::WorldPartition
