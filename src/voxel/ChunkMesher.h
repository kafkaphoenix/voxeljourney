#pragma once

#include <memory>

namespace se::render {
class Mesh;
}

namespace se::voxel {

class Chunk;
class ChunkManager;

class ChunkMesher {
public:
    static std::unique_ptr<se::render::Mesh> buildMesh(const Chunk& chunk, const ChunkManager& chunkManager);
};

}  // namespace se::voxel