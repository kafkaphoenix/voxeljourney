#include "Model.h"

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <print>
#include <span>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "AssetManager.h"

namespace se::assets {

namespace {

void updateAABB(se::render::AABB& aabb, const glm::vec3& pos, size_t i) {
    if (i == 0) {
        aabb.min = aabb.max = pos;
    } else {
        aabb.min = glm::min(aabb.min, pos);
        aabb.max = glm::max(aabb.max, pos);
    }
}

// attribute name -> raw source bytes, one entry per layout element
using VertexSourceMap = std::unordered_map<std::string, std::span<const uint8_t>>;

// Writes vertex data into a raw byte buffer driven entirely by the layout.
// Each element is written at its layout offset; missing attributes are left zeroed.
void packVertexData(const VertexSourceMap& sources, size_t vertexCount, const se::render::BufferLayout& layout,
                    std::vector<uint8_t>& vertices) {
    vertices.resize(vertexCount * layout.getStride(), 0);
    for (size_t i = 0; i < vertexCount; ++i) {
        uint8_t* vptr = vertices.data() + i * layout.getStride();
        for (const auto& elem : layout.getElements()) {
            const size_t elemBytes = static_cast<size_t>(elem.size) * elem.count;
            auto it = sources.find(elem.name);
            if (it == sources.end()) {
                continue;
            }
            const size_t srcOffset = i * elemBytes;
            if (srcOffset + elemBytes <= it->second.size()) {
                std::memcpy(vptr + elem.offset, it->second.data() + srcOffset, elemBytes);
            }
        }
    }
}

// Reads a strided float accessor into a flat vector.
// Necessary because GLB exporters (e.g. Blender) often produce interleaved
// vertex buffers with a non-zero byteStride, which a raw pointer cast would misread.
void readStridedVec(const tinygltf::Model& gltfModel, const tinygltf::Accessor& acc, int components,
                    std::vector<float>& out) {
    if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(gltfModel.bufferViews.size())) {
        throw std::runtime_error("Invalid bufferView for accessor");
    }
    const auto& bv = gltfModel.bufferViews[acc.bufferView];
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(gltfModel.buffers.size())) {
        throw std::runtime_error("Invalid buffer for accessor");
    }
    const auto& buf = gltfModel.buffers[bv.buffer];
    const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
    size_t stride = bv.byteStride > 0 ? bv.byteStride : components * sizeof(float);
    out.reserve(acc.count * components);
    for (size_t i = 0; i < acc.count; ++i) {
        const auto* elem = reinterpret_cast<const float*>(base + i * stride);
        for (int c = 0; c < components; ++c) { out.push_back(elem[c]); }
    }
}

void readPrimitiveAttributes(const tinygltf::Model& gltfModel, const tinygltf::Primitive& primitive,
                             std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texCoords,
                             std::vector<float>& tangents, std::vector<float>& colors) {
    if (auto posIt = primitive.attributes.find("POSITION"); posIt != primitive.attributes.end()) {
        readStridedVec(gltfModel, gltfModel.accessors[posIt->second], 3, positions);
    }

    if (auto normIt = primitive.attributes.find("NORMAL"); normIt != primitive.attributes.end()) {
        readStridedVec(gltfModel, gltfModel.accessors[normIt->second], 3, normals);
    }

    if (auto texIt = primitive.attributes.find("TEXCOORD_0"); texIt != primitive.attributes.end()) {
        readStridedVec(gltfModel, gltfModel.accessors[texIt->second], 2, texCoords);
    }

    if (auto tanIt = primitive.attributes.find("TANGENT"); tanIt != primitive.attributes.end()) {
        readStridedVec(gltfModel, gltfModel.accessors[tanIt->second], 4, tangents);
    }

    if (auto colorIt = primitive.attributes.find("COLOR_0"); colorIt != primitive.attributes.end()) {
        const auto& accessor = gltfModel.accessors[colorIt->second];
        int components = (accessor.type == TINYGLTF_TYPE_VEC4) ? 4 : 3;
        // Only support float for now; unsupported types will be left as white (1.0) in the shader fallback logic
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            readStridedVec(gltfModel, accessor, components, colors);
            // If VEC3, expand to VEC4 with alpha=1
            if (components == 3) {
                std::vector<float> expanded;
                expanded.reserve((colors.size() / 3) * 4);
                for (size_t i = 0; i < colors.size(); i += 3) {
                    expanded.push_back(colors[i]);
                    expanded.push_back(colors[i + 1]);
                    expanded.push_back(colors[i + 2]);
                    expanded.push_back(1.0f);
                }
                colors = std::move(expanded);
            }
        }
    }
}

