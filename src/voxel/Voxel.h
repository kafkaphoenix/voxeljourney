#pragma once
#include <cstdint>

namespace se::voxel {

enum class VoxelType : uint8_t {
    Air = 0,
    Stone = 1,
    Dirt = 2,
    Grass = 3,
};

constexpr bool isSolid(VoxelType t) { return t != VoxelType::Air; }

}  // namespace se::voxel