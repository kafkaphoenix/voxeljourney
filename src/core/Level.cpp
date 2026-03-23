#include "Level.h"

#include "assets/AssetManager.h"
#include "core/Config.h"
#include "core/Input.h"
#include "render/RenderManager.h"
#include "scene/SceneBuilder.h"

namespace se::core {

void Level::initialize(const se::core::Config& config, se::render::RenderManager& renderManager, se::assets::AssetManager& assetManager) {
    m_Player.applyConfig(config);

    se::scene::SceneBuilder::Build(m_Scene, assetManager);
}

void Level::update(float deltaTime, const se::core::Input& input) {
    m_Player.update(deltaTime, input);
}

void Level::render(se::render::RenderManager& renderManager) {
    renderManager.beginFrame(m_Player.getCamera());
    for (const auto& r : m_Scene.getRenderables())
        renderManager.submit(r);
    renderManager.endFrame(m_Scene.getLightData());
}

}  // namespace se::core