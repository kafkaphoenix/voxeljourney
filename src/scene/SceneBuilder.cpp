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
        .direction = glm::normalize(glm::vec3(-0.5f, -0.5f, -0.5f)),  // 45 degree angle from above and front
        .color = glm::vec3(1.0f, 0.92f, 0.75f),                       // warm sunlight
        .intensity = 2.0f,
    });

    // Additional fill light to reduce contrast and give impression of light bouncing around the scene. Not physically
    // accurate but looks better.
    scene.addDirectionalLight(DirectionalLight{
        .direction = glm::normalize(glm::vec3(0.5f, 0.5f, 0.3f)),  // opposite of sun
        .color = glm::vec3(0.6f, 0.65f, 0.75f),                    // cool fill
        .intensity = 0.2f,
    });

    scene.getSky().setAmbientColor(glm::vec3(0.5f, 0.55f, 0.65f));  // cool blue-grey
    scene.getSky().setAmbientIntensity(0.4f);
}

void SceneBuilder::loadModels(Scene& scene, se::assets::AssetManager& assetManager) {
    se::core::Timer timer;
    auto animShader = assetManager.getOrLoadShader("assets/shaders/animated_model", "assets/shaders/model");
    auto handle = assetManager.getOrLoadModel("assets/models/fox.glb", animShader);
    std::println("Animated models loaded in {} ms", timer.millis());
    submitAnimatedModel(handle, Transform{.scale = {0.1f, 0.1f, 0.1f}}, scene, "player_body",
                        AnimationController::LocomotionClips{.idle = "Survey", .walk = "Walk", .run = "Run"});
}

void SceneBuilder::submitModel(const se::assets::ModelHandle& handle, const Transform& transform, Scene& scene) {
    auto model = handle.get();
    if (!model) {
        throw std::runtime_error("Model handle is invalid");
    }

    for (const auto& sub : model->getSubMeshes()) {
        if (!sub.mesh) {
            throw std::runtime_error("SubMesh is missing mesh data");
        }

        scene.addRenderable(Renderable::makeStatic(sub.mesh.get(), sub.material, transform));
    }
}

void SceneBuilder::submitAnimatedModel(const se::assets::ModelHandle& handle, const Transform& transform, Scene& scene,
                                       std::string animatedTag,
                                       const AnimationController::LocomotionClips& locomotionClips) {
    auto model = handle.get();
    if (!model) {
        throw std::runtime_error("Model handle is invalid");
    }

    auto& animatedInstance =
        scene.addAnimatedInstance(AnimatedInstance(handle, transform, std::move(animatedTag), locomotionClips));

    for (const auto& sub : model->getSubMeshes()) {
        if (!sub.mesh) {
            throw std::runtime_error("SubMesh is missing mesh data");
        }

        scene.addRenderable(Renderable::makeAnimated(sub.mesh.get(), sub.material, animatedInstance.transform,
                                                     animatedInstance.animator));
    }
}
}  // namespace se::scene