size_t getIndexElementSize(int componentType) {
    switch (componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return sizeof(uint16_t);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return sizeof(uint32_t);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return sizeof(uint8_t);
    default: throw std::runtime_error("Unsupported index component type");
    }
}

unsigned int extractIndex(const std::vector<unsigned char>& buffer, size_t offset, int componentType) {
    switch (componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return *reinterpret_cast<const uint16_t*>(&buffer[offset]);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return *reinterpret_cast<const uint32_t*>(&buffer[offset]);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return *reinterpret_cast<const uint8_t*>(&buffer[offset]);
    default: throw std::runtime_error("Unsupported index component type");
    }
}

void validateAccessorBuffer(const tinygltf::Accessor& accessor, const tinygltf::Model& gltfModel) {
    if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(gltfModel.bufferViews.size())) {
        throw std::runtime_error("Invalid bufferView for indices");
    }
    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
    if (bufferView.buffer < 0 || bufferView.buffer >= static_cast<int>(gltfModel.buffers.size())) {
        throw std::runtime_error("Invalid buffer for indices");
    }
}

tinygltf::Model loadGltfModel(std::string_view gltfPath) {
    std::string gltfPathStr(gltfPath);
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = gltfPath.ends_with(".glb") ? loader.LoadBinaryFromFile(&gltfModel, &err, &warn, gltfPathStr)
                                          : loader.LoadASCIIFromFile(&gltfModel, &err, &warn, gltfPathStr);

    if (!ret) {
        throw std::runtime_error(std::format("Failed to load GLTF: {}", err));
    }
    if (!warn.empty()) {
        std::println("GLTF Warning: {}", warn);
    }
    return gltfModel;
}

std::vector<TextureHandle> loadGltfTextures(const tinygltf::Model& gltfModel, std::string_view gltfDir,
                                            AssetManager& assetManager, std::string_view gltfPath) {
    std::vector<TextureHandle> gltfTextures;
    gltfTextures.reserve(gltfModel.textures.size());

    for (const auto& texture : gltfModel.textures) {
        TextureHandle handle;

        try {
            if (texture.source < 0 || texture.source >= static_cast<int>(gltfModel.images.size())) {
                throw std::runtime_error(std::format("Invalid texture source: {}", texture.source));
            }

            const auto& image = gltfModel.images[texture.source];

            if (!image.uri.empty()) {
                std::filesystem::path path = (std::filesystem::path(gltfDir) / image.uri).lexically_normal();
                handle = assetManager.getOrLoadTexture(path.string());
            } else if (!image.image.empty()) {
                auto imageBytes =
                    std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(image.image.data()), image.image.size());
                std::string id = std::format("gltf_embedded_{}_{}", gltfPath, texture.source);
                handle =
                    assetManager.getOrLoadTextureFromBinary(id, imageBytes, image.width, image.height, image.component);
            } else {
                throw std::runtime_error("Texture has no URI or embedded image");
            }
        } catch (const std::exception&) {
            // On failure, use an invalid handle and let the material creation logic assign a fallback texture and log a
            // warning
        }

        gltfTextures.push_back(handle);
    }
    return gltfTextures;
}

