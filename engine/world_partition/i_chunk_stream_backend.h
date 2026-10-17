#pragma once

#include "world_partition_types.h"

namespace Engine::WorldPartition {

class IChunkStreamBackend {
public:
    virtual ~IChunkStreamBackend() = default;

    virtual void requestLoad(const ChunkCoord& chunk) = 0;
    virtual void requestUnload(const ChunkCoord& chunk) = 0;
};

class NoopChunkStreamBackend final : public IChunkStreamBackend {
public:
    void requestLoad(const ChunkCoord& chunk) override {
        (void)chunk;
    }

    void requestUnload(const ChunkCoord& chunk) override {
        (void)chunk;
    }
};

} // namespace Engine::WorldPartition
