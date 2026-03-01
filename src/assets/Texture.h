#pragma once
#include <string>
#include <cstdint>

#include "Asset.h"

namespace se::assets {

class Texture : public Asset {
   public:
    // For textures loaded from files with stbi, we default to flipping vertically since OpenGL's texture coordinate system has (0,0) at the bottom left
    Texture(const std::string& path, bool flipVertically = true);
    // For textures created from memory GLB embedded images, we assume they are already in the correct orientation since they are not subject to the same coordinate system mismatch
    Texture(const uint8_t* data, int width, int height, int channels);
    ~Texture();
    void bind(unsigned int slot = 0) const;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;

    const std::string& getPath() const override { return m_Path; }

   private:
    unsigned int m_ID;
};

}  // namespace se::assets
