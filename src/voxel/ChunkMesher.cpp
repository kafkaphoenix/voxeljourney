#include "ChunkMesher.h"

#include <array>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <ranges>
#include <vector>

#include "Chunk.h"
#include "ChunkCoords.h"
#include "ChunkMap.h"
#include "Voxel.h"
#include "render/Mesh.h"

namespace se::voxel::chunkmesher {

namespace {
#pragma pack(push, 1)  // to ensure no padding is added to ChunkVertex, since we use sizeof(ChunkVertex) for buffer
                       // strides and offsets
struct ChunkVertex {
    uint8_t posX;  // 1 byte
    uint8_t posY;  // 1 byte
    uint8_t posZ;  // 1 byte
    glm::vec2 uv;  // 8 bytes
};
#pragma pack(pop)

static_assert(sizeof(ChunkVertex) == 11);
static_assert(offsetof(ChunkVertex, uv) == 3);

struct VoxelFace {
    glm::ivec3 normal;
    std::array<glm::ivec3, 4> corners;
};

// counter-clockwise winding
const std::array<VoxelFace, 6> FACES = {{
    // +X
    {{1, 0, 0}, {glm::ivec3(1, 0, 0), glm::ivec3(1, 0, 1), glm::ivec3(1, 1, 1), glm::ivec3(1, 1, 0)}},
    // -X
    {{-1, 0, 0}, {glm::ivec3(0, 0, 1), glm::ivec3(0, 0, 0), glm::ivec3(0, 1, 0), glm::ivec3(0, 1, 1)}},
    // +Y
    {{0, 1, 0}, {glm::ivec3(0, 1, 1), glm::ivec3(0, 1, 0), glm::ivec3(1, 1, 0), glm::ivec3(1, 1, 1)}},
    // -Y
    {{0, -1, 0}, {glm::ivec3(0, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(1, 0, 1), glm::ivec3(1, 0, 0)}},
    // +Z
    {{0, 0, 1}, {glm::ivec3(1, 0, 1), glm::ivec3(0, 0, 1), glm::ivec3(0, 1, 1), glm::ivec3(1, 1, 1)}},
    // -Z
    {{0, 0, -1}, {glm::ivec3(0, 0, 0), glm::ivec3(1, 0, 0), glm::ivec3(1, 1, 0), glm::ivec3(0, 1, 0)}},
}};

// This will be replaced with proper block textures later, but for now we
// can just use the same UVs for every face since we're not sampling a texture atlas yet.
constexpr std::array<glm::vec2, 4> FACE_UVS = {
    glm::vec2{0.0f, 0.0f},
    glm::vec2{1.0f, 0.0f},
    glm::vec2{1.0f, 1.0f},
    glm::vec2{0.0f, 1.0f},
};

}  // namespace

// worst case: each voxel has 6 faces, each face has 4 vertices and 6 indices
// in practice culling removes most faces but this is the upper bound
static constexpr int FACE_VERTEX_COUNT = 4;
static constexpr int FACE_INDEX_COUNT = 6;
static constexpr int MAX_FACES = Chunk::VOLUME * FACES.size();
static constexpr int MAX_VERTICES = MAX_FACES * FACE_VERTEX_COUNT;
static constexpr int MAX_INDICES = MAX_FACES * FACE_INDEX_COUNT;

std::unique_ptr<se::render::Mesh> buildMesh(const Chunk& chunk, const glm::ivec3& chunkCoord,
                                            const ChunkMap& chunkMap) {
    std::vector<ChunkVertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(MAX_VERTICES);
    indices.reserve(MAX_INDICES);

    const glm::ivec3 chunkWorldOrigin = chunkcoords::chunkToWorld(chunkCoord);

    for (int z = 0; z < Chunk::SIZE; z++) {
        for (int y = 0; y < Chunk::SIZE; y++) {
            for (int x = 0; x < Chunk::SIZE; x++) {
                VoxelType type = chunk.get(x, y, z);
                if (!isSolid(type)) {
                    continue;
                }

                const glm::ivec3 voxelWorldCoord = chunkWorldOrigin + glm::ivec3(x, y, z);

                // check each face of the voxel. If the adjacent voxel in that direction is solid, skip it (don't
                // generate a face there)
                for (const VoxelFace& vf : FACES) {
                    if (isSolid(chunkMap.getVoxelType(voxelWorldCoord + vf.normal))) {
                        continue;
                    }

                    const auto base = static_cast<unsigned int>(vertices.size());
                    for (auto [corner, uv] : std::views::zip(vf.corners, FACE_UVS)) {
                        vertices.push_back({
                            .posX = static_cast<uint8_t>(x + corner.x),
                            .posY = static_cast<uint8_t>(y + corner.y),
                            .posZ = static_cast<uint8_t>(z + corner.z),
                            .uv = uv,
                        });
                    }
                    // first triangle
                    indices.push_back(base + 0);
                    indices.push_back(base + 2);
                    indices.push_back(base + 1);
                    // second triangle
                    indices.push_back(base + 0);
                    indices.push_back(base + 3);
                    indices.push_back(base + 2);
                }
            }
        }
    }

    if (vertices.empty()) {
        return nullptr;
    }

    se::render::AABB aabb{};
    aabb.min = glm::vec3(0.0f);
    aabb.max = glm::vec3(Chunk::SIZE);

    se::render::BufferLayout layout({
        {"a_Position", GL_UNSIGNED_BYTE, sizeof(uint8_t), 0, 3, GL_FALSE},
        {"a_Uv", GL_FLOAT, sizeof(float), 0, 2, GL_FALSE},
    });

    return std::make_unique<se::render::Mesh>(vertices, indices, aabb, layout);
}

}  // namespace se::voxel