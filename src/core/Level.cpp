#include "Level.h"

#include "Config.h"
#include "Input.h"
#include "assets/AssetManager.h"
#include "render/RenderManager.h"
#include "scene/SceneBuilder.h"

namespace se::core {

Level::Level(const Config& config, Input& input, se::render::RenderManager& renderManager,
             se::assets::AssetManager& assetManager)
    : m_Input(&input), m_Player(config), m_World(config.world()) {
    // this would be a simplification of loading a level from a file
    se::scene::SceneBuilder::build(m_Scene, assetManager);
    // this would be entity-component style linking of the player controller to the player body model, instead of
    // hardcoding the "player_body" tag
    if (auto* body = m_Scene.findAnimatedInstance("player_body")) {
        m_Player.setBodyInstance(body);
    }
    renderManager.setTerrainShader(assetManager.getOrLoadShader("assets/shaders/voxel"));
}

// Update order:
// 1. Player::update - input → locomotion intent
// 2. Scene::update - animator samples pose, produces rootMotionDelta
// 3. Player::finalizeFrame - consumes delta, syncs transform and camera
void Level::update(float deltaTime) {
    if (m_Input == nullptr) {
        return;
    }

    m_PendingPlayerIntent.accumulateFrame(m_Player.sampleIntent(*m_Input));
    m_Accumulator += deltaTime;

    int steps = 0;
    while (m_Accumulator >= FIXED_SIMULATION_DT && steps < MAX_SIMULATION_STEPS_PER_FRAME) {
        m_Player.update(FIXED_SIMULATION_DT, m_PendingPlayerIntent);
        m_World.updateChunks(m_Scene, m_Player.getPosition());
        m_Scene.update(FIXED_SIMULATION_DT);
        m_Player.finalizeFrame(FIXED_SIMULATION_DT);
        m_PendingPlayerIntent.clearFrameEvents();
        m_Accumulator -= FIXED_SIMULATION_DT;
        ++steps;
    }

    if (m_Accumulator >= FIXED_SIMULATION_DT) {
        m_Accumulator = FIXED_SIMULATION_DT;
    }
}

void Level::render(se::render::RenderManager& renderManager) {
    renderManager.beginFrame(m_Player.getCamera());
    for (const auto& [pos, cr] : m_Scene.getChunkRenderables()) { renderManager.submit(cr); }
    for (const auto& r : m_Scene.getRenderables()) { renderManager.submit(r); }
    renderManager.endFrame(m_Scene.prepareLightData());
}

}  // namespace se::core