#include "Model.h"

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include <cstdint>
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <stdexcept>

#include "AssetManager.h"

namespace se::assets {

namespace {

tinygltf::Model loadGltfModel(const std::string& gltfPath) {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool isBinary = gltfPath.size() >= 4 &&
                    gltfPath.substr(gltfPath.size() - 4) == ".glb";

    bool ret = isBinary ? loader.LoadBinaryFromFile(&gltfModel, &err, &warn, gltfPath)
                        : loader.LoadASCIIFromFile(&gltfModel, &err, &warn, gltfPath);

    if (!ret) throw std::runtime_error("Failed to load GLTF: " + err);
    if (!warn.empty()) std::cout << "GLTF Warning: " << warn << std::endl;

    return gltfModel;
}

std::vector<TextureHandle> loadGltfTextures(const tinygltf::Model& gltfModel,
                                            const std::string& gltfDir,
                                            AssetManager& assetManager) {
    std::vector<TextureHandle> gltfTextures;
    gltfTextures.reserve(gltfModel.textures.size());

    for (const auto& texture : gltfModel.textures) {
        if (texture.source < 0 || texture.source >= static_cast<int>(gltfModel.images.size()))
            throw std::runtime_error("Invalid texture source: " + std::to_string(texture.source));

        const auto& image = gltfModel.images[texture.source];
        TextureHandle handle;

        if (!image.uri.empty()) {
            // External file
            std::string path = gltfDir + "/" + image.uri;
            handle = assetManager.getOrLoadTexture(path);
        } else if (!image.image.empty()) {
            // Embedded texture
            handle = assetManager.getOrLoadTextureFromMemory(
                image.image.data(), image.width, image.height, image.component);
        } else {
            throw std::runtime_error("Texture has no URI or embedded image");
        }

        if (!handle.isValid())
            throw std::runtime_error("Failed to load texture: " + (image.uri.empty() ? "embedded" : image.uri));

        gltfTextures.push_back(handle);
    }

    return gltfTextures;
}

MaterialHandle createDefaultMaterial(const std::string& name,
                                     AssetManager& assetManager,
                                     const ShaderHandle& shader) {
    MaterialTextures defaultTextures;
    MaterialParams defaultParams;
    return assetManager.getOrLoadMaterial(name, shader, defaultTextures, defaultParams, RenderState{});
}

std::vector<MaterialHandle> buildMaterials(const tinygltf::Model& gltfModel,
                                           AssetManager& assetManager,
                                           const ShaderHandle& shader,
                                           const std::vector<TextureHandle>& textures) {
    std::vector<MaterialHandle> materials;
    materials.reserve(gltfModel.materials.size());

    for (size_t i = 0; i < gltfModel.materials.size(); ++i) {
        const auto& mat = gltfModel.materials[i];
        MaterialTextures matTextures;
        MaterialParams params;

        auto assignTexture = [&](int texIndex, TextureHandle& dst) {
            if (texIndex >= 0 && texIndex < static_cast<int>(textures.size()))
                dst = textures[texIndex];
        };

        assignTexture(mat.pbrMetallicRoughness.baseColorTexture.index, matTextures.baseColor);
        assignTexture(mat.pbrMetallicRoughness.metallicRoughnessTexture.index, matTextures.metallicRoughness);
        assignTexture(mat.normalTexture.index, matTextures.normal);
        assignTexture(mat.emissiveTexture.index, matTextures.emissive);
        assignTexture(mat.occlusionTexture.index, matTextures.occlusion);

        if (mat.pbrMetallicRoughness.baseColorFactor.size() == 4) {
            params.baseColorFactor = glm::vec4(
                static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[0]),
                static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[1]),
                static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[2]),
                static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[3]));
        }
        params.metallicFactor = static_cast<float>(mat.pbrMetallicRoughness.metallicFactor);
        params.roughnessFactor = static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor);

        if (mat.emissiveFactor.size() == 3) {
            params.emissiveFactor = glm::vec3(
                static_cast<float>(mat.emissiveFactor[0]),
                static_cast<float>(mat.emissiveFactor[1]),
                static_cast<float>(mat.emissiveFactor[2]));
        }

        params.alphaCutoff = (mat.alphaMode == "MASK") ? static_cast<float>(mat.alphaCutoff) : 0.0f;

        RenderState state;
        state.blend = (mat.alphaMode == "BLEND");
        state.depthWrite = !state.blend;
        state.cull = !mat.doubleSided;

        std::string matName = mat.name.empty() ? "material_" + std::to_string(i) : mat.name;
        materials.push_back(assetManager.getOrLoadMaterial(matName, shader, matTextures, params, state));
    }

    return materials;
}

