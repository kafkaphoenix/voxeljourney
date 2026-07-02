#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "voxel/ChunkCoords.h"

namespace se::render {
class Mesh;
}

namespace se::scene {
struct ChunkRenderable {
    std::unique_ptr<se::render::Mesh> mesh;
    glm::ivec3 position{0};

    [[nodiscard]] glm::mat4 getMatrix() const {
        return glm::translate(glm::mat4(1.0f), glm::vec3(se::voxel::chunkcoords::chunkToWorld(position)));
    }
};

}  // namespace se::scene