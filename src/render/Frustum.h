#pragma once
#include <glm/glm.hpp>

#include "Mesh.h"

namespace se::render {

struct Frustum {
    glm::vec4 planes[6];  // x,y,z,w: plane normal.xyz, d
};

inline Frustum extractFrustum(const glm::mat4& vp) {
    Frustum frustum;
    // Left
    frustum.planes[0] = glm::vec4(
        vp[0][3] + vp[0][0],
        vp[1][3] + vp[1][0],
        vp[2][3] + vp[2][0],
        vp[3][3] + vp[3][0]);
    // Right
    frustum.planes[1] = glm::vec4(
        vp[0][3] - vp[0][0],
        vp[1][3] - vp[1][0],
        vp[2][3] - vp[2][0],
        vp[3][3] - vp[3][0]);
    // Bottom
    frustum.planes[2] = glm::vec4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]);
    // Top
    frustum.planes[3] = glm::vec4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]);
    // Near
    frustum.planes[4] = glm::vec4(
        vp[0][3] + vp[0][2],
        vp[1][3] + vp[1][2],
        vp[2][3] + vp[2][2],
        vp[3][3] + vp[3][2]);
    // Far
    frustum.planes[5] = glm::vec4(
        vp[0][3] - vp[0][2],
        vp[1][3] - vp[1][2],
        vp[2][3] - vp[2][2],
        vp[3][3] - vp[3][2]);
    // Normalize
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(glm::vec3(frustum.planes[i]));
        if (len > 0.0f) frustum.planes[i] /= len;
    }
    return frustum;
}

inline bool frustumIntersectsAABB(const Frustum& frustum, const AABB& aabb, const glm::mat4& modelMatrix) {
    // Transform AABB min/max to world space
    glm::vec3 wsMin = glm::vec3(modelMatrix * glm::vec4(aabb.min, 1.0f));
    glm::vec3 wsMax = glm::vec3(modelMatrix * glm::vec4(aabb.max, 1.0f));

    // For each plane, test the most positive vertex
    for (int p = 0; p < 6; ++p) {
        const glm::vec3 n = glm::vec3(frustum.planes[p]);
        float d = frustum.planes[p].w;
        // Most positive vertex: pick max or min for each axis based on plane normal sign
        glm::vec3 v;
        v.x = n.x >= 0.0f ? wsMax.x : wsMin.x;
        v.y = n.y >= 0.0f ? wsMax.y : wsMin.y;
        v.z = n.z >= 0.0f ? wsMax.z : wsMin.z;
        if (glm::dot(n, v) + d < 0.0f) return false;
    }
    return true;
}

}  // namespace se::render
