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

    static void Build(Scene& scene, se::assets::AssetManager& assetManager);

   private:
    static void LoadSky(Scene& scene);
    static void LoadModels(Scene& scene, se::assets::AssetManager& assetManager);
    static void SubmitModel(const se::assets::ModelHandle& model, Scene& scene);
};

}  // namespace se::scene