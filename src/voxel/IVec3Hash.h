#pragma once

#include <functional>
#include <glm/vec3.hpp>

// hash function for glm::ivec3 so we can use it as a key in unordered_map/set
namespace std {
template <>
struct hash<glm::ivec3> {
    size_t operator()(const glm::ivec3& v) const noexcept {
        size_t hx = hash<int>{}(v.x);
        size_t hy = hash<int>{}(v.y);
        size_t hz = hash<int>{}(v.z);

        return hx ^ (hy << 1) ^ (hz << 2);
    }
};

}