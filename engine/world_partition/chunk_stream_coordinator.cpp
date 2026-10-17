#include "chunk_stream_coordinator.h"

namespace Engine::WorldPartition {

ChunkStreamCoordinator::ChunkStreamCoordinator(
    IChunkStreamBackend& backend,
    WorldGridConfig gridConfig,
    int preloadRadius,
    int retentionRadius)
    : m_backend(backend),
      m_tracker(gridConfig, preloadRadius, retentionRadius) {
}

RuntimeTickResult ChunkStreamCoordinator::update(const glm::vec3& streamSourceWorldPosition) {
    RuntimeTickResult tick = m_tracker.tick(streamSourceWorldPosition);

    for (const ChunkCoord& chunk : tick.chunksToLoad) {
        m_backend.requestLoad(chunk);
    }

    for (const ChunkCoord& chunk : tick.chunksToUnload) {
        m_backend.requestUnload(chunk);
    }

    return tick;
}

} // namespace Engine::WorldPartition
