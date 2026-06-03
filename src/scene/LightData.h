#pragma once
#include <glm/vec3.hpp>
#include <span>

#include "Light.h"

namespace se::scene {

struct LightData {
    std::span<const DirectionalLight> directionalLights;
    std::span<const PointLight> pointLights;
    std::span<const SpotLight> spotLights;
    glm::vec3 ambientColor{1.0f};
    float ambientIntensity = 0.2f;
};

}  // namespace se::scene