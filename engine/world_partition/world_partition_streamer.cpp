#include "world_partition_streamer.h"

#include <algorithm>
#include <cstdlib>

namespace Engine::WorldPartition {

namespace {

int ChebyshevDistance(const ChunkCoord& a, const ChunkCoord& b) {
    const int dx = std::abs(a.x - b.x);
    const int dz = std::abs(a.z - b.z);
    return std::max(dx, dz);
}

std::vector<ChunkCoord> BuildChunkSquare(const ChunkCoord& center, int radius, const WorldIndex& index) {
    std::vector<ChunkCoord> chunks;
    chunks.reserve((radius * 2 + 1) * (radius * 2 + 1));

    for (int z = center.z - radius; z <= center.z + radius; ++z) {
        for (int x = center.x - radius; x <= center.x + radius; ++x) {
            const ChunkCoord candidate{x, z};
            if (index.isValidChunk(candidate)) {
                chunks.push_back(candidate);
            }
        }
    }

    std::sort(chunks.begin(), chunks.end(), [](const ChunkCoord& a, const ChunkCoord& b) {
        if (a.z == b.z) {
            return a.x < b.x;
        }
        return a.z < b.z;
    });

    return chunks;
}

} // namespace

ChunkStreamer::ChunkStreamer(int preloadRadius, int retentionRadius)
    : m_preloadRadius(std::max(0, preloadRadius)),
      m_retentionRadius(std::max(preloadRadius, retentionRadius)) {
}

StreamUpdateResult ChunkStreamer::update(const ChunkCoord& centerChunk, const WorldIndex& index) {
    StreamUpdateResult result{};
    result.activeChunk = centerChunk;
    m_activeChunk = centerChunk;

    const std::vector<ChunkCoord> desired = BuildChunkSquare(centerChunk, m_preloadRadius, index);

    for (const ChunkCoord& coord : desired) {
        if (m_loadedChunks.find(coord) == m_loadedChunks.end()) {
            m_loadedChunks.insert(coord);
            result.toLoad.push_back(coord);
        }
    }

    std::vector<ChunkCoord> unloadCandidates;
    unloadCandidates.reserve(m_loadedChunks.size());
    for (const ChunkCoord& loaded : m_loadedChunks) {
        if (ChebyshevDistance(loaded, centerChunk) > m_retentionRadius) {
            unloadCandidates.push_back(loaded);
        }
    }

    for (const ChunkCoord& unload : unloadCandidates) {
        m_loadedChunks.erase(unload);
        result.toUnload.push_back(unload);
    }

    std::sort(result.toLoad.begin(), result.toLoad.end(), [](const ChunkCoord& a, const ChunkCoord& b) {
        if (a.z == b.z) {
            return a.x < b.x;
        }
        return a.z < b.z;
    });
    std::sort(result.toUnload.begin(), result.toUnload.end(), [](const ChunkCoord& a, const ChunkCoord& b) {
        if (a.z == b.z) {
            return a.x < b.x;
        }
        return a.z < b.z;
    });

    return result;
}

const std::unordered_set<ChunkCoord, ChunkCoordHash>& ChunkStreamer::loadedChunks() const {
    return m_loadedChunks;
}

ChunkCoord ChunkStreamer::activeChunk() const {
    return m_activeChunk;
}

} // namespace Engine::WorldPartition
