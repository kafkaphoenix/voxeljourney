#pragma once

#include <optional>
#include <vector>

#include "FrameRenderData.h"
#include "Frustum.h"
#include "Mesh.h"
#include "RenderStats.h"
#include "TerrainSubmission.h"
#include "UniformBuffer.h"
#include "assets/AssetHandle.h"

namespace se::assets {
class Shader;
}

namespace se::render {

class TerrainRenderer {
public:
    TerrainRenderer();

    void submit(const TerrainSubmission& submission, const Frustum& frustum);
    void flush(const FrameLightData& lights, const FrameCameraData& camera, RenderStats& stats);
    void setWireframe(bool enabled) { m_Wireframe = enabled; }
    void setShader(se::assets::ShaderHandle shader) { m_Shader = shader; }

private:
    void updateUbo(const FrameLightData& lights, const FrameCameraData& camera);

    se::assets::ShaderHandle m_Shader;
    std::vector<const Mesh*> m_Meshes;
    std::optional<UniformBuffer> m_TerrainUbo;
    bool m_Wireframe = false;
};

}  // namespace se::render