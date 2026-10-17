#pragma once

#include "world_partition_index.h"
#include "world_partition_streamer.h"

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Engine::WorldPartition {

struct RuntimeTickResult {
    ChunkCoord activeChunk{};
    bool activeChunkChanged = false;
    std::vector<ChunkCoord> chunksToLoad;
    std::vector<ChunkCoord> chunksToUnload;
};

class RuntimeWorldPartitionTracker {
public:
    RuntimeWorldPartitionTracker(
        WorldGridConfig gridConfig = {},
        int preloadRadius = 1,
        int retentionRadius = 2);

    RuntimeTickResult tick(const glm::vec3& cameraWorldPosition);

    ChunkCoord activeChunk() const;
    std::string activeChunkLabel() const;

private:
    WorldIndex m_index;
    ChunkStreamer m_streamer;
    bool m_hasActiveChunk = false;
    ChunkCoord m_lastActiveChunk{};
};

} // namespace Engine::WorldPartition
