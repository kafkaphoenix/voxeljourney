#pragma once
#include <cstdint>
#include <span>
#include <string>

#include "Asset.h"

namespace se::assets {

class Texture : public Asset {
public:
    // For textures loaded from files with stbi, we default to flipping vertically since OpenGL's texture coordinate
    // system has (0,0) at the bottom left
    explicit Texture(std::string path, bool flipVertically = true);
    // For textures created from memory (e.g. embedded GLTF images), we flip vertically by default since GLB
    // embedded images are stored top-left span<const uint8_t> as we don't want to take ownership of the data
    explicit Texture(std::span<const uint8_t> data, int width, int height, int channels);
    ~Texture() override;
    void bind(unsigned int slot = 0) const;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;

    [[nodiscard]] unsigned int id() const { return m_Id; }
    [[nodiscard]] int width() const { return m_Width; }
    [[nodiscard]] int height() const { return m_Height; }

private:
    unsigned int m_Id = 0;
    int m_Width = 0;
    int m_Height = 0;
};

}  // namespace se::assets
