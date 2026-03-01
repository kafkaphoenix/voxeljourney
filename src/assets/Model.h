#pragma once

#include <memory>
#include <string>
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
    Model(const std::string& gltfPath,
          const std::string& shaderPath,
          AssetManager& assetManager);

    const std::vector<SubMesh>& getSubMeshes() const { return m_SubMeshes; }
    const std::string& getPath() const override { return m_Path; }

   private:
    static std::string getDirectory(const std::string& filepath);

    std::vector<SubMesh> m_SubMeshes;
};

}  // namespace se::assets