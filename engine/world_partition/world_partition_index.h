#pragma once

#include "world_partition_types.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

namespace Engine::WorldPartition {

struct WorldGridConfig {
    int macroColumns = 4;
    int macroRows = 8;
    int chunksPerMacroSide = 4;
    float chunkSizeMeters = 250.0f;
    bool clampToDefinedGrid = true;

    constexpr int maxChunkX() const {
        return (macroColumns * chunksPerMacroSide) - 1;
    }

    constexpr int maxChunkZ() const {
        return (macroRows * chunksPerMacroSide) - 1;
    }
};

class WorldIndex {
public:
    explicit WorldIndex(WorldGridConfig config = {})
        : m_config(config) {
    }

    const WorldGridConfig& config() const {
        return m_config;
    }

    bool isValidChunk(const ChunkCoord& coord) const {
        if (coord.x < 0 || coord.z < 0) {
            return false;
        }
        return coord.x <= m_config.maxChunkX() && coord.z <= m_config.maxChunkZ();
    }

    ChunkCoord resolveChunkFromWorldPosition(const glm::vec3& worldPosition) const {
        const float invChunkSize = 1.0f / std::max(0.001f, m_config.chunkSizeMeters);
        int chunkX = static_cast<int>(std::floor(worldPosition.x * invChunkSize));
        int chunkZ = static_cast<int>(std::floor(worldPosition.z * invChunkSize));

        if (m_config.clampToDefinedGrid) {
            chunkX = std::clamp(chunkX, 0, m_config.maxChunkX());
            chunkZ = std::clamp(chunkZ, 0, m_config.maxChunkZ());
        }

        return ChunkCoord{chunkX, chunkZ};
    }

    glm::vec3 chunkOriginWorld(const ChunkCoord& coord) const {
        return glm::vec3(
            static_cast<float>(coord.x) * m_config.chunkSizeMeters,
            0.0f,
            static_cast<float>(coord.z) * m_config.chunkSizeMeters);
    }

private:
    WorldGridConfig m_config;
};

} // namespace Engine::WorldPartition
