#pragma once

#include <glm/mat4x4.hpp>
#include <span>

#include "Renderable.h"

namespace se::scene {

struct AnimatedRenderable {
    Renderable renderable;
    std::span<const glm::mat4> boneMatrices;
};
}  // namespace se::scene
