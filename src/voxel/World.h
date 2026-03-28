#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "ChunkManager.h"
#include "core/Config.h"

namespace se::scene {
class Scene;
}
namespace se::render {
class Mesh;
}

namespace se::voxel {

class World {
public:
    World(const se::core::Config::World& config);

    void updateChunks(se::scene::Scene& scene, const glm::vec3& playerPos);

private:
    int m_RenderDistance;
    ChunkManager m_ChunkManager;

    void unloadFarChunks(se::scene::Scene& scene, const glm::ivec3& playerChunk);
    void loadChunks(const glm::ivec3& playerChunk);
    void rebuildDirtyChunks(se::scene::Scene& scene);
};

}  // namespace se::voxel