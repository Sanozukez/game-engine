#pragma once

#include "world_partition_index.h"

#include <filesystem>
#include <string>

namespace Engine::WorldPartition {

struct TerrainProfile {
    float minHeightMeters = -200.0f;
    float maxHeightMeters = 1800.0f;
    float zeroLevelMeters = 0.0f;
    int bitDepth = 16;
    std::string normalizationMode = "global_range_linear";
    WorldGridConfig grid{};
};

bool EnsureTerrainProfileFileExists(const std::filesystem::path& filePath);
TerrainProfile LoadTerrainProfileFromFile(const std::filesystem::path& filePath);
std::filesystem::path ResolveTerrainProfilePath(const std::filesystem::path& baseDirectory);

} // namespace Engine::WorldPartition
