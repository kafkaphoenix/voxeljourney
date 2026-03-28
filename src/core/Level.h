#pragma once
#include "scene/Player.h"
#include "scene/Scene.h"

namespace se::render {
class RenderManager;
}

namespace se::assets {
class AssetManager;
}

namespace se::core {

class Level {
public:
    Level(const Config& config, se::render::RenderManager& renderManager, se::assets::AssetManager& assetManager);
    ~Level() = default;

    void update(float deltaTime, const Input& input);
    void render(se::render::RenderManager& renderManager);

    [[nodiscard]] se::scene::Player& getPlayer() { return m_Player; }
    [[nodiscard]] const se::scene::Player& getPlayer() const { return m_Player; }

private:
    se::scene::Scene m_Scene;
    se::scene::Player m_Player;
};

}  // namespace se::core