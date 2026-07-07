#pragma once

#include <array>
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
#include <utility>

#include "Asset.h"
#include "AssetHandle.h"
#include "Material.h"
#include "Model.h"
#include "Shader.h"
#include "StringHash.h"
#include "Texture.h"
#include "TextureArray.h"
#include "UUID.h"

namespace se::assets {

class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;

    ShaderHandle getOrLoadShader(std::string_view shaderPath) {
        return getOrLoadAsset<Shader>(std::format("shader_{}", shaderPath), std::string(shaderPath));
    }
    ShaderHandle getOrLoadShader(std::string_view vertPath, std::string_view fragPath) {
        return getOrLoadAsset<Shader>(std::format("shader_{}_{}", vertPath, fragPath), std::string(vertPath),
                                      std::string(fragPath));
    }
    ModelHandle getOrLoadModel(std::string_view gltfPath, ShaderHandle shader) {
        return getOrLoadAsset<Model>(std::format("model_{}", gltfPath), std::string(gltfPath), shader, *this);
    }
    TextureHandle getOrLoadTexture(std::string_view path) {
        return getOrLoadAsset<Texture>(std::format("texture_{}", path), std::string(path));
    }
    TextureHandle getOrLoadTextureFromBinary(std::string_view id, std::span<const uint8_t> data, int width, int height,
                                             int channels) {
        std::string path = std::format("texture_{}", id);
        auto it = m_PathToId.find(path);
        if (it != m_PathToId.end()) {
            return {this, it->second};
        }

        UUID uuid = UUID();
        auto tex = std::make_unique<Texture>(data, width, height, channels);
        m_Assets[uuid] = std::move(tex);
        m_PathToId[path] = uuid;
        return {this, uuid};
    }
    TextureHandle getOrLoadGeneratedTexture(std::string_view name, std::span<const uint8_t> data, int width, int height,
                                            int channels) {
        std::string key = std::format("texture_{}", name);
        auto it = m_PathToId.find(key);
        if (it != m_PathToId.end()) {
            return {this, it->second};
        }

        UUID id = UUID();
        auto tex = std::make_unique<Texture>(data, width, height, channels);
        m_Assets[id] = std::move(tex);
        m_PathToId[key] = id;
        return {this, id};
    }
    TextureArrayHandle getOrLoadGeneratedTextureArray(std::string_view name, std::span<const uint8_t> data, int width,
                                                      int height, int layers, int channels) {
        std::string key = std::format("texture_array_{}", name);
        auto it = m_PathToId.find(key);
        if (it != m_PathToId.end()) {
            return {this, it->second};
        }

        UUID id = UUID();
        auto tex = std::make_unique<TextureArray>(std::string(name), data, width, height, layers, channels);
        m_Assets[id] = std::move(tex);
        m_PathToId[key] = id;
        return {this, id};
    }
    MaterialHandle getOrLoadMaterial(std::string_view name, ShaderHandle shader, const MaterialTextures& textures,
                                     const MaterialParams& params, const RenderState& state) {
        return getOrLoadAsset<Material>(std::format("material_{}", name), std::string(name), shader, textures, params,
                                        state);
    }

    void removeShader(std::string_view shaderPath) { removeAssetByPath(std::format("shader_{}", shaderPath)); }
    void removeModel(std::string_view gltfPath) { removeAssetByPath(std::format("model_{}", gltfPath)); }
    void removeTexture(std::string_view path) { removeAssetByPath(std::format("texture_{}", path)); }
    void removeTextureArray(std::string_view name) { removeAssetByPath(std::format("texture_array_{}", name)); }
    void removeMaterial(std::string_view name) { removeAssetByPath(std::format("material_{}", name)); }

    ShaderHandle getShader(UUID id) { return getAssetById<Shader>(id); }
    ModelHandle getModel(UUID id) { return getAssetById<Model>(id); }
    TextureHandle getTexture(UUID id) { return getAssetById<Texture>(id); }
    TextureArrayHandle getTextureArray(UUID id) { return getAssetById<TextureArray>(id); }
    MaterialHandle getMaterial(UUID id) { return getAssetById<Material>(id); }

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
        auto asset = std::make_unique<T>(std::forward<Args>(args)...);
        m_Assets[id] = std::move(asset);
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
    AssetHandle<T> getAssetById(UUID id) {
        auto it = m_Assets.find(id);
        if (it != m_Assets.end()) {
            return {this, id};
        }
        return {};  // invalid
    }

    template <typename T>
    T* getAssetPtr(UUID id) const {
        auto it = m_Assets.find(id);
        if (it != m_Assets.end()) {
            // Caller must ensure type T matches the actual asset type, otherwise this cast is unsafe.
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    // No multithreading support, so no need for mutexes. If you add multithreading, you'll need to add mutexes to
    // protect these maps.
    std::unordered_map<UUID, std::unique_ptr<Asset>> m_Assets;
    std::unordered_map<std::string, UUID, StringHash, std::equal_to<>> m_PathToId;

    template <typename T>
    friend class AssetHandle;
};

}  // namespace se::assets