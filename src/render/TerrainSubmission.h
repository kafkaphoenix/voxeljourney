#pragma once

#include <glm/mat4x4.hpp>

namespace se::render {

class Mesh;
}

namespace se::render {
struct TerrainSubmission {
    const Mesh* mesh = nullptr;
    glm::mat4 modelMatrix{1.0f};
};

}  // namespace se::render