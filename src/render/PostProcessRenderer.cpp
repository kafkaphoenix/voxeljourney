#include "PostProcessRenderer.h"

namespace se::render {

PostProcessRenderer::PostProcessRenderer() : m_Shader("assets/shaders/postprocess") { setupSampler(); }

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
    m_Shader.setInt("u_Effect", static_cast<int>(m_Effect));
    float texelSize[2] = {1.0f / static_cast<float>(source.width()), 1.0f / static_cast<float>(source.height())};
    m_Shader.setVec2("u_TexelSize", texelSize);

    source.bindColorTexture(0);
    glBindSampler(0, m_Sampler);

    m_ScreenQuad.draw();

    glBindSampler(0, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}

void PostProcessRenderer::cycleEffect() {
    auto next = static_cast<uint8_t>(m_Effect) + 1;
    if (next >= static_cast<uint8_t>(PostEffect::COUNT)) {
        next = 0;
    }
    m_Effect = static_cast<PostEffect>(next);
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
