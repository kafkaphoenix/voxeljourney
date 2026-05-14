#include "World.h"

#include "ChunkCoords.h"
#include "ChunkMesher.h"
#include "core/Timer.h"
#include "render/Mesh.h"
#include "scene/ChunkRenderable.h"
#include "scene/Scene.h"

namespace se::voxel {

World::World(const se::core::Config::World& config) : m_RenderDistance(config.renderDistance) {
    // to remove
    auto& chunk = m_ChunkMap.createChunk(glm::ivec3(0));
    // replace with actual terrain generation later.
    for (int x = 0; x < Chunk::SIZE; x++) {
        for (int z = 0; z < Chunk::SIZE; z++) { chunk.set(x, 0, z, VoxelType::Grass); }
    }
    m_DirtyChunks.insert(glm::ivec3(0));
}

void World::spawnChunk(const glm::ivec3& chunkCoord) {
    auto& chunk = m_ChunkMap.createChunk(chunkCoord);

    // terrain generation here

    if (chunk.hasGeometry()) {
        m_DirtyChunks.insert(chunkCoord);
    }
}

void World::setVoxel(const glm::ivec3& worldCoord, VoxelType type) {
    const glm::ivec3 chunkCoord = chunkcoords::worldToChunk(worldCoord);
    const glm::ivec3 voxelCoord = chunkcoords::worldToVoxel(worldCoord);

    auto* chunk = m_ChunkMap.getChunk(chunkCoord);
    if (!chunk) {
        return;  // can't set voxel in a chunk that doesn't exist
    }
    if (chunk->set(voxelCoord.x, voxelCoord.y, voxelCoord.z, type)) {
        m_DirtyChunks.insert(chunkCoord);
    }
}

void World::updateChunks(se::scene::Scene& scene, const glm::vec3& playerWorldPos) {
    const glm::ivec3 playerChunkCoord = chunkcoords::worldToChunk(playerWorldPos);

    if (playerChunkCoord != m_LastPlayerChunkCoord) {
        m_LastPlayerChunkCoord = playerChunkCoord;
        updateChunkStreaming(scene, playerChunkCoord);
    }

    rebuildDirtyChunks(scene);
}

void World::updateChunkStreaming(se::scene::Scene& scene, const glm::ivec3& playerChunkCoord) {
    std::unordered_set<glm::ivec3> wanted;
    size_t d = static_cast<size_t>(m_RenderDistance) * 2 + 1;
    wanted.reserve(d * d * d);

    // determine chunks that should exist
    for (int x = -m_RenderDistance; x <= m_RenderDistance; ++x) {
        for (int y = -m_RenderDistance; y <= m_RenderDistance; ++y) {
            for (int z = -m_RenderDistance; z <= m_RenderDistance; ++z) {
                const glm::ivec3 chunkCoord = playerChunkCoord + glm::ivec3(x, y, z);

                wanted.insert(chunkCoord);

                if (!m_ChunkMap.hasChunk(chunkCoord)) {
                    spawnChunk(chunkCoord);
                }
            }
        }
    }

    // unload chunks no longer wanted
    std::vector<glm::ivec3> toRemove;

    for (const auto& [chunkCoord, _] : m_ChunkMap.getChunks()) {
        if (!wanted.contains(chunkCoord)) {
            toRemove.push_back(chunkCoord);
        }
    }

    for (const auto& chunkCoord : toRemove) {
        scene.removeChunkRenderable(chunkCoord);
        m_ChunkMap.removeChunk(chunkCoord);
        m_DirtyChunks.erase(chunkCoord);
    }
}

void World::rebuildDirtyChunks(se::scene::Scene& scene) {
    if (m_DirtyChunks.empty()) {
        return;
    }

    constexpr float BUDGET_MS = 4.0f;
    se::core::Timer timer;

    auto it = m_DirtyChunks.begin();
    while (it != m_DirtyChunks.end()) {
        const glm::ivec3 chunkCoord = *it;

        auto* chunk = m_ChunkMap.getChunk(chunkCoord);
        if (!chunk || !chunk->hasGeometry()) {
            it = m_DirtyChunks.erase(it);
            continue;
        }

        auto mesh = chunkmesher::buildMesh(*chunk, chunkCoord, m_ChunkMap);
        if (mesh) {
            scene.updateChunkRenderable({.mesh = std::move(mesh), .position = chunkCoord});
        }
        it = m_DirtyChunks.erase(it);

        if (timer.millis() >= BUDGET_MS) {
            break;
        }
    }
}

}  // namespace se::voxel