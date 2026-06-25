#pragma once

#include <glad/glad.h>

#include <array>
#include <vector>

namespace se::render {

struct FramebufferSpec {
    int width = 0;
    int height = 0;
    int samples = 1;  // 1 = single-sample, >1 = MSAA
    std::vector<GLenum> colorAttachments = {GL_RGBA8};
    bool depthStencil = true;   // depth24/stencil8 renderbuffer
    bool depthTexture = false;  // use a depth texture instead of renderbuffer (for shadow maps)
};

class Framebuffer {
public:
    explicit Framebuffer(FramebufferSpec spec);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    void bind() const;
    static void bindDefault();

    void resize(int width, int height);
    void clear(const std::array<float, 4>& color = {0.2f, 0.3f, 0.8f, 1.0f}) const;

    void bindColorTexture(unsigned int slot, size_t attachmentIndex = 0) const;
    void bindDepthTexture(unsigned int slot) const;

    [[nodiscard]] unsigned int id() const { return m_Id; }
    [[nodiscard]] unsigned int colorAttachment(size_t index = 0) const;
    [[nodiscard]] unsigned int depthAttachment() const { return m_DepthStencilId; }
    [[nodiscard]] int width() const { return m_Spec.width; }
    [[nodiscard]] int height() const { return m_Spec.height; }
    [[nodiscard]] bool isMultisampled() const { return m_Spec.samples > 1; }
    [[nodiscard]] const FramebufferSpec& spec() const { return m_Spec; }

private:
    void create();
    void release();

    FramebufferSpec m_Spec;
    unsigned int m_Id = 0;
    std::vector<unsigned int> m_ColorAttachments;
    unsigned int m_DepthStencilId = 0;
};

}  // namespace se::render
