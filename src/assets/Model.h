#pragma once
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Animation.h"
#include "Asset.h"
#include "Material.h"
#include "Skeleton.h"
#include "StringHash.h"
#include "render/Mesh.h"

namespace se::assets {

class AssetManager;

struct SubMesh {
    std::unique_ptr<se::render::Mesh> mesh;
    MaterialHandle material;
};

class Model : public Asset {
public:
    Model(std::string gltfPath, ShaderHandle handle, AssetManager& assetManager);

    [[nodiscard]] const std::vector<SubMesh>& getSubMeshes() const { return m_SubMeshes; }
    [[nodiscard]] const std::vector<AnimationClip>& getAnimations() const { return m_Animations; }
    [[nodiscard]] std::optional<int> findAnimationClipIndex(std::string_view clipName) const;
    [[nodiscard]] const Skeleton& getSkeleton() const;

private:
    static std::string getDirectory(std::string_view filepath);
    std::vector<SubMesh> m_SubMeshes;
    std::optional<Skeleton> m_Skeleton;
    std::vector<AnimationClip> m_Animations;
    std::unordered_map<std::string, int, StringHash, std::equal_to<>> m_AnimationIndexByName;
};

}  // namespace se::assets