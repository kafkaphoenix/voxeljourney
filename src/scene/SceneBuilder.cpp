#include "SceneBuilder.h"

#include <glm/glm.hpp>
#include <print>
#include <stdexcept>

#include "Light.h"
#include "Renderable.h"
#include "Scene.h"
#include "Sun.h"
#include "Transform.h"
#include "assets/AssetManager.h"
#include "core/Timer.h"

namespace se::scene {

void SceneBuilder::build(Scene& scene, se::assets::AssetManager& assetManager) {
    createSky(scene);
    loadModels(scene, assetManager);
}

void SceneBuilder::createSky(Scene& scene) {
    scene.addDirectionalLight(DirectionalLight{
        .direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f)),
        .color = glm::vec3(1.0f, 0.95f, 0.9f),
        .intensity = 1.0f,
    });

    scene.getSky().setAmbientColor(glm::vec3(1.0f, 1.0f, 1.0f));
    scene.getSky().setAmbientStrength(0.7f);
}

void SceneBuilder::loadModels(Scene& scene, se::assets::AssetManager& assetManager) {
    se::core::Timer timer;
    auto animShader = assetManager.getOrLoadShader("assets/shaders/animated_model", "assets/shaders/model");
    auto handle = assetManager.getOrLoadModel("assets/models/fox.glb", animShader);
    std::println("Animated models loaded in {} ms", timer.millis());
    submitModel(handle, Transform{.scale = {0.1f, 0.1f, 0.1f}}, scene, "player_body");
}

void SceneBuilder::submitModel(const se::assets::ModelHandle& handle, const Transform& transform, Scene& scene,
                               std::optional<std::string> animatedTag) {
    auto model = handle.get();
    if (!model) {
        throw std::runtime_error("Model handle is invalid");
    }

    AnimatedInstance* animatedInstance = nullptr;
    if (animatedTag.has_value()) {
        animatedInstance = &scene.addAnimatedInstance(AnimatedInstance(handle, transform, std::move(*animatedTag)));
    }

    for (const auto& sub : model->getSubMeshes()) {
        if (!sub.mesh) {
            throw std::runtime_error("SubMesh is missing mesh data");
        }

        if (animatedInstance) {
            scene.addRenderable(Renderable::makeAnimated(sub.mesh.get(), sub.material, animatedInstance->transform,
                                                         animatedInstance->animator));
        } else {
            scene.addRenderable(Renderable::makeStatic(sub.mesh.get(), sub.material, transform));
        }
    }
}
}  // namespace se::scene