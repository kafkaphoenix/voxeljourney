#pragma once

#include <glm/mat4x4.hpp>
#include <memory>
#include <optional>
#include <span>

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
struct AnimatedRenderable;
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
    void cyclePostEffect();
    void setPostEffect(PostEffect effect);
    void setBatchSize(size_t maxInstances);
    void setTerrainShader(se::assets::ShaderHandle shader);
    void reset();

    [[nodiscard]] PostEffect getPostEffect() const noexcept { return m_PostProcess.getEffect(); }
    [[nodiscard]] const RenderStats& getStats() const noexcept { return m_Stats; }

private:
    void initFramebuffer(int width, int height);
    void resolveMsaaToFinalFramebuffer() const;
    static void setupGlState();

    const se::scene::Camera* m_Camera = nullptr;
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