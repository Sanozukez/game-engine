#include "world_partition_runtime_tracker.h"

namespace Engine::WorldPartition {

RuntimeWorldPartitionTracker::RuntimeWorldPartitionTracker(
    WorldGridConfig gridConfig,
    int preloadRadius,
    int retentionRadius)
    : m_index(gridConfig),
      m_streamer(preloadRadius, retentionRadius) {
}

RuntimeTickResult RuntimeWorldPartitionTracker::tick(const glm::vec3& cameraWorldPosition) {
    RuntimeTickResult result{};

    const ChunkCoord currentChunk = m_index.resolveChunkFromWorldPosition(cameraWorldPosition);
    const StreamUpdateResult stream = m_streamer.update(currentChunk, m_index);

    result.activeChunk = stream.activeChunk;
    result.chunksToLoad = stream.toLoad;
    result.chunksToUnload = stream.toUnload;

    if (!m_hasActiveChunk || !(m_lastActiveChunk == stream.activeChunk)) {
        result.activeChunkChanged = true;
        m_lastActiveChunk = stream.activeChunk;
        m_hasActiveChunk = true;
    }

    return result;
}

ChunkCoord RuntimeWorldPartitionTracker::activeChunk() const {
    return m_streamer.activeChunk();
}

std::string RuntimeWorldPartitionTracker::activeChunkLabel() const {
    return ToChunkLabel(m_streamer.activeChunk());
}

} // namespace Engine::WorldPartition
