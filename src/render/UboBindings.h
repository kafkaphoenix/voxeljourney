#pragma once

#include <array>
#include <cstdint>
#include <string_view>

// Central registry of UBO binding points.
// Must stay in sync with `layout(std140, binding = N)` in GLSL shaders.
namespace se::render {

enum class UboBinding : uint8_t {
    Frame = 0,
    Bones = 1,
    Terrain = 2,
};

inline constexpr std::array<std::pair<std::string_view, UboBinding>, 3> k_UboBindings = {{
    {"FrameData", UboBinding::Frame},
    {"BoneData", UboBinding::Bones},
    {"TerrainFrame", UboBinding::Terrain},
}};

}  // namespace se::render
