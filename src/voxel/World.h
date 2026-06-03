#pragma once

#include <glm/vec3.hpp>
#include <memory>
#include <unordered_set>

#include "ChunkMap.h"
#include "IVec3Hash.h"
#include "core/Config.h"

namespace se::scene {
class Scene;
}

namespace se::voxel {

class World {
public:
    explicit World(const se::core::config::World& config);

    void setVoxel(const glm::ivec3& worldCoord, VoxelType type);
    void updateChunks(se::scene::Scene& scene, const glm::vec3& playerWorldPos);
    const ChunkMap& getChunkMap() const { return m_ChunkMap; }

private:
    glm::ivec3 m_LastPlayerChunkCoord{INT_MIN};
    int m_RenderDistance;
    ChunkMap m_ChunkMap;
    std::unordered_set<glm::ivec3> m_DirtyChunks;

    void spawnChunk(const glm::ivec3& chunkCoord);
    void updateChunkStreaming(se::scene::Scene& scene, const glm::ivec3& playerChunkCoord);
    void rebuildDirtyChunks(se::scene::Scene& scene);
};

}  // namespace se::voxel