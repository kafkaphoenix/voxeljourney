#pragma once
#include <glm/glm.hpp>
#include <optional>
#include <vector>

#include "Frustum.h"
#include "Mesh.h"
#include "RenderStats.h"
#include "UniformBuffer.h"
#include "assets/AssetHandle.h"

namespace se::scene {
class Camera;
struct LightData;
struct ChunkRenderable;
}
namespace se::assets {
class AssetManager;
}

namespace se::render {

class TerrainRenderer {
public:
    TerrainRenderer();

    void submit(const se::scene::ChunkRenderable& chunkRenderable, const Frustum& frustum);
    void flush(const se::scene::LightData& lights, const se::scene::Camera& camera, RenderStats& stats);
    void setWireframe(bool enabled) { m_Wireframe = enabled; }
    void setShader(se::assets::ShaderHandle shader) { m_Shader = shader; }

private:
    struct ChunkDraw {
        const Mesh* mesh = nullptr;
    };

    void updateUbo(const se::scene::LightData& lights, const se::scene::Camera& camera);

    se::assets::ShaderHandle m_Shader;
    std::vector<ChunkDraw> m_DrawList;
    std::optional<UniformBuffer> m_TerrainUbo;
    bool m_Wireframe = false;
};

}  // namespace se::render