#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "Asset.h"
#include "AssetHandle.h"
#include "Material.h"
#include "Model.h"
#include "Shader.h"
#include "Texture.h"
#include "UUID.h"

namespace se::assets {

class AssetManager {
   public:
    AssetManager() = default;
    ~AssetManager() = default;

    ShaderHandle getOrLoadShader(const std::string& shaderPath) {
        return getOrLoadAsset<Shader>("shader_" + shaderPath, shaderPath);
    }
    ModelHandle getOrLoadModel(const std::string& gltfPath, const std::string& shaderPath) {
        return getOrLoadAsset<Model>("model_" + gltfPath, gltfPath, shaderPath, *this);
    }
    TextureHandle getOrLoadTexture(const std::string& path) {
        return getOrLoadAsset<Texture>("texture_" + path, path);
    }
    TextureHandle getOrLoadTextureFromMemory(const uint8_t* data, int width, int height, int channels) {
        std::string key = "texture_<memory>_" + std::to_string(reinterpret_cast<uintptr_t>(data));
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
    MaterialHandle getOrLoadMaterial(const std::string& name,
                                     ShaderHandle shader,
                                     const MaterialTextures& textures,
                                     const MaterialParams& params,
                                     const RenderState& state) {
        return getOrLoadAsset<Material>("material_" + name, name, shader, textures, params, state);
    }

    void removeShader(const std::string& shaderPath) {
        removeAssetByPath("shader_" + shaderPath);
    }
    void removeModel(const std::string& gltfPath) {
        removeAssetByPath("model_" + gltfPath);
    }
    void removeTexture(const std::string& path) {
        removeAssetByPath("texture_" + path);
    }
    void removeMaterial(const std::string& name) {
        removeAssetByPath("material_" + name);
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
    AssetHandle<T> getOrLoadAsset(const std::string& path, Args&&... args) {
        auto it = m_PathToId.find(path);
        if (it != m_PathToId.end()) {
            return AssetHandle<T>(this, it->second);
        }
        UUID id = UUID();
        auto asset = std::make_shared<T>(std::forward<Args>(args)...);
        m_Assets[id] = asset;
        m_PathToId[path] = id;
        return AssetHandle<T>(this, id);
    }

    void removeAssetByPath(const std::string& path) {
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
    std::unordered_map<std::string, UUID> m_PathToId;

    template <typename T>
    friend class AssetHandle;
};

}  // namespace se::assets