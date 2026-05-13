#include "Chunk.h"

#include <cassert>

namespace se::voxel {

inline int idx(int x, int y, int z) { return x + Chunk::SIZE * (y + Chunk::SIZE * z); }

bool Chunk::inBounds(int x, int y, int z) { return x >= 0 && x < SIZE && y >= 0 && y < SIZE && z >= 0 && z < SIZE; }

VoxelType Chunk::get(int x, int y, int z) const {
    assert(inBounds(x, y, z));
    return m_Voxels.at(idx(x, y, z));
}

bool Chunk::set(int x, int y, int z, VoxelType type) {
    assert(inBounds(x, y, z));

    auto& voxel = m_Voxels.at(idx(x, y, z));
    const VoxelType oldType = voxel;

    if (oldType == type) {
        return false;
    }

    voxel = type;

    if (oldType == VoxelType::Air && type != VoxelType::Air) {
        ++m_SolidCount;
    } else if (oldType != VoxelType::Air && type == VoxelType::Air) {
        --m_SolidCount;
    }
    m_SolidCount = std::max(0, m_SolidCount);  // just in case

    m_HasGeometry = (m_SolidCount > 0);
    return true;
}

}  // namespace se::voxel