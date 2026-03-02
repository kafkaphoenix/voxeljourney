#pragma once
#include <memory>
#include <vector>

#include "Player.h"
#include "Renderable.h"
#include "Sky.h"
#include "assets/AssetManager.h"
#include "assets/Model.h"
#include "render/Renderer.h"

namespace se::core {
class Input;
class Config;
}

namespace se::scene {

class Scene {
   public:
    Scene(float aspectRatio, se::assets::AssetManager& assetManager);
    ~Scene() = default;

    void addRenderable(const Renderable& renderable) { m_Renderables.push_back(renderable); }
    const std::vector<Renderable>& getRenderables() const { return m_Renderables; }

    Player& getPlayer() { return m_Player; }
    const Player& getPlayer() const { return m_Player; }

    se::render::Renderer::LightSet buildLightSet() const;

    void update(float deltaTime, const se::core::Input& input);
    void applyConfig(const se::core::Config& config);
    void initialize();

   private:
    void createSponzaModel();

    std::vector<Renderable> m_Renderables;
    Player m_Player;
    Sky m_Sky;
    std::vector<Light> m_PointLights;
    se::assets::AssetManager& m_AssetManager;
};

}  // namespace se::scene