TextureHandle createCheckerboardTexture(AssetManager& assetManager) {
    static constexpr int size = 64;
    static constexpr int tileSize = 8;
    static constexpr auto pixels = [] {
        std::array<uint8_t, size * size * 4> p{};
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                uint8_t* px = p.data() + (y * size + x) * 4;
                bool on = (((x / tileSize) + (y / tileSize)) % 2 == 0);
                px[0] = on ? 255 : 0;
                px[1] = 0;
                px[2] = on ? 255 : 0;
                px[3] = 255;
            }
        }
        return p;
    }();
    return assetManager.getOrLoadGeneratedTexture(
        "<checkerboard>", std::span<const uint8_t>(pixels.data(), pixels.size()), size, size, 4);
}

MaterialHandle createDefaultMaterial(std::string_view name, AssetManager& assetManager, const ShaderHandle& shader) {
    return assetManager.getOrLoadMaterial(name, shader, MaterialTextures{}, MaterialParams{}, RenderState{});
}

std::vector<MaterialHandle> buildMaterials(const tinygltf::Model& gltfModel, AssetManager& assetManager,
                                           const ShaderHandle& shader, const std::vector<TextureHandle>& textures,
                                           const TextureHandle& fallbackBaseColor) {
    std::vector<MaterialHandle> materials;
    materials.reserve(gltfModel.materials.size());

    for (size_t i = 0; i < gltfModel.materials.size(); ++i) {
        const auto& mat = gltfModel.materials[i];
        MaterialTextures matTextures;
        MaterialParams params;

        auto assignTexture = [&](int texIndex, TextureHandle& dst) {
            if (texIndex >= 0 && texIndex < static_cast<int>(textures.size())) {
                dst = textures[texIndex];
            }
        };

        assignTexture(mat.pbrMetallicRoughness.baseColorTexture.index, matTextures.baseColor);
        assignTexture(mat.pbrMetallicRoughness.metallicRoughnessTexture.index, matTextures.metallicRoughness);
        assignTexture(mat.normalTexture.index, matTextures.normal);
        assignTexture(mat.emissiveTexture.index, matTextures.emissive);
        assignTexture(mat.occlusionTexture.index, matTextures.occlusion);

        std::string matName = mat.name.empty() ? "material_" + std::to_string(i) : mat.name;

        // Warn for each slot that the GLTF references a texture but failed to resolve
        auto warnSlot = [&](int texIndex, const TextureHandle& handle, const char* slot) {
            if (texIndex >= 0 && !handle.isValid()) {
                std::string texPath = "unknown";
                if (texIndex < static_cast<int>(gltfModel.textures.size())) {
                    int src = gltfModel.textures[texIndex].source;
                    if (src >= 0 && src < static_cast<int>(gltfModel.images.size())) {
                        texPath =
                            gltfModel.images[src].uri.empty() ? gltfModel.images[src].name : gltfModel.images[src].uri;
                    }
                }
                std::println("Warning: material '{}' {} texture failed to load: '{}'", matName, slot, texPath);
            }
        };
        warnSlot(mat.pbrMetallicRoughness.baseColorTexture.index, matTextures.baseColor, "base color");
        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0 && !matTextures.baseColor.isValid()) {
            matTextures.baseColor = fallbackBaseColor;
        }
        warnSlot(mat.pbrMetallicRoughness.metallicRoughnessTexture.index, matTextures.metallicRoughness,
                 "metallic-roughness");
        warnSlot(mat.normalTexture.index, matTextures.normal, "normal");
        warnSlot(mat.emissiveTexture.index, matTextures.emissive, "emissive");
        warnSlot(mat.occlusionTexture.index, matTextures.occlusion, "occlusion");

        if (mat.pbrMetallicRoughness.baseColorFactor.size() == 4) {
            params.baseColorFactor = glm::vec4(static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[0]),
                                               static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[1]),
                                               static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[2]),
                                               static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[3]));
        }
        params.metallicFactor = static_cast<float>(mat.pbrMetallicRoughness.metallicFactor);
        params.roughnessFactor = static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor);

        if (mat.emissiveFactor.size() == 3) {
            params.emissiveFactor =
                glm::vec3(static_cast<float>(mat.emissiveFactor[0]), static_cast<float>(mat.emissiveFactor[1]),
                          static_cast<float>(mat.emissiveFactor[2]));
        }

        params.alphaCutoff = (mat.alphaMode == "MASK") ? static_cast<float>(mat.alphaCutoff) : 0.0f;

        RenderState state;
        state.blend = (mat.alphaMode == "BLEND");
        state.depthWrite = !state.blend;
        state.cull = !mat.doubleSided;

        materials.push_back(assetManager.getOrLoadMaterial(matName, shader, matTextures, params, state));
    }
    return materials;
}

