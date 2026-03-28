#include "ChunkMesher.h"

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <ranges>
#include <vector>

#include "Chunk.h"
#include "ChunkManager.h"
#include "Voxel.h"
#include "render/Mesh.h"

namespace se::voxel {

namespace {
#pragma pack(push, 1)  // to ensure no padding is added to ChunkVertex, since we use sizeof(ChunkVertex) for buffer
                       // strides and offsets
struct ChunkVertex {
    glm::vec3 position;  // 12 bytes
    glm::vec2 uv;        // 8 bytes
};
#pragma pack(pop)

static_assert(sizeof(ChunkVertex) == 20);
static_assert(offsetof(ChunkVertex, uv) == 12);

struct FaceDef {
    glm::ivec3 normal;
    std::array<glm::vec3, 4> corners;
};

// counter-clockwise winding
const std::array<FaceDef, 6> FACES = {{
    // +X
    {{1, 0, 0}, {glm::vec3(1, 0, 0), glm::vec3(1, 0, 1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 0)}},
    // -X
    {{-1, 0, 0}, {glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 1, 1)}},
    // +Y
    {{0, 1, 0}, {glm::vec3(0, 1, 1), glm::vec3(0, 1, 0), glm::vec3(1, 1, 0), glm::vec3(1, 1, 1)}},
    // -Y
    {{0, -1, 0}, {glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(1, 0, 1), glm::vec3(1, 0, 0)}},
    // +Z
    {{0, 0, 1}, {glm::vec3(1, 0, 1), glm::vec3(0, 0, 1), glm::vec3(0, 1, 1), glm::vec3(1, 1, 1)}},
    // -Z
    {{0, 0, -1}, {glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 1, 0), glm::vec3(0, 1, 0)}},
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

std::unique_ptr<se::render::Mesh> ChunkMesher::buildMesh(const Chunk& chunk, const ChunkManager& chunkManager) {
    std::vector<ChunkVertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(MAX_VERTICES);
    indices.reserve(MAX_INDICES);

    const glm::vec3 origin = chunk.worldOrigin();

    for (int z = 0; z < Chunk::SIZE; z++) {
        for (int y = 0; y < Chunk::SIZE; y++) {
            for (int x = 0; x < Chunk::SIZE; x++) {
                VoxelType type = chunk.get(x, y, z);
                if (!isSolid(type)) {
                    continue;
                }

                glm::ivec3 worldPos = glm::ivec3(origin) + glm::ivec3(x, y, z);

                for (const FaceDef& fd : FACES) {
                    if (isSolid(chunkManager.getVoxel(worldPos + fd.normal))) {
                        continue;
                    }

                    const auto base = static_cast<unsigned int>(vertices.size());
                    for (auto [corner, uv] : std::views::zip(fd.corners, FACE_UVS)) {
                        vertices.push_back(ChunkVertex{
                            .position = origin + glm::vec3(x, y, z) + corner,
                            .uv = uv,
                        });
                    }
                    // first triangle: 0, 2, 1
                    // second triangle: 0, 3, 2
                    indices.insert(indices.end(), {base + 0, base + 2, base + 1, base + 0, base + 3, base + 2});
                }
            }
        }
    }

    if (vertices.empty()) {
        return nullptr;
    }

    se::render::AABB aabb{};
    aabb.min = origin;
    aabb.max = origin + glm::vec3(Chunk::SIZE);

    se::render::BufferLayout layout({
        {"a_Position", GL_FLOAT, sizeof(float), 0, 3, GL_FALSE},
        {"a_Uv", GL_FLOAT, sizeof(float), 0, 2, GL_FALSE},
    });

    return std::make_unique<se::render::Mesh>(vertices, indices, aabb, layout);
}

}  // namespace se::voxel