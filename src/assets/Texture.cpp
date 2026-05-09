#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>
#include <vector>

namespace se::assets {

namespace {
// Compute mip levels so minified textures sample smaller images, reducing moire/aliasing.
int calcMipLevels(int width, int height) {
    int size = std::max(width, height);
    return 1 + static_cast<int>(std::floor(std::log2(size)));
}
}

Texture::Texture(std::string path, bool flipVertically) : Asset(std::move(path)) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(static_cast<int>(flipVertically));
    unsigned char* data = stbi_load(m_Name.c_str(), &width, &height, &channels, 0);
    if (data == nullptr) {
        throw std::runtime_error(std::format("Failed to load texture: {}", m_Name));
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);

    GLenum internalFormat = 0;
    GLenum format = 0;
    if (channels == 4) {
        internalFormat = GL_RGBA8;
        format = GL_RGBA;
    } else if (channels == 3) {
        internalFormat = GL_RGB8;
        format = GL_RGB;
        // Fix pixel alignment for RGB textures
        // https://stackoverflow.com/questions/71284184/opengl-distorted-texture
        glPixelStorei(GL_UNPACK_ALIGNMENT, (3 * width % 4 == 0) ? 4 : 1);
    } else if (channels == 2) {
        internalFormat = GL_RG8;
        format = GL_RG;
    } else if (channels == 1) {
        internalFormat = GL_R8;
        format = GL_RED;
    } else {
        stbi_image_free(data);
        throw std::runtime_error(std::format("Unsupported texture format: {} ({} channels)", m_Name, channels));
    }

    m_Width = width;
    m_Height = height;

    int mipLevels = calcMipLevels(width, height);
    glTextureStorage2D(m_Id, mipLevels, internalFormat, width, height);
    glTextureSubImage2D(m_Id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
    glGenerateTextureMipmap(m_Id);

    // Reset alignment to default
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    stbi_image_free(data);
    std::string texLabel = std::format("Texture [{}]", m_Name);
    glObjectLabel(GL_TEXTURE, m_Id, static_cast<GLsizei>(texLabel.size()), texLabel.c_str());
}

Texture::Texture(std::span<const uint8_t> data, int width, int height, int channels) : Asset("<memory>") {
    if (width <= 0 || height <= 0 || channels <= 0) {
        throw std::runtime_error(std::format("Invalid texture dimensions: {}x{}x{}", width, height, channels));
    }
    size_t rowSize = static_cast<size_t>(width) * static_cast<size_t>(channels);
    size_t expectedSize = rowSize * static_cast<size_t>(height);
    if (data.size() < expectedSize) {
        throw std::runtime_error(std::format("Texture data too small: expected {}, got {}", expectedSize, data.size()));
    }
    // Flip image vertically (GLB embedded images are stored top-left, OpenGL expects bottom-left)
    std::vector<uint8_t> flipped(data.begin(), std::next(data.begin(), static_cast<std::ptrdiff_t>(expectedSize)));
    for (int y = 0; y < height / 2; ++y) {
        uint8_t* row1 = &flipped[y * rowSize];
        uint8_t* row2 = &flipped[(height - 1 - y) * rowSize];
        for (size_t x = 0; x < rowSize; ++x) { std::swap(row1[x], row2[x]); }
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);

    GLenum internalFormat = 0;
    GLenum format = 0;
    if (channels == 4) {
        internalFormat = GL_RGBA8;
        format = GL_RGBA;
    } else if (channels == 3) {
        internalFormat = GL_RGB8;
        format = GL_RGB;
        // Fix pixel alignment for RGB textures
        if (3 * width % 4 == 0) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        } else {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }
    } else if (channels == 2) {
        internalFormat = GL_RG8;
        format = GL_RG;
    } else if (channels == 1) {
        internalFormat = GL_R8;
        format = GL_RED;
    } else {
        throw std::runtime_error("Unsupported texture format from memory");
    }

    m_Width = width;
    m_Height = height;

    int mipLevels = calcMipLevels(width, height);
    glTextureStorage2D(m_Id, mipLevels, internalFormat, width, height);
    glTextureSubImage2D(m_Id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, flipped.data());
    glGenerateTextureMipmap(m_Id);

    // Reset alignment to default
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

Texture::~Texture() { glDeleteTextures(1, &m_Id); }

void Texture::bind(unsigned int slot) const { glBindTextureUnit(slot, m_Id); }

}  // namespace se::assets
