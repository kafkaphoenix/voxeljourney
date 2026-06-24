#pragma once

#include "scene/Player.h"
#include "scene/Scene.h"
#include "voxel/World.h"

namespace se::render {
class RenderManager;
}

namespace se::assets {
class AssetManager;
}

namespace se::core {

class Input;

class Level {
public:
    Level(const Config& config, Input& input, se::render::RenderManager& renderManager,
          se::assets::AssetManager& assetManager);
    ~Level() = default;

    void update(float deltaTime);
    void render(se::render::RenderManager& renderManager);

    [[nodiscard]] se::scene::Player& getPlayer() { return m_Player; }
    [[nodiscard]] const se::scene::Player& getPlayer() const { return m_Player; }

private:
    static constexpr float FIXED_SIMULATION_DT = 1.0f / 60.0f;
    static constexpr int MAX_SIMULATION_STEPS_PER_FRAME = 4;

    Input* m_Input = nullptr;
    float m_Accumulator = 0.0f;
    se::scene::PlayerIntent m_PendingPlayerIntent{};
    se::scene::Scene m_Scene;
    se::scene::Player m_Player;
    se::voxel::World m_World;
};

}  // namespace se::core