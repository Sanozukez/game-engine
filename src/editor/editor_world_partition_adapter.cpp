#include "editor_world_partition_adapter.h"

namespace Editor {

EditorWorldPartitionAdapter::EditorWorldPartitionAdapter()
    : m_index(Engine::WorldPartition::WorldGridConfig{}),
      m_streamer(1, 2) {
}

void EditorWorldPartitionAdapter::updateFromCamera(const glm::vec3& cameraWorldPosition) {
    const Engine::WorldPartition::ChunkCoord cameraChunk = m_index.resolveChunkFromWorldPosition(cameraWorldPosition);
    m_streamer.update(cameraChunk, m_index);
}

void EditorWorldPartitionAdapter::setGridConfig(const Engine::WorldPartition::WorldGridConfig& config) {
    m_index = Engine::WorldPartition::WorldIndex(config);
}

Engine::WorldPartition::ChunkCoord EditorWorldPartitionAdapter::activeChunk() const {
    return m_streamer.activeChunk();
}

Engine::WorldPartition::ChunkCoord EditorWorldPartitionAdapter::resolveChunkFromWorldPosition(const glm::vec3& worldPosition) const {
    return m_index.resolveChunkFromWorldPosition(worldPosition);
}

const Engine::WorldPartition::WorldGridConfig& EditorWorldPartitionAdapter::gridConfig() const {
    return m_index.config();
}

std::string EditorWorldPartitionAdapter::activeChunkLabel() const {
    return Engine::WorldPartition::ToChunkLabel(m_streamer.activeChunk());
}

int EditorWorldPartitionAdapter::loadedChunkCount() const {
    return static_cast<int>(m_streamer.loadedChunks().size());
}

bool EditorWorldPartitionAdapter::isObjectInsideActiveChunk(const Engine::WorldPartition::ChunkCoord& objectChunk) const {
    return objectChunk == m_streamer.activeChunk();
}

} // namespace Editor
