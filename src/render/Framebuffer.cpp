#include "Framebuffer.h"

#include <format>
#include <stdexcept>

namespace se::render {

Framebuffer::Framebuffer(FramebufferSpec spec) : m_Spec(std::move(spec)) {
    if (m_Spec.width <= 0 || m_Spec.height <= 0) {
        throw std::runtime_error(std::format("Invalid framebuffer dimensions: {}x{}", m_Spec.width, m_Spec.height));
    }
    create();
}

Framebuffer::~Framebuffer() { release(); }

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_Spec(std::move(other.m_Spec)),
      m_Id(other.m_Id),
      m_ColorAttachments(std::move(other.m_ColorAttachments)),
      m_DepthStencilId(other.m_DepthStencilId) {
    other.m_Id = 0;
    other.m_DepthStencilId = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        release();
        m_Spec = std::move(other.m_Spec);
        m_Id = other.m_Id;
        m_ColorAttachments = std::move(other.m_ColorAttachments);
        m_DepthStencilId = other.m_DepthStencilId;
        other.m_Id = 0;
        other.m_DepthStencilId = 0;
    }
    return *this;
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
    glViewport(0, 0, m_Spec.width, m_Spec.height);
}

void Framebuffer::bindDefault() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void Framebuffer::resize(int width, int height) {
    if (width <= 0 || height <= 0)
        return;
    m_Spec.width = width;
    m_Spec.height = height;
    release();
    create();
}

void Framebuffer::clear(float r, float g, float b, float a) const {
    const GLfloat clearColor[] = {r, g, b, a};
    for (size_t i = 0; i < m_ColorAttachments.size(); ++i) {
        glClearNamedFramebufferfv(m_Id, GL_COLOR, static_cast<GLint>(i), clearColor);
    }
    if (m_DepthStencilId != 0) {
        const GLfloat depthValue = 1.0f;
        glClearNamedFramebufferfv(m_Id, GL_DEPTH, 0, &depthValue);
    }
}

void Framebuffer::bindColorTexture(unsigned int slot, size_t attachmentIndex) const {
    if (attachmentIndex >= m_ColorAttachments.size()) {
        throw std::runtime_error(
            std::format("Color attachment index {} out of range ({})", attachmentIndex, m_ColorAttachments.size()));
    }
    glBindTextureUnit(slot, m_ColorAttachments[attachmentIndex]);
}

void Framebuffer::bindDepthTexture(unsigned int slot) const {
    if (!m_Spec.depthTexture) {
        throw std::runtime_error("Depth attachment is a renderbuffer, not a texture");
    }
    glBindTextureUnit(slot, m_DepthStencilId);
}

unsigned int Framebuffer::colorAttachment(size_t index) const {
    if (index >= m_ColorAttachments.size()) {
        throw std::runtime_error(
            std::format("Color attachment index {} out of range ({})", index, m_ColorAttachments.size()));
    }
    return m_ColorAttachments[index];
}

void Framebuffer::create() {
    glCreateFramebuffers(1, &m_Id);
    std::string label = std::format("Framebuffer [{}]", reinterpret_cast<uintptr_t>(this));
    glObjectLabel(GL_FRAMEBUFFER, m_Id, static_cast<GLsizei>(label.size()), label.c_str());

    // Color attachments (textures)
    m_ColorAttachments.resize(m_Spec.colorAttachments.size());
    if (!m_ColorAttachments.empty()) {
        glCreateTextures(GL_TEXTURE_2D, static_cast<GLsizei>(m_ColorAttachments.size()), m_ColorAttachments.data());
    }

    for (size_t i = 0; i < m_ColorAttachments.size(); ++i) {
        unsigned int texId = m_ColorAttachments[i];

        glTextureStorage2D(texId, 1, m_Spec.colorAttachments[i], m_Spec.width, m_Spec.height);
        glNamedFramebufferTexture(m_Id, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), texId, 0);

        std::string texLabel = std::format("FBO Color {} [{}]", i, reinterpret_cast<uintptr_t>(this));
        glObjectLabel(GL_TEXTURE, texId, static_cast<GLsizei>(texLabel.size()), texLabel.c_str());
    }

    // Set draw buffers
    if (!m_ColorAttachments.empty()) {
        std::vector<GLenum> drawBuffers(m_ColorAttachments.size());
        for (size_t i = 0; i < drawBuffers.size(); ++i) {
            drawBuffers[i] = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
        }
        glNamedFramebufferDrawBuffers(m_Id, static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
    } else {
        // Depth-only framebuffer (e.g. shadow map)
        glNamedFramebufferDrawBuffer(m_Id, GL_NONE);
        glNamedFramebufferReadBuffer(m_Id, GL_NONE);
    }

    // Depth/stencil attachment
    if (m_Spec.depthTexture) {
        glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthStencilId);
        glTextureStorage2D(m_DepthStencilId, 1, GL_DEPTH24_STENCIL8, m_Spec.width, m_Spec.height);
        glNamedFramebufferTexture(m_Id, GL_DEPTH_STENCIL_ATTACHMENT, m_DepthStencilId, 0);

        std::string depthLabel = std::format("FBO Depth Tex [{}]", reinterpret_cast<uintptr_t>(this));
        glObjectLabel(GL_TEXTURE, m_DepthStencilId, static_cast<GLsizei>(depthLabel.size()), depthLabel.c_str());
    } else if (m_Spec.depthStencil) {
        glCreateRenderbuffers(1, &m_DepthStencilId);
        glNamedRenderbufferStorage(m_DepthStencilId, GL_DEPTH24_STENCIL8, m_Spec.width, m_Spec.height);
        glNamedFramebufferRenderbuffer(m_Id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthStencilId);

        std::string rboLabel = std::format("FBO Depth RBO [{}]", reinterpret_cast<uintptr_t>(this));
        glObjectLabel(GL_RENDERBUFFER, m_DepthStencilId, static_cast<GLsizei>(rboLabel.size()), rboLabel.c_str());
    }

    // Validate
    GLenum status = glCheckNamedFramebufferStatus(m_Id, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error(std::format("Framebuffer incomplete: 0x{:X}", status));
    }
}

void Framebuffer::release() {
    if (!m_ColorAttachments.empty()) {
        glDeleteTextures(static_cast<GLsizei>(m_ColorAttachments.size()), m_ColorAttachments.data());
        m_ColorAttachments.clear();
    }

    if (m_DepthStencilId != 0) {
        if (m_Spec.depthTexture) {
            glDeleteTextures(1, &m_DepthStencilId);
        } else if (m_Spec.depthStencil) {
            glDeleteRenderbuffers(1, &m_DepthStencilId);
        }
        m_DepthStencilId = 0;
    }

    if (m_Id != 0) {
        glDeleteFramebuffers(1, &m_Id);
        m_Id = 0;
    }
}

}  // namespace se::render
