#include "PostProcessRenderer.h"

namespace se::render {

PostProcessRenderer::PostProcessRenderer(const se::core::config::PostProcess& pp)
    : m_Shader("assets/shaders/postprocess"), m_Exposure(pp.exposure) {
    setupSampler();
}

PostProcessRenderer::~PostProcessRenderer() {
    if (m_Sampler) {
        glDeleteSamplers(1, &m_Sampler);
    }
}

void PostProcessRenderer::execute(const Framebuffer& source) {
    Framebuffer::bindDefault();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_Shader.bind();
    m_Shader.setFloat("u_Exposure", m_Exposure);
    m_Shader.setInt("u_ScreenTexture", 0);
    m_Shader.setInt("u_OITAccumTexture", 1);
    m_Shader.setInt("u_OITRevealTexture", 2);

    source.bindColorTexture(0);
    source.bindColorTexture(1, 1);
    source.bindColorTexture(2, 2);
    glBindSampler(0, m_Sampler);
    glBindSampler(1, m_Sampler);
    glBindSampler(2, m_Sampler);

    m_ScreenQuad.draw();

    glBindSampler(0, 0);
    glBindSampler(1, 0);
    glBindSampler(2, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}

void PostProcessRenderer::setupSampler() {
    glCreateSamplers(1, &m_Sampler);
    glSamplerParameteri(m_Sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(m_Sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(m_Sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(m_Sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glObjectLabel(GL_SAMPLER, m_Sampler, -1, "PostProcessSampler");
}

}  // namespace se::render
