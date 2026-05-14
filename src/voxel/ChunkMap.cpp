#include "ChunkMap.h"

#include <stdexcept>

#include "ChunkCoords.h"

namespace se::voxel {

VoxelType ChunkMap::getVoxelType(glm::ivec3 worldCoord) const {
    const Chunk* chunk = getChunk(chunkcoords::worldToChunk(worldCoord));
    if (!chunk) {
        return VoxelType::Air;  // return Air instead of throwing — mesher samples OOB neighbours
    }
    const glm::ivec3 voxelCoord = chunkcoords::worldToVoxel(worldCoord);
    return chunk->get(voxelCoord.x, voxelCoord.y, voxelCoord.z);
}

Chunk& ChunkMap::createChunk(glm::ivec3 chunkCoord) {
    if (m_Chunks.contains(chunkCoord)) {
        throw std::runtime_error("Chunk already exists");
    }
    auto& ptr = m_Chunks[chunkCoord] = std::make_unique<Chunk>();
    return *ptr;
}

void ChunkMap::removeChunk(glm::ivec3 chunkCoord) { m_Chunks.erase(chunkCoord); }
bool ChunkMap::hasChunk(glm::ivec3 chunkCoord) const { return m_Chunks.contains(chunkCoord); }

Chunk* ChunkMap::getChunk(glm::ivec3 chunkCoord) {
    auto it = m_Chunks.find(chunkCoord);
    return it != m_Chunks.end() ? it->second.get() : nullptr;
}

const Chunk* ChunkMap::getChunk(glm::ivec3 chunkCoord) const {
    auto it = m_Chunks.find(chunkCoord);
    return it != m_Chunks.end() ? it->second.get() : nullptr;
}

}  // namespace se::voxel