#include "World.h"

#include "ChunkMesher.h"
#include "render/Mesh.h"
#include "scene/ChunkRenderable.h"
#include "scene/Scene.h"

namespace se::voxel {

World::World(const se::core::Config::World& config) : m_RenderDistance(config.renderDistance) {
    m_ChunkManager.createChunk({0, 0, 0});
    for (int x = 0; x < se::voxel::Chunk::SIZE; x++) {
        for (int z = 0; z < se::voxel::Chunk::SIZE; z++) {
            m_ChunkManager.setVoxel({x, 0, z}, se::voxel::VoxelType::Grass);
        }
    }
}

void World::updateChunks(se::scene::Scene& scene, const glm::vec3& playerPos) {
    const glm::ivec3 playerChunk = glm::ivec3(glm::floor(playerPos)) / Chunk::SIZE;
    unloadFarChunks(scene, playerChunk);
    loadChunks(playerChunk);
    rebuildDirtyChunks(scene);
}

void World::unloadFarChunks(se::scene::Scene& scene, const glm::ivec3& playerChunk) {
    std::vector<glm::ivec3> toRemove;
    for (const auto& [pos, chunk] : m_ChunkManager.getChunks()) {
        const glm::ivec3 delta = pos - playerChunk;
        if (std::abs(delta.x) > m_RenderDistance || std::abs(delta.y) > m_RenderDistance ||
            std::abs(delta.z) > m_RenderDistance) {
            toRemove.push_back(pos);
        }
    }

    for (const auto& pos : toRemove) {
        scene.removeChunkRenderable(pos);
        m_ChunkManager.removeChunk(pos);
    }
}

void World::loadChunks(const glm::ivec3& playerChunk) {
    for (int x = -m_RenderDistance; x <= m_RenderDistance; ++x) {
        for (int y = -m_RenderDistance; y <= m_RenderDistance; ++y) {
            for (int z = -m_RenderDistance; z <= m_RenderDistance; ++z) {
                const glm::ivec3 chunkPos = playerChunk + glm::ivec3(x, y, z);
                if (!m_ChunkManager.hasChunk(chunkPos)) {
                    m_ChunkManager.createChunk(chunkPos);
                }
            }
        }
    }
}

void World::rebuildDirtyChunks(se::scene::Scene& scene) {
    for (const auto& [pos, chunk] : m_ChunkManager.getChunks()) {
        if (!chunk->dirty) {
            continue;
        }
        chunk->mesh = ChunkMesher::buildMesh(*chunk, m_ChunkManager);
        chunk->dirty = false;
        if (!chunk->mesh) {
            continue;
        }
        scene.updateChunkRenderable({.mesh = chunk->mesh.get(), .position = pos});
    }
}

}  // namespace se::voxel