#include "RenderManager.h"

#include <glad/glad.h>

#include <stdexcept>

#include "SceneRenderAdapters.h"
#include "core/Config.h"
#include "scene/ChunkRenderable.h"

namespace se::render {

RenderManager::RenderManager(const se::core::Config& config)
    : m_ModelRenderer(config.render().anisotropy),
      m_MsaaSamples((std::max)(1, config.render().msaaSamples)),
      m_PostProcess(config.postProcess()) {
    setupGlState();
}

void RenderManager::beginFrame(const se::scene::Camera& camera) {
    m_FrameCamera = toFrameCameraData(camera);
    m_Frustum = calculateFrustum(m_FrameCamera->projectionMatrix * m_FrameCamera->viewMatrix);

    if (m_SceneMsaaFbo) {
        m_SceneMsaaFbo->bind();
        clearSceneFramebuffer(*m_SceneMsaaFbo);
    } else if (m_SceneFinalFbo) {
        m_SceneFinalFbo->bind();
        clearSceneFramebuffer(*m_SceneFinalFbo);
    }
}

void RenderManager::submit(const se::scene::Renderable& renderable) {
    if (!m_FrameCamera) {
        throw std::runtime_error("RenderManager: submit called before beginFrame!");
    }

    if (!visibility::isVisible(renderable.visibilityMask, m_FrameCamera->visibilityMask)) {
        return;
    }

    m_ModelRenderer.submit(toModelSubmission(renderable), m_Frustum, m_FrameCamera->viewMatrix);
}

void RenderManager::submit(const se::scene::ChunkRenderable& chunkRenderable) {
    if (!m_FrameCamera) {
        throw std::runtime_error("RenderManager: submit called before beginFrame!");
    }

    m_TerrainRenderer.submit(toTerrainSubmission(chunkRenderable), m_Frustum);
}

void RenderManager::endFrame(const se::scene::LightData& lights) {
    if (!m_FrameCamera) {
        throw std::runtime_error("RenderManager: endFrame called before beginFrame!");
    }

    const FrameLightData frameLights = toFrameLightData(lights);

    m_Stats.reset();

    if (m_SceneMsaaFbo && m_SceneFinalFbo) {
        m_SceneMsaaFbo->bind();
        m_TerrainRenderer.flush(frameLights, *m_FrameCamera, m_Stats);
        m_ModelRenderer.flushOpaque(frameLights, *m_FrameCamera, m_Stats);

        resolveMsaaSceneToFinalFramebuffer();

        m_SceneFinalFbo->bind();
        clearTransparencyTargets(*m_SceneFinalFbo);
        m_ModelRenderer.flushTransparent(frameLights, *m_FrameCamera, m_Stats);
        m_ModelRenderer.clearQueuedDraws();

        m_PostProcess.execute(*m_SceneFinalFbo);
    } else if (m_SceneFinalFbo) {
        m_TerrainRenderer.flush(frameLights, *m_FrameCamera, m_Stats);
        m_ModelRenderer.flush(frameLights, *m_FrameCamera, m_Stats);
        m_PostProcess.execute(*m_SceneFinalFbo);
    }

    m_FrameCamera.reset();
}

void RenderManager::resizeFramebuffer(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    if (!m_SceneFinalFbo) {
        initFramebuffer(width, height);
    } else {
        m_SceneFinalFbo->resize(width, height);
        if (m_SceneMsaaFbo) {
            m_SceneMsaaFbo->resize(width, height);
        }
    }
}

void RenderManager::toggleWireframe() {
    m_Wireframe = !m_Wireframe;
    m_TerrainRenderer.setWireframe(m_Wireframe);
    m_ModelRenderer.setWireframe(m_Wireframe);
}

void RenderManager::cycleRenderDebugView() { m_ModelRenderer.cycleRenderDebugView(); }

void RenderManager::cyclePostEffect() { m_PostProcess.cycleEffect(); }

void RenderManager::setPostEffect(PostEffect effect) { m_PostProcess.setEffect(effect); }

void RenderManager::setBatchSize(const size_t maxInstances) { m_ModelRenderer.setBatchSize(maxInstances); }

void RenderManager::setTerrainShader(se::assets::ShaderHandle shader) { m_TerrainRenderer.setShader(shader); }

void RenderManager::reset() {
    m_Stats.reset();
    m_FrameCamera.reset();
}

void RenderManager::initFramebuffer(int width, int height) {
    m_SceneFinalFbo.emplace(FramebufferSpec{
        .width = width,
        .height = height,
        .samples = 1,
        .colorAttachments = {GL_RGBA16F, GL_RGBA16F, GL_R8},
        .depthStencil = true,
    });

    if (m_MsaaSamples > 1) {
        m_SceneMsaaFbo.emplace(FramebufferSpec{
            .width = width,
            .height = height,
            .samples = m_MsaaSamples,
            .colorAttachments = {GL_RGBA16F, GL_RGBA16F, GL_R8},
            .depthStencil = true,
        });
    }
}

void RenderManager::clearSceneFramebuffer(const Framebuffer& framebuffer) {
    constexpr std::array<GLfloat, 4> SCENE_COLOR = {0.2f, 0.3f, 0.8f, 1.0f};
    constexpr std::array<GLfloat, 4> OIT_ACCUM_CLEAR = {0.0f, 0.0f, 0.0f, 0.0f};
    constexpr std::array<GLfloat, 1> OIT_REVEAL_CLEAR = {1.0f};

    glClearNamedFramebufferfv(framebuffer.id(), GL_COLOR, 0, SCENE_COLOR.data());
    glClearNamedFramebufferfv(framebuffer.id(), GL_COLOR, 1, OIT_ACCUM_CLEAR.data());
    glClearNamedFramebufferfv(framebuffer.id(), GL_COLOR, 2, OIT_REVEAL_CLEAR.data());

    constexpr GLfloat DEPTH_CLEAR = 1.0f;
    glClearNamedFramebufferfv(framebuffer.id(), GL_DEPTH, 0, &DEPTH_CLEAR);
}

void RenderManager::clearTransparencyTargets(const Framebuffer& framebuffer) {
    constexpr std::array<GLfloat, 4> OIT_ACCUM_CLEAR = {0.0f, 0.0f, 0.0f, 0.0f};
    constexpr std::array<GLfloat, 1> OIT_REVEAL_CLEAR = {1.0f};

    glClearNamedFramebufferfv(framebuffer.id(), GL_COLOR, 1, OIT_ACCUM_CLEAR.data());
    glClearNamedFramebufferfv(framebuffer.id(), GL_COLOR, 2, OIT_REVEAL_CLEAR.data());
}

void RenderManager::resolveMsaaSceneToFinalFramebuffer() const {
    if (!m_SceneMsaaFbo || !m_SceneFinalFbo) {
        return;
    }

    // Resolve only scene color attachment; OIT attachments are rendered directly on single-sample FBO.
    glNamedFramebufferReadBuffer(m_SceneMsaaFbo->id(), GL_COLOR_ATTACHMENT0);
    glNamedFramebufferDrawBuffer(m_SceneFinalFbo->id(), GL_COLOR_ATTACHMENT0);
    glBlitNamedFramebuffer(m_SceneMsaaFbo->id(), m_SceneFinalFbo->id(), 0, 0, m_SceneMsaaFbo->width(),
                           m_SceneMsaaFbo->height(), 0, 0, m_SceneFinalFbo->width(), m_SceneFinalFbo->height(),
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glNamedFramebufferReadBuffer(m_SceneMsaaFbo->id(), GL_NONE);
    glNamedFramebufferDrawBuffer(m_SceneFinalFbo->id(), GL_NONE);
    glBlitNamedFramebuffer(m_SceneMsaaFbo->id(), m_SceneFinalFbo->id(), 0, 0, m_SceneMsaaFbo->width(),
                           m_SceneMsaaFbo->height(), 0, 0, m_SceneFinalFbo->width(), m_SceneFinalFbo->height(),
                           GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glNamedFramebufferReadBuffer(m_SceneMsaaFbo->id(), GL_COLOR_ATTACHMENT0);

    constexpr std::array<GLenum, 3> FINAL_DRAW_BUFFERS = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                                          GL_COLOR_ATTACHMENT2};
    glNamedFramebufferDrawBuffers(m_SceneFinalFbo->id(), static_cast<GLsizei>(FINAL_DRAW_BUFFERS.size()),
                                  FINAL_DRAW_BUFFERS.data());
}

void RenderManager::setupGlState() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

}  // namespace se::render
