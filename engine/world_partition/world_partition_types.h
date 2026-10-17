#pragma once

#include <cstddef>
#include <string>

namespace Engine::WorldPartition {

struct ChunkCoord {
    int x = 0;
    int z = 0;

    constexpr bool operator==(const ChunkCoord& other) const {
        return x == other.x && z == other.z;
    }
};

struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& coord) const {
        return (static_cast<std::size_t>(static_cast<unsigned int>(coord.x)) << 32) ^
               static_cast<std::size_t>(static_cast<unsigned int>(coord.z));
    }
};

inline std::string ToChunkLabel(const ChunkCoord& coord) {
    return "chunk_" + std::to_string(coord.x) + "_" + std::to_string(coord.z);
}

} // namespace Engine::WorldPartition
