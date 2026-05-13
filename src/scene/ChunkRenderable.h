#pragma once

#include <glm/vec3.hpp>
#include <memory>

namespace se::render {
class Mesh;
}
namespace se::scene {
struct ChunkRenderable {
    std::unique_ptr<se::render::Mesh> mesh;
    glm::ivec3 position{0};
};

}  // namespace se::scene