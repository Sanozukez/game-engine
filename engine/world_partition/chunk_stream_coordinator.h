#pragma once

#include "i_chunk_stream_backend.h"
#include "world_partition_runtime_tracker.h"

#include <glm/glm.hpp>

namespace Engine::WorldPartition {

class ChunkStreamCoordinator {
public:
    ChunkStreamCoordinator(
        IChunkStreamBackend& backend,
        WorldGridConfig gridConfig = {},
        int preloadRadius = 1,
        int retentionRadius = 2);

    RuntimeTickResult update(const glm::vec3& streamSourceWorldPosition);

private:
    IChunkStreamBackend& m_backend;
    RuntimeWorldPartitionTracker m_tracker;
};

} // namespace Engine::WorldPartition
