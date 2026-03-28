#include "ChunkManager.h"

#include <cmath>
#include <exception>
#include <stdexcept>

#include "render/Mesh.h"

namespace se::voxel {

glm::ivec3 ChunkManager::worldToChunk(glm::ivec3 w) {
    auto fd = [](int v) { return v >= 0 ? v / Chunk::SIZE : (v - Chunk::SIZE + 1) / Chunk::SIZE; };
    return {fd(w.x), fd(w.y), fd(w.z)};
}

glm::ivec3 ChunkManager::worldToLocal(glm::ivec3 w) {
    glm::ivec3 c = worldToChunk(w);
    return w - c * Chunk::SIZE;
}

VoxelType ChunkManager::getVoxel(glm::ivec3 worldPos) const {
    const Chunk* chunk = getChunk(worldToChunk(worldPos));
    if (!chunk) {
        throw std::runtime_error("No chunk at " + std::to_string(worldPos.x) + "," + std::to_string(worldPos.y) + "," +
                                 std::to_string(worldPos.z));
    }
    auto local = worldToLocal(worldPos);
    return chunk->get(local.x, local.y, local.z);
}

void ChunkManager::setVoxel(glm::ivec3 worldPos, VoxelType type) {
    auto* chunk = getChunk(worldToChunk(worldPos));
    if (!chunk) {
        throw std::runtime_error("No chunk at " + std::to_string(worldPos.x) + "," + std::to_string(worldPos.y) + "," +
                                 std::to_string(worldPos.z));
    }
    auto local = worldToLocal(worldPos);
    chunk->set(local.x, local.y, local.z, type);
    // TODO remesh neighboring chunks if we're on a border voxel
}

Chunk& ChunkManager::createChunk(glm::ivec3 chunkPos) {
    auto it = m_Chunks.find(chunkPos);
    if (it != m_Chunks.end()) {
        throw std::runtime_error("Chunk already exists at " + std::to_string(chunkPos.x) + "," +
                                 std::to_string(chunkPos.y) + "," + std::to_string(chunkPos.z));
    }
    auto chunk = std::make_unique<Chunk>();
    chunk->position = chunkPos;
    auto& ref = *chunk;
    m_Chunks.emplace(chunkPos, std::move(chunk));
    return ref;
}

void ChunkManager::removeChunk(glm::ivec3 chunkPos) { m_Chunks.erase(chunkPos); }

bool ChunkManager::hasChunk(glm::ivec3 chunkPos) const { return m_Chunks.find(chunkPos) != m_Chunks.end(); }

Chunk* ChunkManager::getChunk(glm::ivec3 chunkPos) {
    auto it = m_Chunks.find(chunkPos);
    return it != m_Chunks.end() ? it->second.get() : nullptr;
}

const Chunk* ChunkManager::getChunk(glm::ivec3 chunkPos) const {
    auto it = m_Chunks.find(chunkPos);
    return it != m_Chunks.end() ? it->second.get() : nullptr;
}

}  // namespace se::voxel