#pragma once

#include "Frustum.h"
#include "ModelRenderer.h"
#include "RenderStats.h"
#include "TerrainRenderer.h"

namespace se::scene {
class Camera;
struct LightData;
struct Renderable;
struct ChunkRenderable;
}

namespace se::render {

class RenderManager {
public:
    RenderManager();

    void beginFrame(const se::scene::Camera& camera);
    void submit(const se::scene::Renderable& renderable);
    void submit(const se::scene::ChunkRenderable& chunkRenderable);
    void endFrame(const se::scene::LightData& lights);

    void toggleWireframe();
    void setBatchSize(size_t maxInstances);
    void setTerrainShader(se::assets::ShaderHandle shader);
    void reset();

    [[nodiscard]] const RenderStats& getStats() const noexcept { return m_Stats; }

private:
    static void clear();
    static void setupGlState();

    const se::scene::Camera* m_Camera = nullptr;
    Frustum m_Frustum{};
    ModelRenderer m_ModelRenderer;
    TerrainRenderer m_TerrainRenderer;
    RenderStats m_Stats;
    bool m_Wireframe = false;
};

}  // namespace se::render