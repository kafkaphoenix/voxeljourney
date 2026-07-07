#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace se::assets {

struct GLTextureFormat {
    GLenum internalFormat;
    GLenum format;
};

inline GLTextureFormat channelsToGLFormat(int channels) {
    switch (channels) {
    case 4: return {GL_RGBA8, GL_RGBA};
    case 3: return {GL_RGB8, GL_RGB};
    case 2: return {GL_RG8, GL_RG};
    case 1: return {GL_R8, GL_RED};
    default: throw std::runtime_error(std::format("Unsupported texture format ({} channels)", channels));
    }
}

// Compute mip levels so minified textures sample smaller images, reducing moire/aliasing.
inline int calcMipLevels(int width, int height) {
    int size = std::max(width, height);
    return 1 + static_cast<int>(std::floor(std::log2(size)));
}

// Avoid using stb global flip state; it can leak into tinygltf image decoding.
inline void flipImageVerticallyInPlace(unsigned char* data, int width, int height, int channels) {
    if (!data || width <= 0 || height <= 1 || channels <= 0) {
        return;
    }

    const size_t rowSize = static_cast<size_t>(width) * static_cast<size_t>(channels);
    std::vector<unsigned char> temp(rowSize);

    for (int y = 0; y < height / 2; ++y) {
        unsigned char* rowTop = data + static_cast<size_t>(y) * rowSize;
        unsigned char* rowBottom = data + static_cast<size_t>(height - 1 - y) * rowSize;
        std::memcpy(temp.data(), rowTop, rowSize);
        std::memcpy(rowTop, rowBottom, rowSize);
        std::memcpy(rowBottom, temp.data(), rowSize);
    }
}

}  // namespace se::assets