#include "ChunkMap.h"

#include <glm/gtx/string_cast.hpp>

#include <stdexcept>

#include "ChunkCoords.h"

namespace se::voxel {

Chunk& ChunkMap::createChunk(glm::ivec3 chunkCoord) {
    auto [it, inserted] = m_Chunks.try_emplace(chunkCoord);
    if (!inserted) {
        throw std::runtime_error("Chunk already exists at coordinate: " + glm::to_string(chunkCoord));
    }
    return it->second;
}

void ChunkMap::removeChunk(glm::ivec3 chunkCoord) { m_Chunks.erase(chunkCoord); }
bool ChunkMap::hasChunk(glm::ivec3 chunkCoord) const { return m_Chunks.contains(chunkCoord); }

Chunk* ChunkMap::getChunk(glm::ivec3 chunkCoord) {
    auto it = m_Chunks.find(chunkCoord);
    return it != m_Chunks.end() ? &it->second : nullptr;
}

const Chunk* ChunkMap::getChunk(glm::ivec3 chunkCoord) const {
    auto it = m_Chunks.find(chunkCoord);
    return it != m_Chunks.end() ? &it->second : nullptr;
}

VoxelType ChunkMap::getVoxelType(glm::ivec3 worldCoord) const {
    const Chunk* chunk = getChunk(chunkcoords::worldToChunk(worldCoord));
    if (!chunk) {
        return VoxelType::Air;  // return Air instead of throwing — mesher samples OOB neighbours
    }
    const glm::ivec3 voxelCoord = chunkcoords::worldToVoxel(worldCoord);
    return chunk->get(voxelCoord.x, voxelCoord.y, voxelCoord.z);
}

}  // namespace se::voxel