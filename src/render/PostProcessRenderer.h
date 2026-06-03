#pragma once
#include <glad/glad.h>

#include <cstdint>
#include <optional>

#include "Framebuffer.h"
#include "ScreenQuad.h"
#include "assets/Shader.h"
#include "core/Config.h"

namespace se::render {

enum class PostEffect : uint8_t {
    None = 0,
    ToneMap,
    Inversion,
    Grayscale,
    Sharpen,
    Blur,
    EdgeDetect,
    COUNT  // keep last for iterating over effects
};

class PostProcessRenderer {
public:
    explicit PostProcessRenderer(const se::core::config::PostProcess& pp);
    ~PostProcessRenderer();

    PostProcessRenderer(const PostProcessRenderer&) = delete;
    PostProcessRenderer& operator=(const PostProcessRenderer&) = delete;
    PostProcessRenderer(PostProcessRenderer&&) = delete;
    PostProcessRenderer& operator=(PostProcessRenderer&&) = delete;

    void execute(const Framebuffer& source);

    void cycleEffect();
    void setEffect(PostEffect effect) { m_Effect = effect; }
    [[nodiscard]] PostEffect getEffect() const noexcept { return m_Effect; }

private:
    void setupSampler();

    ScreenQuad m_ScreenQuad;
    se::assets::Shader m_Shader;
    GLuint m_Sampler = 0;
    PostEffect m_Effect = PostEffect::None;
    float m_Exposure = 1.0f;
};

}  // namespace se::render
