#pragma once

#include <array>

#include "Voxel.h"

namespace se::voxel {

class Chunk {
public:
    static constexpr int SIZE = 16;
    static constexpr int VOLUME = SIZE * SIZE * SIZE;

    [[nodiscard]] VoxelType get(int x, int y, int z) const;
    bool set(int x, int y, int z, VoxelType type);
    static bool inBounds(int x, int y, int z);

    [[nodiscard]] bool hasGeometry() const { return m_HasGeometry; }

private:
    // flat array: index = x + SIZE * (y + SIZE * z)
    std::array<VoxelType, VOLUME> m_Voxels{};
    int m_SolidCount{0};
    bool m_HasGeometry{false};
};

}  // namespace se::voxel