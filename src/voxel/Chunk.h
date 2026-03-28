#pragma once
#include <array>
#include <glm/vec3.hpp>
#include <memory>

#include "Voxel.h"

namespace se::render {
class Mesh;
}

namespace se::voxel {

class Chunk {
public:
    static constexpr int SIZE = 16;
    static constexpr int VOLUME = SIZE * SIZE * SIZE;

    [[nodiscard]] VoxelType get(int x, int y, int z) const;
    void set(int x, int y, int z, VoxelType type);
    static bool inBounds(int x, int y, int z);

    [[nodiscard]] glm::vec3 worldOrigin() const { return glm::vec3(position) * static_cast<float>(SIZE); }

    // chunk position in chunk-space (not world-space)
    glm::ivec3 position{0};
    bool dirty = true;
    std::unique_ptr<se::render::Mesh> mesh;

private:
    // flat array: index = x + SIZE * (y + SIZE * z)
    std::array<VoxelType, VOLUME> m_Voxels{};
};

}  // namespace se::voxel