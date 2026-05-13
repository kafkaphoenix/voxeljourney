#pragma once

#include <glm/vec3.hpp>
#include <memory>
#include <unordered_map>

#include "Chunk.h"
#include "IVec3Hash.h"

namespace se::voxel {

class ChunkMap {
public:
    Chunk& createChunk(glm::ivec3 chunkCoord);
    void removeChunk(glm::ivec3 chunkCoord);
    bool hasChunk(glm::ivec3 chunkCoord) const;
    Chunk* getChunk(glm::ivec3 chunkCoord);
    const Chunk* getChunk(glm::ivec3 chunkCoord) const;
    const auto& getChunks() const { return m_Chunks; }
    VoxelType getVoxelType(glm::ivec3 worldCoord) const;

private:
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> m_Chunks;
};

}  // namespace se::voxel