#pragma once
#include "scene/Player.h"
#include "scene/Scene.h"

namespace se::core {
class Config;
class Input;
}

namespace se::render {
class RenderManager;
}

namespace se::assets {
class AssetManager;
}

namespace se::core {

class Level {
   public:
    Level() = default;
    ~Level() = default;

    void initialize(const se::core::Config& config, se::render::RenderManager& renderManager, se::assets::AssetManager& assetManager);
    void update(float deltaTime, const se::core::Input& input);
    void render(se::render::RenderManager& renderManager);

    se::scene::Player& getPlayer() { return m_Player; }
    const se::scene::Player& getPlayer() const { return m_Player; }

   private:
    se::scene::Scene m_Scene;
    se::scene::Player m_Player;
};

}  // namespace se::core