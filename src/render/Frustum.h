#pragma once
#include <glm/glm.hpp>

#include "Mesh.h"

namespace se::render {

struct Frustum {
    glm::vec4 planes[6];  // x,y,z,w: plane normal.xyz, d
};

inline Frustum calculateFrustum(const glm::mat4& viewProj) {
    Frustum frustum;
    // Left
    frustum.planes[0] = glm::vec4(
        viewProj[0][3] + viewProj[0][0],
        viewProj[1][3] + viewProj[1][0],
        viewProj[2][3] + viewProj[2][0],
        viewProj[3][3] + viewProj[3][0]);
    // Right
    frustum.planes[1] = glm::vec4(
        viewProj[0][3] - viewProj[0][0],
        viewProj[1][3] - viewProj[1][0],
        viewProj[2][3] - viewProj[2][0],
        viewProj[3][3] - viewProj[3][0]);
    // Bottom
    frustum.planes[2] = glm::vec4(
        viewProj[0][3] + viewProj[0][1],
        viewProj[1][3] + viewProj[1][1],
        viewProj[2][3] + viewProj[2][1],
        viewProj[3][3] + viewProj[3][1]);
    // Top
    frustum.planes[3] = glm::vec4(
        viewProj[0][3] - viewProj[0][1],
        viewProj[1][3] - viewProj[1][1],
        viewProj[2][3] - viewProj[2][1],
        viewProj[3][3] - viewProj[3][1]);
    // Near
    frustum.planes[4] = glm::vec4(
        viewProj[0][3] + viewProj[0][2],
        viewProj[1][3] + viewProj[1][2],
        viewProj[2][3] + viewProj[2][2],
        viewProj[3][3] + viewProj[3][2]);
    // Far
    frustum.planes[5] = glm::vec4(
        viewProj[0][3] - viewProj[0][2],
        viewProj[1][3] - viewProj[1][2],
        viewProj[2][3] - viewProj[2][2],
        viewProj[3][3] - viewProj[3][2]);
    // Normalize
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(glm::vec3(frustum.planes[i]));
        if (len > 0.0f) frustum.planes[i] /= len;
    }
    return frustum;
}

// For renderables without a model matrix, we can test the AABB directly in view-projection space.
inline bool frustumIntersectsAABB(const Frustum& frustum, const AABB& aabb) {
    for (int p = 0; p < 6; ++p) {
        const glm::vec3 n = glm::vec3(frustum.planes[p]);
        float d = frustum.planes[p].w;
        glm::vec3 v;
        v.x = n.x >= 0.0f ? aabb.max.x : aabb.min.x;
        v.y = n.y >= 0.0f ? aabb.max.y : aabb.min.y;
        v.z = n.z >= 0.0f ? aabb.max.z : aabb.min.z;
        if (glm::dot(n, v) + d < 0.0f) return false;
    }
    return true;
}

// For general renderables with a model matrix, we need to transform the AABB and test in world space.
inline bool frustumIntersectsAABB(const Frustum& frustum, const AABB& aabb, const glm::mat4& modelMatrix) {
    // recompute world-space AABB from 8 corners
    glm::vec3 wsMin(FLT_MAX), wsMax(-FLT_MAX);
    for (int i = 0; i < 8; ++i) {
        glm::vec3 corner = glm::vec3(
            modelMatrix * glm::vec4(
                              i & 1 ? aabb.max.x : aabb.min.x,
                              i & 2 ? aabb.max.y : aabb.min.y,
                              i & 4 ? aabb.max.z : aabb.min.z,
                              1.0f));
        wsMin = glm::min(wsMin, corner);
        wsMax = glm::max(wsMax, corner);
    }
    return frustumIntersectsAABB(frustum, AABB{wsMin, wsMax});
}

}  // namespace se::render