#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "Asset.h"

namespace se::assets {

class TextureArray : public Asset {
public:
    // It takes raw pixel data, how to get that data is up to the caller
    TextureArray(std::string name, std::span<const uint8_t> data, int width, int height, int layers, int channels);
    ~TextureArray() override;

    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;
    TextureArray(TextureArray&&) = delete;
    TextureArray& operator=(TextureArray&&) = delete;

    void bind(unsigned int slot = 0) const;

    [[nodiscard]] unsigned int id() const { return m_Id; }
    [[nodiscard]] int width() const { return m_Width; }
    [[nodiscard]] int height() const { return m_Height; }
    // Number of textures stacked in the array
    [[nodiscard]] int layers() const { return m_Layers; }

private:
    unsigned int m_Id = 0;
    int m_Width = 0;
    int m_Height = 0;
    int m_Layers = 0;
};

}  // namespace se::assets
