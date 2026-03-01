#include "Material.h"

namespace se::assets {

Material::Material(const std::string& name,
                   ShaderHandle shader,
                   const MaterialTextures& textures,
                   const MaterialParams& params,
                   const RenderState& state)
    : Asset(name), m_Shader(shader), m_Textures(textures), m_Params(params), m_State(state) {}

}  // namespace se::assets
