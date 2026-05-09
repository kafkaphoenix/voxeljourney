#pragma once
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Animation.h"
#include "Asset.h"
#include "Material.h"
#include "Skeleton.h"
#include "render/Mesh.h"

namespace se::assets {

class AssetManager;

struct SubMesh {
    std::unique_ptr<se::render::Mesh> mesh;
    MaterialHandle material;
};

class Model : public Asset {
public:
    Model(std::string gltfPath, ShaderHandle shader, AssetManager& assetManager);

    [[nodiscard]] const std::vector<SubMesh>& getSubMeshes() const { return m_SubMeshes; }
    [[nodiscard]] bool isAnimated() const { return m_Skeleton.has_value(); }
    [[nodiscard]] const Skeleton& getSkeleton() const { return *m_Skeleton; }
    [[nodiscard]] const std::vector<AnimationClip>& getAnimations() const { return m_Animations; }

private:
    static std::string getDirectory(std::string_view filepath);
    std::vector<SubMesh> m_SubMeshes;
    std::optional<Skeleton> m_Skeleton;
    std::vector<AnimationClip> m_Animations;
};

}  // namespace se::assets