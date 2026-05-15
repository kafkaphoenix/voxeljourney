#pragma once

#include <array>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace se::assets {

inline constexpr int MAX_BONES = 128;

struct BonePose {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

using Pose = std::array<BonePose, MAX_BONES>;
using BonePalette = std::array<glm::mat4, MAX_BONES>;

}  // namespace se::assets
