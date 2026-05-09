#include "RenderManager.h"

#include <glad/glad.h>

#include <stdexcept>

namespace se::render {

RenderManager::RenderManager() { setupGlState(); }

void RenderManager::beginFrame(const se::scene::Camera& camera) {
    m_Camera = &camera;
    m_Frustum = calculateFrustum(m_Camera->getViewProjection());

    if (m_SceneFbo) {
        m_SceneFbo->bind();
        m_SceneFbo->clear(0.2f, 0.3f, 0.8f);
    }
}

void RenderManager::submit(const se::scene::Renderable& renderable) {
    if (!m_Camera) {
        throw std::runtime_error("RenderManager: submit called before beginFrame!");
    }

    m_ModelRenderer.submit(renderable, m_Frustum);
}

void RenderManager::submit(const se::scene::ChunkRenderable& chunkRenderable) {
    if (!m_Camera) {
        throw std::runtime_error("RenderManager: submit called before beginFrame!");
    }
    m_TerrainRenderer.submit(chunkRenderable, m_Frustum);
}

void RenderManager::submitAnimated(const se::scene::Renderable& renderable, std::span<const glm::mat4> boneMatrices) {
    if (!m_Camera) {
        throw std::runtime_error("RenderManager: submitAnimated called before beginFrame!");
    }

    m_ModelRenderer.submitAnimated(renderable, m_Frustum, boneMatrices);
}

void RenderManager::endFrame(const se::scene::LightData& lights) {
    if (!m_Camera) {
        throw std::runtime_error("RenderManager: endFrame called before beginFrame!");
    }

    m_Stats.reset();

    m_TerrainRenderer.flush(lights, *m_Camera, m_Stats);
    m_ModelRenderer.flush(lights, *m_Camera, m_Stats);

    if (m_SceneFbo) {
        m_PostProcess.execute(*m_SceneFbo);
    }

    m_Camera = nullptr;
}

void RenderManager::resizeFramebuffer(int width, int height) {
    if (width <= 0 || height <= 0)
        return;

    if (!m_SceneFbo) {
        initFramebuffer(width, height);
    } else {
        m_SceneFbo->resize(width, height);
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
    m_SceneFbo.emplace(FramebufferSpec{
        .width = width,
        .height = height,
        .colorAttachments = {GL_RGBA16F},
        .depthStencil = true,
    });
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