std::vector<unsigned int> readIndices(const tinygltf::Model& gltfModel,
                                      const tinygltf::Primitive& primitive,
                                      size_t vertexCount) {
    std::vector<unsigned int> indices;

    if (primitive.indices < 0) {
        indices.reserve(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i) indices.push_back(static_cast<unsigned int>(i));
        return indices;
    }

    const auto& accessor = gltfModel.accessors[primitive.indices];
    if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(gltfModel.bufferViews.size()))
        throw std::runtime_error("Invalid bufferView for indices");
    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
    if (bufferView.buffer < 0 || bufferView.buffer >= static_cast<int>(gltfModel.buffers.size()))
        throw std::runtime_error("Invalid buffer for indices");
    const auto& buffer = gltfModel.buffers[bufferView.buffer];

    indices.reserve(accessor.count);
    size_t stride = bufferView.byteStride > 0 ? bufferView.byteStride : 0;
    size_t elemSize = 0;
    switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            elemSize = sizeof(uint16_t);
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            elemSize = sizeof(uint32_t);
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            elemSize = sizeof(uint8_t);
            break;
        default:
            throw std::runtime_error("Unsupported index component type");
    }
    stride = stride ? stride : elemSize;
    const size_t baseOffset = bufferView.byteOffset + accessor.byteOffset;
    for (size_t i = 0; i < accessor.count; ++i) {
        size_t offset = baseOffset + i * stride;
        switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                const uint16_t* elem = reinterpret_cast<const uint16_t*>(&buffer.data[offset]);
                indices.push_back(*elem);
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                const uint32_t* elem = reinterpret_cast<const uint32_t*>(&buffer.data[offset]);
                indices.push_back(*elem);
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                const uint8_t* elem = reinterpret_cast<const uint8_t*>(&buffer.data[offset]);
                indices.push_back(*elem);
                break;
            }
        }
    }
    return indices;
}

// Reads a strided float accessor into a flat vector.
// Necessary because GLB exporters (e.g. Blender) often produce interleaved
// vertex buffers with a non-zero byteStride, which a raw pointer cast would misread.
void readStridedVec(const tinygltf::Model& gltfModel, const tinygltf::Accessor& acc, int components, std::vector<float>& out) {
    if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(gltfModel.bufferViews.size()))
        throw std::runtime_error("Invalid bufferView for accessor");
    const auto& bv = gltfModel.bufferViews[acc.bufferView];
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(gltfModel.buffers.size()))
        throw std::runtime_error("Invalid buffer for accessor");
    const auto& buf = gltfModel.buffers[bv.buffer];
    const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
    size_t stride = bv.byteStride > 0 ? bv.byteStride : components * sizeof(float);
    out.reserve(acc.count * components);
    for (size_t i = 0; i < acc.count; ++i) {
        const float* elem = reinterpret_cast<const float*>(base + i * stride);
        for (int c = 0; c < components; ++c)
            out.push_back(elem[c]);
    }
}

