#include "Scene.h"

#include <print>
#include <stdexcept>

#include "core/Config.h"
#include "core/Input.h"
#include "core/Timer.h"

namespace se::scene {

Scene::Scene(float aspectRatio, se::assets::AssetManager& assetManager) : m_Player(aspectRatio), m_AssetManager(assetManager) {
}

void Scene::initialize() {
    createSponzaModel();
}

void Scene::createSponzaModel() {
    se::core::Timer timer;
    Transform t;
    t.position = {0.0f, 0.0f, 0.0f};
    t.scale = {0.1f, 0.1f, 0.1f};
    std::string shaderPath = "assets/shaders/basic";
    std::string modelPath = "assets/models/sponza_glb/sponza.glb";
    // std::string modelPath = "assets/models/sponza/sponza.gltf";
    auto shader = m_AssetManager.getOrLoadShader(shaderPath);
    auto model = m_AssetManager.getOrLoadModel(modelPath, shaderPath);

    auto modelPtr = model.get();
    if (!modelPtr) {
        throw std::runtime_error("Model handle is invalid");
    }

    for (const auto& sub : modelPtr->getSubMeshes()) {
        if (!sub.mesh) {
            throw std::runtime_error("SubMesh is missing mesh data");
        }
        Renderable renderable;
        renderable.mesh = sub.mesh.get();
        renderable.material = sub.material;
        renderable.transform = t;
        addRenderable(renderable);
    }
    std::println("Sponza model loaded in {} ms", timer.get_milliseconds());
}

void Scene::update(float deltaTime, const se::core::Input& input) {
    m_Player.update(deltaTime, input);
}

void Scene::applyConfig(const se::core::Config& config) {
    se::scene::Camera::Settings settings;
    settings.position = {config.camera().startPosX, config.camera().startPosY, config.camera().startPosZ};
    settings.moveSpeed = config.camera().moveSpeed;
    settings.mouseSensitivity = config.input().mouseSensitivity;
    settings.fov = config.camera().fov;
    settings.nearPlane = config.camera().nearPlane;
    settings.farPlane = config.camera().farPlane;
    m_Player.getCamera().applySettings(settings);
    m_Player.setMouseSmoothing(config.input().mouseSmoothAlpha);
    m_Player.setFixedStep(config.input().fixedStep);
}

se::render::Renderer::LightSet Scene::buildLightSet() const {
    se::render::Renderer::LightSet lights;
    const auto& sun = m_Sky.getSun().getLight();
    lights.sunDir = sun.direction;
    lights.sunColor = sun.color * sun.intensity;
    lights.ambientColor = m_Sky.getAmbientColor();
    lights.ambientStrength = m_Sky.getAmbientStrength();

    lights.pointLights.reserve(m_PointLights.size());
    for (const auto& light : m_PointLights) {
        if (light.type != se::scene::LightType::Point) {
            continue;
        }
        se::render::Renderer::PointLightData data;
        data.position = light.position;
        data.color = light.color;
        data.intensity = light.intensity;
        data.range = light.range;
        lights.pointLights.push_back(data);
    }

    return lights;
}

}  // namespace se::scene
