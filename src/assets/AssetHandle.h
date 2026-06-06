#pragma once

#include "UUID.h"

namespace se::assets {

class AssetManager;
class Asset;
class Model;
class Shader;
class Texture;
class Material;

template <typename T>
class AssetHandle {
public:
    AssetHandle() = default;
    AssetHandle(AssetManager* manager, UUID id) : m_AssetManager(manager), m_Id(id) {}

    [[nodiscard]] T* get() const;

    [[nodiscard]] bool isValid() const { return m_AssetManager != nullptr && m_Id.value() != 0; }
    [[nodiscard]] UUID id() const { return m_Id; }

    bool operator==(const AssetHandle<T>& other) const {
        return m_AssetManager == other.m_AssetManager && m_Id == other.m_Id;
    }
    bool operator!=(const AssetHandle<T>& other) const { return !(*this == other); }

private:
    AssetManager* m_AssetManager{nullptr};
    UUID m_Id{0};
};

using ModelHandle = AssetHandle<Model>;
using ShaderHandle = AssetHandle<Shader>;
using TextureHandle = AssetHandle<Texture>;
using MaterialHandle = AssetHandle<Material>;

}  // namespace se::assets

namespace std {
template <typename T>
struct hash<se::assets::AssetHandle<T>> {
    std::size_t operator()(const se::assets::AssetHandle<T>& handle) const {
        return std::hash<uint64_t>{}(static_cast<uint64_t>(handle.id().value()));
    }
};
}