std::unique_ptr<se::render::Mesh> buildMeshFromPrimitive(const tinygltf::Model& gltfModel,
                                             const tinygltf::Primitive& primitive) {
    auto posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end()) return nullptr;

    const auto& posAccessor = gltfModel.accessors[posIt->second];
    if (posAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || posAccessor.type != TINYGLTF_TYPE_VEC3)
        return nullptr;
    if (posAccessor.bufferView < 0 || posAccessor.bufferView >= static_cast<int>(gltfModel.bufferViews.size()))
        return nullptr;
    const size_t vertexCount = posAccessor.count;

    std::vector<float> positions;
    readStridedVec(gltfModel, posAccessor, 3, positions);
    std::vector<float> normals;
    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
        const auto& nAccessor = gltfModel.accessors[primitive.attributes.at("NORMAL")];
        readStridedVec(gltfModel, nAccessor, 3, normals);
    }
    std::vector<float> texCoords;
    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
        const auto& tAccessor = gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")];
        readStridedVec(gltfModel, tAccessor, 2, texCoords);
    }

    std::vector<float> vertices;
    vertices.reserve(vertexCount * 8);  // 3 pos + 3 normal + 2 tex

    se::render::AABB aabb;
    if (vertexCount > 0 && positions.size() >= 3) {
        aabb.min = aabb.max = glm::vec3(positions[0], positions[1], positions[2]);
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        // Position
        glm::vec3 pos(0.0f);
        if (i * 3 + 2 < positions.size())
            pos = glm::vec3(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
        vertices.push_back(pos.x);
        vertices.push_back(pos.y);
        vertices.push_back(pos.z);

        if (i > 0) {
            aabb.min = glm::min(aabb.min, pos);
            aabb.max = glm::max(aabb.max, pos);
        }

        // Normal
        if (i * 3 + 2 < normals.size()) {
            vertices.push_back(normals[i * 3]);
            vertices.push_back(normals[i * 3 + 1]);
            vertices.push_back(normals[i * 3 + 2]);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }

        // TexCoord
        if (i * 2 + 1 < texCoords.size()) {
            vertices.push_back(texCoords[i * 2]);
            vertices.push_back(1.0f - texCoords[i * 2 + 1]);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    auto indices = readIndices(gltfModel, primitive, vertexCount);

    return std::make_unique<se::render::Mesh>(vertices.data(), vertices.size() * sizeof(float),
                                  indices.data(), indices.size(), aabb);
}

MaterialHandle resolveMaterial(const tinygltf::Primitive& primitive,
                               const std::vector<MaterialHandle>& materials,
                               const MaterialHandle& fallback) {
    if (primitive.material >= 0 && primitive.material < static_cast<int>(materials.size()))
        return materials[primitive.material];
    return fallback;
}

} // namespace

Model::Model(const std::string& gltfPath, const std::string& shaderPath, AssetManager& assetManager)
    : Asset(gltfPath) {
    try {
        tinygltf::Model gltfModel = loadGltfModel(gltfPath);
        std::string gltfDir = getDirectory(gltfPath);

        auto gltfTextures = loadGltfTextures(gltfModel, gltfDir, assetManager);
        auto shader = assetManager.getOrLoadShader(shaderPath);
        auto defaultMaterial = createDefaultMaterial(m_Path + "#default", assetManager, shader);
        auto gltfMaterials = buildMaterials(gltfModel, assetManager, shader, gltfTextures);

        size_t totalPrimitives = 0;
        for (const auto& mesh : gltfModel.meshes) totalPrimitives += mesh.primitives.size();
        m_SubMeshes.reserve(totalPrimitives);

        for (const auto& mesh : gltfModel.meshes) {
            for (const auto& primitive : mesh.primitives) {
                auto meshPtr = buildMeshFromPrimitive(gltfModel, primitive);
                if (!meshPtr) continue;
                auto mat = resolveMaterial(primitive, gltfMaterials, defaultMaterial);
                m_SubMeshes.push_back({std::move(meshPtr), mat});
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading model '" << gltfPath << "': " << e.what() << std::endl;
        throw;
    }
}

std::string Model::getDirectory(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    return (lastSlash == std::string::npos) ? "." : filepath.substr(0, lastSlash);
}

}  // namespace se::assets