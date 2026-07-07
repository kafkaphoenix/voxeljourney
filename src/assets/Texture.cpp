#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <format>
#include <stdexcept>
#include <vector>

#include "TextureUtils.h"

namespace se::assets {

namespace {

void uploadToGPU(GLuint id, const uint8_t* data, int width, int height, int channels) {
    const auto [internalFormat, format] = channelsToGLFormat(channels);

    if (channels == 3) {
        // Fix pixel alignment for RGB textures whose rows aren't 4-byte aligned.
        // https://stackoverflow.com/questions/71284184/opengl-distorted-texture
        glPixelStorei(GL_UNPACK_ALIGNMENT, (3 * width % 4 == 0) ? 4 : 1);
    }

    int mipLevels = calcMipLevels(width, height);
    glTextureStorage2D(id, mipLevels, internalFormat, width, height);
    glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
    glGenerateTextureMipmap(id);

    if (channels == 3) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);  // reset to default
    }
}
}

Texture::Texture(std::string path, bool flipVertically) : Asset(std::move(path)) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(m_Name.c_str(), &width, &height, &channels, 0);
    if (!data) {
        throw std::runtime_error(std::format("Failed to load texture: {}", m_Name));
    }

    if (width <= 0 || height <= 0 || channels <= 0) {
        stbi_image_free(data);
        throw std::runtime_error(std::format("Invalid texture dimensions: {}x{}x{}", width, height, channels));
    }

    if (flipVertically) {
        flipImageVerticallyInPlace(data, width, height, channels);
    }

    m_Width = width;
    m_Height = height;

    glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
    uploadToGPU(m_Id, data, width, height, channels);
    stbi_image_free(data);

    std::string label = std::format("Texture [{}]", m_Name);
    glObjectLabel(GL_TEXTURE, m_Id, static_cast<GLsizei>(label.size()), label.c_str());
}

Texture::Texture(std::span<const uint8_t> data, int width, int height, int channels) : Asset("<memory>") {
    if (width <= 0 || height <= 0 || channels <= 0) {
        throw std::runtime_error(std::format("Invalid texture dimensions: {}x{}x{}", width, height, channels));
    }

    const size_t rowSize = static_cast<size_t>(width) * static_cast<size_t>(channels);
    const size_t expectedSize = rowSize * static_cast<size_t>(height);
    if (data.size() < expectedSize) {
        throw std::runtime_error(std::format("Texture data too small: expected {}, got {}", expectedSize, data.size()));
    }

    // Flip image vertically (GLB embedded images are stored top-left, OpenGL expects bottom-left).
    std::vector<uint8_t> flipped(data.begin(), std::next(data.begin(), static_cast<std::ptrdiff_t>(expectedSize)));
    flipImageVerticallyInPlace(flipped.data(), width, height, channels);

    m_Width = width;
    m_Height = height;

    glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
    uploadToGPU(m_Id, flipped.data(), width, height, channels);

    std::string label = std::format("Texture [{}]", m_Name);  // m_Name is "<memory>"
    glObjectLabel(GL_TEXTURE, m_Id, static_cast<GLsizei>(label.size()), label.c_str());
}

Texture::~Texture() { glDeleteTextures(1, &m_Id); }

void Texture::bind(unsigned int slot) const { glBindTextureUnit(slot, m_Id); }

}  // namespace se::assets