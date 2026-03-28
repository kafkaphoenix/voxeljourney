#include "RenderManager.h"

#include <glad/glad.h>

#include <stdexcept>

namespace se::render {

RenderManager::RenderManager() { setupGlState(); }

void RenderManager::beginFrame(const se::scene::Camera& camera) {
    m_Camera = &camera;
    m_Frustum = calculateFrustum(m_Camera->getViewProjection());
    clear();
}

void RenderManager::submit(const se::scene::Renderable& renderable) {
    if (!m_Camera) {
        throw std::runtime_error("RenderManager: submit called before beginFrame!");
    }

    m_ModelRenderer.submit(renderable, m_Frustum);
}

void RenderManager::endFrame(const se::scene::LightData& lights) {
    if (!m_Camera) {
        throw std::runtime_error("RenderManager: endFrame called before beginFrame!");
    }

    m_Stats.reset();
    m_ModelRenderer.flush(lights, *m_Camera, m_Stats);

    m_Camera = nullptr;  // invalidate for next frame
}

void RenderManager::toggleWireframe() {
    m_Wireframe = !m_Wireframe;
    m_ModelRenderer.setWireframe(m_Wireframe);
}

void RenderManager::setBatchSize(const size_t maxInstances) { m_ModelRenderer.setBatchSize(maxInstances); }

void RenderManager::reset() {
    m_Stats.reset();
    m_Camera = nullptr;
}

void RenderManager::clear() {
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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