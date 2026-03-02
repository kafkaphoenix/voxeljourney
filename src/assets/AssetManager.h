#pragma once
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>

#include "Asset.h"
#include "AssetHandle.h"
#include "Material.h"
#include "Model.h"
#include "Shader.h"
#include "StringHash.h"
#include "Texture.h"
#include "UUID.h"

namespace se::assets {

class AssetManager {
   public:
    AssetManager() = default;
    ~AssetManager() = default;

    ShaderHandle getOrLoadShader(std::string_view shaderPath) {
        return getOrLoadAsset<Shader>(
            std::format("shader_{}", shaderPath), std::string(shaderPath));
    }
    ModelHandle getOrLoadModel(std::string_view gltfPath, std::string_view shaderPath) {
        return getOrLoadAsset<Model>(
            std::format("model_{}", gltfPath), std::string(gltfPath), std::string(shaderPath), *this);
    }
    TextureHandle getOrLoadTexture(std::string_view path) {
        return getOrLoadAsset<Texture>(std::format("texture_{}", path), std::string(path));
    }
    TextureHandle getOrLoadTextureFromMemory(std::span<const uint8_t> data, int width, int height, int channels) {
        std::string key = std::format("texture_<memory>_{}:{}",
                                      reinterpret_cast<uintptr_t>(data.data()),
                                      data.size());
        auto it = m_PathToId.find(key);
        if (it != m_PathToId.end()) {
            return TextureHandle(this, it->second);
        }

        UUID id = UUID();
        auto tex = std::make_shared<Texture>(data, width, height, channels);
        m_Assets[id] = tex;
        m_PathToId[key] = id;
        return TextureHandle(this, id);
    }
    MaterialHandle getOrLoadMaterial(std::string_view name,
                                     ShaderHandle shader,
                                     const MaterialTextures& textures,
                                     const MaterialParams& params,
                                     const RenderState& state) {
        return getOrLoadAsset<Material>(
            std::format("material_{}", name), std::string(name), shader, textures, params, state);
    }

    void removeShader(std::string_view shaderPath) {
        removeAssetByPath(std::format("shader_{}", shaderPath));
    }
    void removeModel(std::string_view gltfPath) {
        removeAssetByPath(std::format("model_{}", gltfPath));
    }
    void removeTexture(std::string_view path) {
        removeAssetByPath(std::format("texture_{}", path));
    }
    void removeMaterial(std::string_view name) {
        removeAssetByPath(std::format("material_{}", name));
    }

    ShaderHandle getShader(UUID id) const { return getAssetById<Shader>(id); }
    ModelHandle getModel(UUID id) const { return getAssetById<Model>(id); }
    TextureHandle getTexture(UUID id) const { return getAssetById<Texture>(id); }
    MaterialHandle getMaterial(UUID id) const { return getAssetById<Material>(id); }

    void clear() {
        m_Assets.clear();
        m_PathToId.clear();
    }

   private:
    template <typename T, typename... Args>
    AssetHandle<T> getOrLoadAsset(std::string_view path, Args&&... args) {
        auto it = m_PathToId.find(path);
        if (it != m_PathToId.end()) {
            return AssetHandle<T>(this, it->second);
        }
        UUID id = UUID();
        auto asset = std::make_shared<T>(std::forward<Args>(args)...);
        m_Assets[id] = asset;
        m_PathToId[std::string(path)] = id;
        return AssetHandle<T>(this, id);
    }

    void removeAssetByPath(std::string_view path) {
        auto it = m_PathToId.find(path);
        if (it != m_PathToId.end()) {
            m_Assets.erase(it->second);
            m_PathToId.erase(it);
        }
    }

    template <typename T>
    AssetHandle<T> getAssetById(UUID id) const {
        auto it = m_Assets.find(id);
        if (it != m_Assets.end()) {
            return AssetHandle<T>(const_cast<AssetManager*>(this), id);
        }
        return AssetHandle<T>();  // invalid
    }

    template <typename T>
    std::shared_ptr<T> getAssetPtr(UUID id) const {
        auto it = m_Assets.find(id);
        if (it != m_Assets.end()) {
            return std::dynamic_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    // No multithreading support, so no need for mutexes. If you add multithreading, you'll need to add mutexes to protect these maps.
    std::unordered_map<UUID, std::shared_ptr<Asset>> m_Assets;
    std::unordered_map<std::string, UUID, TransparentStringHash, std::equal_to<>> m_PathToId;

    template <typename T>
    friend class AssetHandle;
};

}  // namespace se::assets