#pragma once

#include <glad/glad.h>

#include <cstdint>
#include <optional>

#include "Framebuffer.h"
#include "ScreenQuad.h"
#include "assets/Shader.h"
#include "core/Config.h"

namespace se::render {

class PostProcessRenderer {
public:
    explicit PostProcessRenderer(const se::core::config::PostProcess& pp);
    ~PostProcessRenderer();

    PostProcessRenderer(const PostProcessRenderer&) = delete;
    PostProcessRenderer& operator=(const PostProcessRenderer&) = delete;
    PostProcessRenderer(PostProcessRenderer&&) = delete;
    PostProcessRenderer& operator=(PostProcessRenderer&&) = delete;

    void execute(const Framebuffer& source);

private:
    void setupSampler();

    ScreenQuad m_ScreenQuad;
    se::assets::Shader m_Shader;
    GLuint m_Sampler = 0;
    float m_Exposure = 1.0f;
};

}  // namespace se::render
