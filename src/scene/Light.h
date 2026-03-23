#pragma once
#include <glm/vec3.hpp>

namespace se::scene {

struct DirectionalLight {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f, 0.95f, 0.9f};
    float intensity = 1.0f;
};

struct PointLight {
    glm::vec3 position{0.0f, 5.0f, 0.0f};
    glm::vec3 color{1.0f, 0.9f, 0.7f};
    float intensity = 1.0f;
    float range = 25.0f;
};

struct SpotLight {
    glm::vec3 position{};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 25.0f;
    float innerAngle = 15.0f;
    float outerAngle = 30.0f;
};

}  // namespace se::scene