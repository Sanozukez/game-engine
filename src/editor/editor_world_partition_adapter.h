#pragma once

#include "world_partition/world_partition_index.h"
#include "world_partition/world_partition_streamer.h"

#include <string>

#include <glm/glm.hpp>

namespace Editor {

class EditorWorldPartitionAdapter {
public:
    EditorWorldPartitionAdapter();

    void updateFromCamera(const glm::vec3& cameraWorldPosition);
    void setGridConfig(const Engine::WorldPartition::WorldGridConfig& config);

    Engine::WorldPartition::ChunkCoord activeChunk() const;
    Engine::WorldPartition::ChunkCoord resolveChunkFromWorldPosition(const glm::vec3& worldPosition) const;
    const Engine::WorldPartition::WorldGridConfig& gridConfig() const;
    std::string activeChunkLabel() const;
    int loadedChunkCount() const;

    bool isObjectInsideActiveChunk(const Engine::WorldPartition::ChunkCoord& objectChunk) const;

private:
    Engine::WorldPartition::WorldIndex m_index;
    Engine::WorldPartition::ChunkStreamer m_streamer;
};

} // namespace Editor
