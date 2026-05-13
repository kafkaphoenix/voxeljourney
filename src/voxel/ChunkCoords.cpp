#include "ChunkCoords.h"

#include "Chunk.h"

namespace se::voxel::ChunkCoords {
glm::ivec3 chunkToWorld(glm::ivec3 chunkCoord) { return chunkCoord * Chunk::SIZE; }

glm::ivec3 worldToChunk(glm::ivec3 worldCoord) {
    auto fd = [](int v) { return v >= 0 ? v / Chunk::SIZE : (v - Chunk::SIZE + 1) / Chunk::SIZE; };
    return {fd(worldCoord.x), fd(worldCoord.y), fd(worldCoord.z)};
}

glm::ivec3 worldToChunk(glm::vec3 worldPos) { return worldToChunk(glm::ivec3(glm::floor(worldPos))); }

glm::ivec3 worldToVoxel(glm::ivec3 worldCoord) { return worldCoord - worldToChunk(worldCoord) * Chunk::SIZE; }
}  // namespace se::voxel::ChunkCoords
