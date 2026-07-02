#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "FrameRenderData.h"
#include "ModelSubmission.h"
#include "TerrainSubmission.h"
#include "scene/Animator.h"
#include "scene/Camera.h"
#include "scene/ChunkRenderable.h"
#include "scene/LightData.h"
#include "scene/Renderable.h"
#include "voxel/ChunkCoords.h"

namespace se::render {

[[nodiscard]] inline FrameCameraData toFrameCameraData(const se::scene::Camera& camera) {
    return FrameCameraData{
        .viewMatrix = camera.getViewMatrix(),
        .projectionMatrix = camera.getProjectionMatrix(),
        .worldPosition = camera.getPosition(),
        .visibilityMask = camera.getVisibilityMask(),
    };
}

[[nodiscard]] inline FrameLightData toFrameLightData(const se::scene::LightData& lights) {
    FrameLightData frameLights{};
    frameLights.ambientColor = lights.ambientColor;
    frameLights.ambientIntensity = lights.ambientIntensity;

    frameLights.setDirectionalLights(lights.directionalLights, [](const se::scene::DirectionalLight& light) {
        return DirectionalLightRenderData{
            .direction = light.direction,
            .intensity = light.intensity,
            .color = light.color,
        };
    });

    frameLights.setPointLights(lights.pointLights, [](const se::scene::PointLight& light) {
        return PointLightRenderData{
            .position = light.position,
            .range = light.range,
            .color = light.color,
            .intensity = light.intensity,
        };
    });

    return frameLights;
}

[[nodiscard]] inline ModelSubmission toModelSubmission(const se::scene::Renderable& renderable) {
    const auto* animator = renderable.resolvedAnimator();
    return ModelSubmission{
        .mesh = renderable.mesh,
        .material = renderable.material.get(),
        .modelMatrix = renderable.resolvedTransform().getMatrix(),
        .bones = animator ? &animator->bones() : nullptr,
    };
}

[[nodiscard]] inline TerrainSubmission toTerrainSubmission(const se::scene::ChunkRenderable& chunkRenderable) {
    return TerrainSubmission{
        .mesh = chunkRenderable.mesh.get(),
        .modelMatrix = chunkRenderable.getMatrix(),
    };
}

}  // namespace se::render