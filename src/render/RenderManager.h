#pragma once

#include <glm/mat4x4.hpp>
#include <optional>

#include "FrameRenderData.h"
#include "Framebuffer.h"
#include "Frustum.h"
#include "ModelRenderer.h"
#include "PostProcessRenderer.h"
#include "RenderStats.h"
#include "TerrainRenderer.h"

namespace se::scene {
class Camera;
struct LightData;
struct Renderable;
struct ChunkRenderable;
}

namespace se::core {
class Config;
}

namespace se::render {

class RenderManager {
public:
    explicit RenderManager(const se::core::Config& config);

    void beginFrame(const se::scene::Camera& camera);
    void submit(const se::scene::Renderable& renderable);
    void submit(const se::scene::ChunkRenderable& chunkRenderable);
    void endFrame(const se::scene::LightData& lights);

    void resizeFramebuffer(int width, int height);
    void toggleWireframe();
    void cycleRenderDebugView();
    void setBatchSize(size_t maxInstances);
    void setTerrainShader(se::assets::ShaderHandle shader);
    void reset();

    [[nodiscard]] const RenderStats& getStats() const noexcept { return m_Stats; }

private:
    void initFramebuffer(int width, int height);
    static void clearSceneFramebuffer(const Framebuffer& framebuffer);
    static void clearTransparencyTargets(const Framebuffer& framebuffer);
    void resolveMsaaSceneToFinalFramebuffer() const;
    static void setupGlState();

    std::optional<FrameCameraData> m_FrameCamera;
    Frustum m_Frustum{};
    ModelRenderer m_ModelRenderer;
    TerrainRenderer m_TerrainRenderer;
    PostProcessRenderer m_PostProcess;
    RenderStats m_Stats;
    bool m_Wireframe = false;
    int m_MsaaSamples = 4;
    std::optional<Framebuffer> m_SceneMsaaFbo;
    std::optional<Framebuffer> m_SceneFinalFbo;
};

}  // namespace se::render
