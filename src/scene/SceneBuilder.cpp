#include "SceneBuilder.h"

#include <glm/glm.hpp>
#include <print>
#include <stdexcept>

#include "Scene.h"
#include "assets/AssetManager.h"
#include "core/Timer.h"
#include "scene/Light.h"
#include "scene/Sun.h"
#include "scene/Transform.h"

namespace se::scene {

void SceneBuilder::Build(Scene& scene, se::assets::AssetManager& assetManager) {
    LoadSky(scene);
    LoadModels(scene, assetManager);
}

void SceneBuilder::LoadSky(Scene& scene) {
    DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    sun.color = glm::vec3(1.0f, 0.95f, 0.9f);
    sun.intensity = 1.0f;
    scene.addDirectionalLight(sun);

    scene.getSky().setAmbientColor(glm::vec3(1.0f, 1.0f, 1.0f));
    scene.getSky().setAmbientStrength(0.7f);
}

void SceneBuilder::LoadModels(Scene& scene, se::assets::AssetManager& assetManager) {
}

void SceneBuilder::SubmitModel(const se::assets::ModelHandle& model, Scene& scene) {
    Transform t;
    t.position = {0.0f, 0.0f, 0.0f};
    t.scale = {0.1f, 0.1f, 0.1f};

    auto modelPtr = model.get();
    if (!modelPtr)
        throw std::runtime_error("Model handle is invalid");

    for (const auto& sub : modelPtr->getSubMeshes()) {
        if (!sub.mesh)
            throw std::runtime_error("SubMesh is missing mesh data");

        Renderable r;
        r.mesh = sub.mesh.get();
        r.material = sub.material;
        r.transform = t;
        scene.addRenderable(r);
    }
}
}  // namespace se::scene