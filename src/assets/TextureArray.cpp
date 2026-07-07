#include "TextureArray.h"

#include <glad/glad.h>

#include <format>
#include <stdexcept>

#include "TextureUtils.h"

namespace se::assets {

TextureArray::TextureArray(std::string name, std::span<const uint8_t> data, int width, int height, int layers,
                           int channels)
    : Asset(std::move(name)), m_Width(width), m_Height(height), m_Layers(layers) {
    if (width <= 0 || height <= 0 || layers <= 0 || channels <= 0) {
        throw std::runtime_error(
            std::format("Invalid texture array dimensions: {}x{}x{}x{}", width, height, layers, channels));
    }

    const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(layers) *
                                static_cast<size_t>(channels);
    if (data.size() < expectedSize) {
        throw std::runtime_error(
            std::format("Texture array data too small: expected {}, got {}", expectedSize, data.size()));
    }

    const auto [internalFormat, format] = channelsToGLFormat(channels);

    if (channels == 3) {
        // Fix pixel alignment for RGB textures whose rows aren't 4-byte aligned.
        // https://stackoverflow.com/questions/71284184/opengl-distorted-texture
        glPixelStorei(GL_UNPACK_ALIGNMENT, (3 * width % 4 == 0) ? 4 : 1);
    }

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_Id);
    const int mipLevels = calcMipLevels(width, height);
    glTextureStorage3D(m_Id, mipLevels, internalFormat, width, height, layers);
    glTextureSubImage3D(m_Id, 0, 0, 0, 0, width, height, layers, format, GL_UNSIGNED_BYTE, data.data());
    glGenerateTextureMipmap(m_Id);

    if (channels == 3) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);  // reset to default
    }

    std::string label = std::format("TextureArray [{}]", m_Name);
    glObjectLabel(GL_TEXTURE, m_Id, static_cast<GLsizei>(label.size()), label.c_str());
}

TextureArray::~TextureArray() { glDeleteTextures(1, &m_Id); }

void TextureArray::bind(unsigned int slot) const { glBindTextureUnit(slot, m_Id); }

}  // namespace se::assets
