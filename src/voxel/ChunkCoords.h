#pragma once

#include <glm/glm.hpp>

namespace se::voxel::ChunkCoords {
glm::ivec3 chunkToWorld(glm::ivec3 chunkCoord);
glm::ivec3 worldToChunk(glm::ivec3 worldCoord);
glm::ivec3 worldToChunk(glm::vec3 worldPos);
glm::ivec3 worldToVoxel(glm::ivec3 worldCoord);
}