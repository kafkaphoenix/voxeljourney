#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>

#include "Asset.h"
#include "AssetHandle.h"

namespace se::assets {

struct RenderState {
    bool blend = false;
    bool depthWrite = true;
    bool cull = true;
};

struct MaterialTextures {
    TextureHandle baseColor;
    TextureHandle metallicRoughness;
    TextureHandle normal;
    TextureHandle emissive;
    TextureHandle occlusion;
};

struct MaterialParams {
    glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    glm::vec3 emissiveFactor{0.0f, 0.0f, 0.0f};
    float alphaCutoff = 0.5f;
};

class Material : public Asset {
   public:
    explicit Material(std::string name,
                      ShaderHandle shader,
                      const MaterialTextures& textures,
                      const MaterialParams& params,
                      const RenderState& state);
    ~Material() override = default;

    const ShaderHandle& getShaderHandle() const { return m_Shader; }
    const TextureHandle& getBaseColorHandle() const { return m_Textures.baseColor; }
    const TextureHandle& getAlbedoHandle() const { return m_Textures.baseColor; }
    const MaterialTextures& getTextures() const { return m_Textures; }
    const MaterialParams& getParams() const { return m_Params; }
    const RenderState& getState() const { return m_State; }

    std::string_view getPath() const override { return m_Path; }

   private:
    ShaderHandle m_Shader;
    MaterialTextures m_Textures;
    MaterialParams m_Params;
    RenderState m_State;
};

}  // namespace se::assets
