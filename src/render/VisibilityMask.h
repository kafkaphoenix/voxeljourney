#pragma once

#include <cstdint>

namespace se::render::visibility {

enum class Layer : uint8_t {
    World = 1u << 0,
    PlayerBody = 1u << 1,
};

constexpr Layer operator|(Layer a, Layer b) {
    return static_cast<Layer>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr uint8_t operator&(Layer a, Layer b) { return static_cast<uint8_t>(a) & static_cast<uint8_t>(b); }

constexpr bool isVisible(Layer maskOnMesh, Layer cameraMask) { return (maskOnMesh & cameraMask) != 0; }

inline constexpr Layer FIRST_PERSON_CAMERA = Layer::World;
inline constexpr Layer THIRD_PERSON_CAMERA = Layer::World | Layer::PlayerBody;

}  // namespace se::render::visibility