#pragma once

#include <glm/vec3.hpp>
#include <memory>

namespace se::render {
class Mesh;
}

namespace se::voxel {
class Chunk;
class ChunkMap;
}

namespace se::voxel::ChunkMesher {

std::unique_ptr<se::render::Mesh> buildMesh(const Chunk& chunk, const glm::ivec3& chunkCoord, const ChunkMap& chunkMap);

}  // namespace se::voxel::ChunkMesher