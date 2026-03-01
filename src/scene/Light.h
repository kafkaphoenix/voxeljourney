#pragma once

#include <glm/vec3.hpp>

namespace se::scene {

enum class LightType {
    Directional,
    Point
};

struct Light {
    LightType type = LightType::Directional;
    glm::vec3 color{1.0f, 0.95f, 0.9f};
    float intensity = 1.0f;

    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 position{0.0f, 5.0f, 0.0f};
    float range = 25.0f;
};

}  // namespace se::scene
