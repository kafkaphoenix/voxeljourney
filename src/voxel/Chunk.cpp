#include "Chunk.h"

#include <cassert>

namespace se::voxel {

inline int idx(int x, int y, int z) { return x + Chunk::SIZE * (y + Chunk::SIZE * z); }

bool Chunk::inBounds(int x, int y, int z) { return x >= 0 && x < SIZE && y >= 0 && y < SIZE && z >= 0 && z < SIZE; }

VoxelType Chunk::get(int x, int y, int z) const {
    assert(inBounds(x, y, z));
    return m_Voxels.at(idx(x, y, z));
}

void Chunk::set(int x, int y, int z, VoxelType type) {
    assert(inBounds(x, y, z));
    if (m_Voxels.at(idx(x, y, z)) == type) {
        return;  // no change, no need to remesh
    }
    m_Voxels.at(idx(x, y, z)) = type;
    dirty = true;
}

}  // namespace se::voxel