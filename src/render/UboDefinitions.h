#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <string_view>

#include "assets/Skeleton.h"

namespace se::render {

// Must stay in sync with `layout(std140, binding = N)` in GLSL shaders.
enum class UboBinding : uint8_t {
    Frame = 0,
    Bones = 1,
    Terrain = 2,
};

inline constexpr std::array<std::pair<std::string_view, UboBinding>, 3> UBO_BINDINGS = {{
    {"FrameData", UboBinding::Frame},
    {"BoneData", UboBinding::Bones},
    {"TerrainData", UboBinding::Terrain},
}};

struct alignas(16) PointLightGpuData {
    glm::vec4 positionRange;
    glm::vec4 colorIntensity;
};

struct alignas(16) FrameUbo {
    glm::mat4 viewProj;
    glm::vec4 sunDir;
    glm::vec4 sunColor;
    glm::vec4 ambient;
    glm::vec4 cameraPos;
    glm::ivec4 lightCounts;
    std::array<PointLightGpuData, 4> pointLights;
};

struct alignas(16) BoneUbo {
    std::array<glm::mat4, se::assets::MAX_BONES> bones;
};

struct alignas(16) TerrainUbo {
    glm::mat4 viewProj;
    glm::vec4 sunDir;
    glm::vec4 sunColor;
    glm::vec4 ambient;
};

}  // namespace se::render
