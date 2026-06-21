#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <ranges>
#include <span>

#include "VisibilityMask.h"

namespace se::render {

inline constexpr std::size_t MAX_FRAME_DIRECTIONAL_LIGHTS = 1;
inline constexpr std::size_t MAX_FRAME_POINT_LIGHTS = 4;

struct FrameCameraData {
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 worldPosition{0.0f};
    visibility::Layer visibilityMask = visibility::Layer::World;
};

struct DirectionalLightRenderData {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float intensity = 0.0f;
    glm::vec3 color{1.0f};
    float padding0 = 0.0f;
};

struct PointLightRenderData {
    glm::vec3 position{0.0f};
    float range = 0.0f;
    glm::vec3 color{1.0f};
    float intensity = 0.0f;
};

struct FrameLightData {
    std::array<DirectionalLightRenderData, MAX_FRAME_DIRECTIONAL_LIGHTS> directionalLightStorage{};
    std::array<PointLightRenderData, MAX_FRAME_POINT_LIGHTS> pointLightStorage{};
    std::uint32_t directionalLightCount = 0;
    std::uint32_t pointLightCount = 0;
    glm::vec3 ambientColor{1.0f};
    float ambientIntensity = 0.2f;

    [[nodiscard]] std::span<const DirectionalLightRenderData> directionalLights() const {
        return std::span<const DirectionalLightRenderData>(directionalLightStorage)
            .first(static_cast<std::size_t>(directionalLightCount));
    }

    [[nodiscard]] std::span<const PointLightRenderData> pointLights() const {
        return std::span<const PointLightRenderData>(pointLightStorage)
            .first(static_cast<std::size_t>(pointLightCount));
    }

    template <std::ranges::sized_range Source, typename Fn>
    void setDirectionalLights(const Source& source, Fn convert) {
        const std::size_t count = std::min(std::ranges::size(source), directionalLightStorage.size());
        auto first = std::ranges::begin(source);
        for (std::size_t i = 0; i < count; ++i, ++first) { directionalLightStorage.at(i) = convert(*first); }
        directionalLightCount = static_cast<std::uint32_t>(count);
    }

    template <std::ranges::sized_range Source, typename Fn>
    void setPointLights(const Source& source, Fn convert) {
        const std::size_t count = std::min(std::ranges::size(source), pointLightStorage.size());
        auto first = std::ranges::begin(source);
        for (std::size_t i = 0; i < count; ++i, ++first) { pointLightStorage.at(i) = convert(*first); }
        pointLightCount = static_cast<std::uint32_t>(count);
    }
};

}  // namespace se::render