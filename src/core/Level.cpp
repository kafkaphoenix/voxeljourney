#include "Level.h"

#include "Config.h"
#include "Input.h"
#include "assets/AssetManager.h"
#include "render/RenderManager.h"
#include "scene/SceneBuilder.h"

namespace se::core {

Level::Level(const Config& config, se::render::RenderManager& renderManager, se::assets::AssetManager& assetManager)
    : m_Player(config), m_World(config.world()) {
    // this would be a simplification of loading a level from a file
    se::scene::SceneBuilder::build(m_Scene, assetManager);
    // this would be entity-component style linking of the player controller to the player body model, instead of
    // hardcoding the "player_body" tag
    if (auto* body = m_Scene.findAnimatedInstance("player_body")) {
        m_Player.setBodyInstance(body);
    }
    renderManager.setTerrainShader(assetManager.getOrLoadShader("assets/shaders/voxel"));
}

void Level::update(float deltaTime, const Input& input) {
    m_Player.update(deltaTime, input);
    m_World.updateChunks(m_Scene, m_Player.getPosition());
    m_Scene.update(deltaTime);
}

void Level::render(se::render::RenderManager& renderManager) {
    renderManager.beginFrame(m_Player.getCamera());
    for (const auto& [pos, cr] : m_Scene.getChunkRenderables()) { renderManager.submit(cr); }
    for (const auto& r : m_Scene.getRenderables()) { renderManager.submit(r); }
    renderManager.endFrame(m_Scene.prepareLightData());
}

}  // namespace se::core