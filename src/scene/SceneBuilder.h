#pragma once

#include "assets/AssetHandle.h"

namespace se::assets {
class AssetManager;
}

namespace se::scene {

class Scene;

class SceneBuilder {
public:
    SceneBuilder() = delete;

    static void build(Scene& scene, se::assets::AssetManager& assetManager);

private:
    static void createSky(Scene& scene);
    static void loadModels(Scene& scene, se::assets::AssetManager& assetManager);
    static void submitModel(const se::assets::ModelHandle& model, Scene& scene);
};

}  // namespace se::scene