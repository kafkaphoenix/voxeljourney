#include "Material.h"

namespace se::assets {

Material::Material(std::string path, ShaderHandle shader, const MaterialTextures& textures,
                   const MaterialParams& params, const RenderState& state)
    : Asset(std::move(path)), m_Shader(shader), m_Textures(textures), m_Params(params), m_State(state) {}

}  // namespace se::assets
