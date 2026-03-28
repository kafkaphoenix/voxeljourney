#pragma once
#include <glm/vec3.hpp>
#include <memory>
#include <unordered_map>

#include "Chunk.h"
#include "VoxelHash.h"

namespace se::voxel {

class ChunkManager {
public:
    Chunk& createChunk(glm::ivec3 chunkPos);
    void removeChunk(glm::ivec3 chunkPos);
    bool hasChunk(glm::ivec3 chunkPos) const;
    Chunk* getChunk(glm::ivec3 chunkPos);
    const Chunk* getChunk(glm::ivec3 chunkPos) const;
    const auto& getChunks() const { return m_Chunks; }

    VoxelType getVoxel(glm::ivec3 worldPos) const;
    void setVoxel(glm::ivec3 worldPos, VoxelType type);

    static glm::ivec3 worldToChunk(glm::ivec3 worldPos);
    static glm::ivec3 worldToLocal(glm::ivec3 worldPos);

private:
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, IVec3Hash> m_Chunks;
};

}  // namespace se::voxel