#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Asset.h"
#include "Material.h"
#include "render/Mesh.h"

namespace se::assets {

class AssetManager;

struct SubMesh {
    std::unique_ptr<se::render::Mesh> mesh;
    MaterialHandle material;
};

class Model : public Asset {
public:
    Model(std::string gltfPath, std::string_view shaderPath, AssetManager& assetManager);

    [[nodiscard]] const std::vector<SubMesh>& getSubMeshes() const { return m_SubMeshes; }

private:
    static std::string getDirectory(std::string_view filepath);
    std::vector<SubMesh> m_SubMeshes;
};

}  // namespace se::assets