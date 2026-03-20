#include "WorldLoader.h"

#include <glm/glm.hpp>
#include <print>
#include <stdexcept>

#include "World.h"
#include "assets/AssetManager.h"
#include "core/Timer.h"
#include "world/Light.h"
#include "world/Sun.h"
#include "world/Transform.h"

namespace se::world {

WorldLoader::WorldLoader(se::assets::AssetManager& assetManager)
    : m_AssetManager(assetManager) {}

void WorldLoader::load(World& world) {
    loadSky(world);
    loadModels(world);
}

void WorldLoader::loadSky(World& world) {
    DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    sun.color = glm::vec3(1.0f, 0.95f, 0.9f);
    sun.intensity = 1.0f;
    world.addDirectionalLight(sun);

    world.getSky().setAmbientColor(glm::vec3(1.0f, 1.0f, 1.0f));
    world.getSky().setAmbientStrength(0.7f);
}

void WorldLoader::loadModels(World& world) {
}

void WorldLoader::submitModel(const se::assets::ModelHandle& model, World& world) {
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
        world.addRenderable(r);
    }
}
}  // namespace se::world