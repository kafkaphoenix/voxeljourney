#pragma once

#include "assets/AssetHandle.h"

namespace se::assets {
class AssetManager;
class Model;
}

namespace se::scene {

class Scene;
struct Transform;

class SceneBuilder {
public:
    SceneBuilder() = delete;

    static void build(Scene& scene, se::assets::AssetManager& assetManager);

private:
    static void createSky(Scene& scene);
    static void loadModels(Scene& scene, se::assets::AssetManager& assetManager);
    static void loadAnimatedModels(Scene& scene, se::assets::AssetManager& assetManager);
    static void submitModel(const se::assets::ModelHandle& model, const Transform& transform, Scene& scene);
    static void submitAnimatedModel(const se::assets::ModelHandle& model, const Transform& transform, std::string tag,
                                    Scene& scene);
};

}  // namespace se::scene