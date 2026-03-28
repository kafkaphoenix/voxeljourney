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

void SceneBuilder::loadModels(Scene& scene, se::assets::AssetManager& assetManager) {}

void SceneBuilder::submitModel(const se::assets::ModelHandle& model, Scene& scene) {
    auto modelPtr = model.get();
    if (!modelPtr) {
        throw std::runtime_error("Model handle is invalid");
    }

    for (const auto& sub : modelPtr->getSubMeshes()) {
        if (!sub.mesh) {
            throw std::runtime_error("SubMesh is missing mesh data");
        }

        scene.addRenderable(Renderable{
            .mesh = sub.mesh.get(),
            .material = sub.material,
            .transform =
                Transform{
                    .position = {0.0f, 0.0f, 0.0f},
                },
        });
    }
}
}  // namespace se::scene