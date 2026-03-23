#pragma once
#include <glm/vec3.hpp>
#include <span>

#include "Light.h"

namespace se::scene {

struct LightData {
    const DirectionalLight* sun = nullptr;
    std::span<const PointLight> pointLights;
    std::span<const SpotLight> spotLights;
    glm::vec3 ambientColor{1.0f};
    float ambientStrength = 0.2f;
};

}  // namespace se::scene