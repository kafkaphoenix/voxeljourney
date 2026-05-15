#include "RenderManager.h"

#include <glad/glad.h>

#include <algorithm>
#include <stdexcept>

namespace se::render {

RenderManager::RenderManager(const se::core::Config::Render& renderConfig)
    : m_ModelRenderer(renderConfig.anisotropy), m_MsaaSamples((std::max)(1, renderConfig.msaaSamples)) {
    setupGlState();
}

void RenderManager::beginFrame(const se::scene::Camera& camera) {
    m_Camera = &camera;
    m_Frustum = calculateFrustum(m_Camera->getViewProjection());

    if (m_SceneMsaaFbo) {
        m_SceneMsaaFbo->bind();
        m_SceneMsaaFbo->clear();
    } else if (m_SceneFinalFbo) {
        m_SceneFinalFbo->bind();
        m_SceneFinalFbo->clear();
    }
}

void RenderManager::submit(const se::scene::Renderable& renderable) {
    if (!m_Camera) {
        throw std::runtime_error("RenderManager: submit called before beginFrame!");
    }

    m_ModelRenderer.submit(renderable, m_Frustum, m_Camera->getViewProjection());
}

void RenderManager::submit(const se::scene::ChunkRenderable& chunkRenderable) {
    if (!m_Camera) {
        throw std::runtime_error("RenderManager: submit called before beginFrame!");
    }
    m_TerrainRenderer.submit(chunkRenderable, m_Frustum);
}

void RenderManager::endFrame(const se::scene::LightData& lights) {
    if (!m_Camera) {
        throw std::runtime_error("RenderManager: endFrame called before beginFrame!");
    }

    m_Stats.reset();
    m_TerrainRenderer.flush(lights, *m_Camera, m_Stats);
    m_ModelRenderer.flush(lights, *m_Camera, m_Stats);

    if (m_SceneMsaaFbo && m_SceneFinalFbo) {
        resolveMsaaToFinalFramebuffer();
        m_PostProcess.execute(*m_SceneFinalFbo);
    } else if (m_SceneFinalFbo) {
        m_PostProcess.execute(*m_SceneFinalFbo);
    }

    m_Camera = nullptr;
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

void RenderManager::cyclePostEffect() { m_PostProcess.cycleEffect(); }

void RenderManager::setPostEffect(PostEffect effect) { m_PostProcess.setEffect(effect); }

void RenderManager::setBatchSize(const size_t maxInstances) { m_ModelRenderer.setBatchSize(maxInstances); }

void RenderManager::setTerrainShader(se::assets::ShaderHandle shader) { m_TerrainRenderer.setShader(shader); }

void RenderManager::reset() {
    m_Stats.reset();
    m_Camera = nullptr;
}

void RenderManager::initFramebuffer(int width, int height) {
    m_SceneFinalFbo.emplace(FramebufferSpec{
        .width = width,
        .height = height,
        .samples = 1,
        .colorAttachments = {GL_RGBA16F},
        .depthStencil = true,
    });

    if (m_MsaaSamples > 1) {
        m_SceneMsaaFbo.emplace(FramebufferSpec{
            .width = width,
            .height = height,
            .samples = m_MsaaSamples,
            .colorAttachments = {GL_RGBA16F},
            .depthStencil = true,
        });
    }
}

void RenderManager::resolveMsaaToFinalFramebuffer() const {
    if (!m_SceneMsaaFbo || !m_SceneFinalFbo) {
        return;
    }

    glBlitNamedFramebuffer(m_SceneMsaaFbo->id(), m_SceneFinalFbo->id(), 0, 0, m_SceneMsaaFbo->width(),
                           m_SceneMsaaFbo->height(), 0, 0, m_SceneFinalFbo->width(), m_SceneFinalFbo->height(),
                           GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
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