std::vector<unsigned int> readIndices(const tinygltf::Model& gltfModel, const tinygltf::Primitive& primitive,
                                      size_t vertexCount) {
    std::vector<unsigned int> indices;

    if (primitive.indices < 0) {
        indices.reserve(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i) { indices.push_back(static_cast<unsigned int>(i)); }
        return indices;
    }

    const auto& accessor = gltfModel.accessors[primitive.indices];
    validateAccessorBuffer(accessor, gltfModel);
    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
    const auto& buffer = gltfModel.buffers[bufferView.buffer];

    indices.reserve(accessor.count);
    size_t elemSize = getIndexElementSize(accessor.componentType);
    size_t stride = bufferView.byteStride > 0 ? bufferView.byteStride : elemSize;
    const size_t baseOffset = bufferView.byteOffset + accessor.byteOffset;

    for (size_t i = 0; i < accessor.count; ++i) {
        indices.push_back(extractIndex(buffer.data, baseOffset + i * stride, accessor.componentType));
    }

    return indices;
}

se::render::BufferLayout buildStaticMeshLayout() {
    return se::render::BufferLayout({
        {"a_Position", GL_FLOAT, sizeof(float), 0, 3, GL_FALSE},
        {"a_Normal", GL_FLOAT, sizeof(float), 0, 3, GL_FALSE},
        {"a_TexCoord", GL_FLOAT, sizeof(float), 0, 2, GL_FALSE},
        {"a_Tangent", GL_FLOAT, sizeof(float), 0, 4, GL_FALSE},
        {"a_Color", GL_FLOAT, sizeof(float), 0, 4, GL_FALSE},
    });
}

