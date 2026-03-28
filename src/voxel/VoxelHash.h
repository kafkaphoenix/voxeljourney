#pragma once
#include <functional>
#include <glm/vec3.hpp>

namespace se::voxel {

// Custom hash function for glm::ivec3 to use as unordered_map keys.
// It combines the hashes of the individual components using a common
// technique to reduce collisions using bitwise operations and the golden ratio constant.
struct IVec3Hash {
    size_t operator()(const glm::ivec3& v) const noexcept {
        size_t h = 0;
        h ^= std::hash<int>{}(v.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

}  // namespace se::voxel