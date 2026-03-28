#include "Level.h"

#include "Config.h"
#include "Input.h"
#include "assets/AssetManager.h"
#include "render/RenderManager.h"
#include "scene/SceneBuilder.h"

namespace se::core {

Level::Level(const Config& config, se::render::RenderManager& renderManager, se::assets::AssetManager& assetManager)
    : m_Player(config), m_World(config.world()) {
    se::scene::SceneBuilder::build(m_Scene, assetManager);
    renderManager.setTerrainShader(assetManager.getOrLoadShader("assets/shaders/voxel"));
}

void Level::update(float deltaTime, const Input& input) {
    m_Player.update(deltaTime, input);
    m_World.updateChunks(m_Scene, m_Player.getPosition());
}

void Level::render(se::render::RenderManager& renderManager) {
    renderManager.beginFrame(m_Player.getCamera());
    for (const auto& [pos, cr] : m_Scene.getChunkRenderables()) { renderManager.submit(cr); }
    for (const auto& r : m_Scene.getRenderables()) { renderManager.submit(r); }
    renderManager.endFrame(m_Scene.prepareLightData());
}

}  // namespace se::core