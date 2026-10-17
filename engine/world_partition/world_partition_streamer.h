#pragma once

#include "world_partition_index.h"

#include <unordered_set>
#include <vector>

namespace Engine::WorldPartition {

struct StreamUpdateResult {
    ChunkCoord activeChunk{};
    std::vector<ChunkCoord> toLoad;
    std::vector<ChunkCoord> toUnload;
};

class ChunkStreamer {
public:
    ChunkStreamer(int preloadRadius = 1, int retentionRadius = 2);

    StreamUpdateResult update(const ChunkCoord& centerChunk, const WorldIndex& index);

    const std::unordered_set<ChunkCoord, ChunkCoordHash>& loadedChunks() const;
    ChunkCoord activeChunk() const;

private:
    int m_preloadRadius = 1;
    int m_retentionRadius = 2;
    ChunkCoord m_activeChunk{};
    std::unordered_set<ChunkCoord, ChunkCoordHash> m_loadedChunks;
};

} // namespace Engine::WorldPartition