std::unique_ptr<se::render::Mesh> buildMeshFromPrimitive(const tinygltf::Model& gltfModel,
                                                         const tinygltf::Primitive& primitive,
                                                         const se::render::BufferLayout& layout, bool instanced) {
    auto posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end()) {
        return nullptr;
    }

    const auto& posAccessor = gltfModel.accessors[posIt->second];
    if (posAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || posAccessor.type != TINYGLTF_TYPE_VEC3 ||
        posAccessor.bufferView < 0 || posAccessor.bufferView >= static_cast<int>(gltfModel.bufferViews.size())) {
        return nullptr;
    }

    const size_t vertexCount = posAccessor.count;

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<float> tangents;
    std::vector<float> colors;
    readPrimitiveAttributes(gltfModel, primitive, positions, normals, texCoords, tangents, colors);

    // Fill defaults before building the source map so packVertexData stays generic.
    // Default normal: up (0,1,0)
    if (normals.empty()) {
        normals.resize(vertexCount * 3, 0.0f);
        for (size_t i = 0; i < vertexCount; ++i) {
            normals[i * 3 + 1] = 1.0f;  // up
        }
    }

    // Default UV: (0,0)
    if (texCoords.empty()) {
        texCoords.resize(vertexCount * 2, 0.0f);
    }

    // Default tangent: +X with w=1 (positive bitangent sign)
    if (tangents.empty()) {
        tangents.resize(vertexCount * 4, 0.0f);
        for (size_t i = 0; i < vertexCount; ++i) {
            tangents[i * 4] = 1.0f;
            tangents[i * 4 + 3] = 1.0f;
        }
    }

    // Default color: white
    if (colors.empty()) {
        colors.resize(vertexCount * 4, 1.0f);
    }

    // GLTF's V coordinate is typically flipped compared to OpenGL, so is flipped here.
    for (size_t i = 1; i < texCoords.size(); i += 2) { texCoords[i] = 1.0f - texCoords[i]; }

    // Compute AABB from positions while we have them as typed floats.
    se::render::AABB aabb{};
    for (size_t i = 0; i < vertexCount; ++i) {
        updateAABB(aabb, {positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]}, i);
    }

    // Build source map keyed by layout attribute name.
    // Adding a new attribute (e.g. bones) means adding one entry here and a layout element.
    auto asBytes = [](const std::vector<float>& v) -> std::span<const uint8_t> {
        return {reinterpret_cast<const uint8_t*>(v.data()), v.size() * sizeof(float)};
    };
    const VertexSourceMap sources = {
        {"a_Position", asBytes(positions)}, {"a_Normal", asBytes(normals)}, {"a_TexCoord", asBytes(texCoords)},
        {"a_Tangent", asBytes(tangents)},   {"a_Color", asBytes(colors)},
    };

    std::vector<uint8_t> vertices;
    packVertexData(sources, vertexCount, layout, vertices);

    auto indices = readIndices(gltfModel, primitive, vertexCount);

    return std::make_unique<se::render::Mesh>(vertices, indices, aabb, layout, instanced);
}

MaterialHandle resolveMaterial(const tinygltf::Primitive& primitive, const std::vector<MaterialHandle>& materials,
                               const MaterialHandle& fallback) {
    if (primitive.material >= 0 && primitive.material < static_cast<int>(materials.size())) {
        return materials[primitive.material];
    }
    return fallback;
}

}  // namespace

Model::Model(std::string gltfPath, std::string_view shaderPath, AssetManager& assetManager)
    : Asset(std::move(gltfPath)) {
    try {
        tinygltf::Model gltfModel = loadGltfModel(m_Path);
        std::string gltfDir = getDirectory(m_Path);

        auto gltfTextures = loadGltfTextures(gltfModel, gltfDir, assetManager, m_Path);
        auto shader = assetManager.getOrLoadShader(shaderPath);
        auto checkerboard = createCheckerboardTexture(assetManager);
        auto defaultMaterial = createDefaultMaterial(std::format("{}#default", m_Path), assetManager, shader);
        auto gltfMaterials = buildMaterials(gltfModel, assetManager, shader, gltfTextures, checkerboard);

        auto meshLayout = buildStaticMeshLayout();
        // Instance attribute slots begin after the per-vertex attributes.
        auto instanceAttribBase = static_cast<GLuint>(meshLayout.getElements().size());
        shader.get()->validateLayout(meshLayout, instanceAttribBase);

        size_t totalPrimitives = 0;
        for (const auto& mesh : gltfModel.meshes) { totalPrimitives += mesh.primitives.size(); }
        m_SubMeshes.reserve(totalPrimitives);

        for (const auto& mesh : gltfModel.meshes) {
            for (const auto& primitive : mesh.primitives) {
                auto meshPtr = buildMeshFromPrimitive(gltfModel, primitive, meshLayout, true);
                if (!meshPtr) {
                    continue;
                }
                m_SubMeshes.push_back({std::move(meshPtr), resolveMaterial(primitive, gltfMaterials, defaultMaterial)});
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format("Failed to load model '{}': {}", m_Path, e.what()));
    }
}

std::string Model::getDirectory(std::string_view filepath) {
    return std::filesystem::path(filepath).parent_path().string();
}

}  // namespace se::assets