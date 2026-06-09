#include "Model.h"

#define TINYGLTF_IMPLEMENTATION
#include <mikktspace.h>
#include <tiny_gltf.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
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

se::render::AABB computeAABB(const std::vector<float>& positions, size_t vertexCount) {
    se::render::AABB aabb{};
    for (size_t i = 0; i < vertexCount; ++i) {
        updateAABB(aabb, {positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]}, i);
    }
    return aabb;
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

se::render::BufferLayout buildSkinnedMeshLayout() {
    return se::render::BufferLayout({
        {"a_Position", GL_FLOAT, sizeof(float), 0, 3, GL_FALSE},
        {"a_Normal", GL_FLOAT, sizeof(float), 0, 3, GL_FALSE},
        {"a_TexCoord", GL_FLOAT, sizeof(float), 0, 2, GL_FALSE},
        {"a_Tangent", GL_FLOAT, sizeof(float), 0, 4, GL_FALSE},
        {"a_Color", GL_FLOAT, sizeof(float), 0, 4, GL_FALSE},
        {"a_Joints", GL_INT, sizeof(int32_t), 0, 4, GL_FALSE},
        {"a_Weights", GL_FLOAT, sizeof(float), 0, 4, GL_FALSE},
    });
}

size_t componentTypeSize(int componentType) {
    switch (componentType) {
    case TINYGLTF_COMPONENT_TYPE_BYTE:
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return sizeof(uint8_t);
    case TINYGLTF_COMPONENT_TYPE_SHORT:
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return sizeof(uint16_t);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    case TINYGLTF_COMPONENT_TYPE_FLOAT: return sizeof(uint32_t);
    default: throw std::runtime_error("Unsupported accessor component type");
    }
}

float decodeComponentAsFloat(const uint8_t* ptr, int componentType, bool normalized) {
    switch (componentType) {
    case TINYGLTF_COMPONENT_TYPE_FLOAT: return *reinterpret_cast<const float*>(ptr);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
        const auto v = *ptr;
        return normalized ? static_cast<float>(v) / 255.0f : static_cast<float>(v);
    }
    case TINYGLTF_COMPONENT_TYPE_BYTE: {
        const auto v = *reinterpret_cast<const int8_t*>(ptr);
        return normalized ? (std::max)(-1.0f, static_cast<float>(v) / 127.0f) : static_cast<float>(v);
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
        const auto v = *reinterpret_cast<const uint16_t*>(ptr);
        return normalized ? static_cast<float>(v) / 65535.0f : static_cast<float>(v);
    }
    case TINYGLTF_COMPONENT_TYPE_SHORT: {
        const auto v = *reinterpret_cast<const int16_t*>(ptr);
        return normalized ? (std::max)(-1.0f, static_cast<float>(v) / 32767.0f) : static_cast<float>(v);
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
        return static_cast<float>(*reinterpret_cast<const uint32_t*>(ptr));
    }
    default: throw std::runtime_error("Unsupported accessor component type");
    }
}

// Reads any strided accessor into a flat float vector, supporting normalized integer source formats.
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
    const size_t compSz = componentTypeSize(acc.componentType);
    const size_t elemSz = static_cast<size_t>(components) * compSz;
    const size_t stride = bv.byteStride > 0 ? bv.byteStride : elemSz;

    out.reserve(out.size() + acc.count * components);
    for (size_t i = 0; i < acc.count; ++i) {
        const uint8_t* elem = base + i * stride;
        for (int c = 0; c < components; ++c) {
            out.push_back(
                decodeComponentAsFloat(elem + static_cast<size_t>(c) * compSz, acc.componentType, acc.normalized));
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
    size_t baseOffset = bufferView.byteOffset + accessor.byteOffset;

    for (size_t i = 0; i < accessor.count; ++i) {
        indices.push_back(extractIndex(buffer.data, baseOffset + i * stride, accessor.componentType));
    }
    return indices;
}

void readPrimitiveAttributes(const tinygltf::Model& gltfModel, const tinygltf::Primitive& primitive,
                             std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texCoords,
                             std::vector<float>& tangents, std::vector<float>& colors) {
    for (const auto& [name, accessorIdx] : primitive.attributes) {
        if (name == "POSITION") {
            readStridedVec(gltfModel, gltfModel.accessors[accessorIdx], 3, positions);
        } else if (name == "NORMAL") {
            readStridedVec(gltfModel, gltfModel.accessors[accessorIdx], 3, normals);
        } else if (name == "TEXCOORD_0") {
            readStridedVec(gltfModel, gltfModel.accessors[accessorIdx], 2, texCoords);
        } else if (name == "TANGENT") {
            readStridedVec(gltfModel, gltfModel.accessors[accessorIdx], 4, tangents);
        } else if (name == "COLOR_0") {
            const auto& accessor = gltfModel.accessors[accessorIdx];
            int components = (accessor.type == TINYGLTF_TYPE_VEC4) ? 4 : 3;
            readStridedVec(gltfModel, accessor, components, colors);
            // If VEC3, expand to VEC4 with alpha=1 (in-place, back-to-front to avoid stomping)
            if (components == 3) {
                size_t count = colors.size() / 3;
                colors.resize(count * 4);
                for (size_t i = count; i-- > 0;) {
                    colors[i * 4 + 3] = 1.0f;
                    colors[i * 4 + 2] = colors[i * 3 + 2];
                    colors[i * 4 + 1] = colors[i * 3 + 1];
                    colors[i * 4 + 0] = colors[i * 3 + 0];
                }
            }
        }
    }
}

void readJointIndices(const tinygltf::Model& gltfModel, const tinygltf::Primitive& primitive,
                      std::vector<int32_t>& joints) {
    auto it = primitive.attributes.find("JOINTS_0");
    if (it == primitive.attributes.end()) {
        return;
    }

    const auto& acc = gltfModel.accessors[it->second];
    const auto& bv = gltfModel.bufferViews[acc.bufferView];
    const auto& buf = gltfModel.buffers[bv.buffer];
    const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;

    joints.resize(acc.count * 4);

    if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        size_t stride = bv.byteStride > 0 ? bv.byteStride : 4 * sizeof(uint8_t);
        for (size_t i = 0; i < acc.count; ++i) {
            const auto* elem = base + i * stride;
            for (int c = 0; c < 4; ++c) { joints[i * 4 + c] = static_cast<int32_t>(elem[c]); }
        }
    } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        size_t stride = bv.byteStride > 0 ? bv.byteStride : 4 * sizeof(uint16_t);
        for (size_t i = 0; i < acc.count; ++i) {
            const auto* elem = reinterpret_cast<const uint16_t*>(base + i * stride);
            for (int c = 0; c < 4; ++c) { joints[i * 4 + c] = static_cast<int32_t>(elem[c]); }
        }
    }
}

void readJointWeights(const tinygltf::Model& gltfModel, const tinygltf::Primitive& primitive,
                      std::vector<float>& weights) {
    auto it = primitive.attributes.find("WEIGHTS_0");
    if (it == primitive.attributes.end()) {
        return;
    }
    readStridedVec(gltfModel, gltfModel.accessors[it->second], 4, weights);
}

void normalizeSkinningWeights(std::vector<float>& weights, size_t vertexCount) {
    constexpr float EPS = 1e-6f;
    if (weights.size() < vertexCount * 4) {
        return;
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        float* w = weights.data() + i * 4;
        const float sum = w[0] + w[1] + w[2] + w[3];
        if (sum > EPS) {
            const float inv = 1.0f / sum;
            w[0] *= inv;
            w[1] *= inv;
            w[2] *= inv;
            w[3] *= inv;
        } else {
            // Keep a valid blend in degenerate cases instead of producing a zero skin matrix.
            w[0] = 1.0f;
            w[1] = 0.0f;
            w[2] = 0.0f;
            w[3] = 0.0f;
        }
    }
}

// Generates smooth normals by accumulating area-weighted face normals per vertex.
// Used when the glTF primitive has no NORMAL attribute.
void generateNormals(std::vector<float>& normals, const std::vector<float>& positions,
                     const std::vector<unsigned int>& indices, size_t vertexCount) {
    normals.assign(vertexCount * 3, 0.0f);

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const size_t i0 = indices[i];
        const size_t i1 = indices[i + 1];
        const size_t i2 = indices[i + 2];
        const glm::vec3 p0 = {positions[i0 * 3], positions[i0 * 3 + 1], positions[i0 * 3 + 2]};
        const glm::vec3 p1 = {positions[i1 * 3], positions[i1 * 3 + 1], positions[i1 * 3 + 2]};
        const glm::vec3 p2 = {positions[i2 * 3], positions[i2 * 3 + 1], positions[i2 * 3 + 2]};
        // Cross product length = 2 * triangle area, so this weights by area automatically.
        const glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
        for (size_t v : {i0, i1, i2}) {
            normals[v * 3] += n.x;
            normals[v * 3 + 1] += n.y;
            normals[v * 3 + 2] += n.z;
        }
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        glm::vec3 n = {normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]};
        float len = glm::length(n);
        n = (len > 1e-6f) ? n / len : glm::vec3(0.0f, 1.0f, 0.0f);
        normals[i * 3] = n.x;
        normals[i * 3 + 1] = n.y;
        normals[i * 3 + 2] = n.z;
    }
}

// Context passed through MikkTSpace callbacks.
struct MikkTSpaceContext {
    const std::vector<float>* positions;
    const std::vector<float>* normals;
    const std::vector<float>* texCoords;
    const std::vector<unsigned int>* indices;
    std::vector<float>* tangents;  // output: flat vec4 array (xyz=tangent, w=sign)
};

// MikkTSpace callback implementations, the library calls these to read geometry
// and write the computed tangents back.
// The C-style array parameters (float[3], float[2]) are part of the MikkTSpace C API
// and cannot be changed.
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
int mtsGetNumFaces(const SMikkTSpaceContext* ctx) {
    const auto* c = static_cast<const MikkTSpaceContext*>(ctx->m_pUserData);
    return static_cast<int>(c->indices->size() / 3);
}

int mtsGetNumVerticesOfFace(const SMikkTSpaceContext* /*ctx*/, int /*faceIdx*/) {
    return 3;  // always triangles
}

void mtsGetPosition(const SMikkTSpaceContext* ctx, float outPos[3], int face, int vert) {
    const auto* c = static_cast<const MikkTSpaceContext*>(ctx->m_pUserData);
    const size_t idx = (*c->indices)[static_cast<size_t>(face) * 3 + vert];
    outPos[0] = (*c->positions)[idx * 3];
    outPos[1] = (*c->positions)[idx * 3 + 1];
    outPos[2] = (*c->positions)[idx * 3 + 2];
}

void mtsGetNormal(const SMikkTSpaceContext* ctx, float outNorm[3], int face, int vert) {
    const auto* c = static_cast<const MikkTSpaceContext*>(ctx->m_pUserData);
    const size_t idx = (*c->indices)[static_cast<size_t>(face) * 3 + vert];
    outNorm[0] = (*c->normals)[idx * 3];
    outNorm[1] = (*c->normals)[idx * 3 + 1];
    outNorm[2] = (*c->normals)[idx * 3 + 2];
}

void mtsGetTexCoord(const SMikkTSpaceContext* ctx, float outUV[2], int face, int vert) {
    const auto* c = static_cast<const MikkTSpaceContext*>(ctx->m_pUserData);
    const size_t idx = (*c->indices)[static_cast<size_t>(face) * 3 + vert];
    outUV[0] = (*c->texCoords)[idx * 2];
    outUV[1] = (*c->texCoords)[idx * 2 + 1];
}

void mtsSetTSpaceBasic(const SMikkTSpaceContext* ctx, const float tangent[3], float sign, int face, int vert) {
    auto* c = static_cast<MikkTSpaceContext*>(ctx->m_pUserData);
    const size_t idx = (*c->indices)[static_cast<size_t>(face) * 3 + vert];
    (*c->tangents)[idx * 4] = tangent[0];
    (*c->tangents)[idx * 4 + 1] = tangent[1];
    (*c->tangents)[idx * 4 + 2] = tangent[2];
    (*c->tangents)[idx * 4 + 3] = sign;
}
// NOLINTEND(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

// Generates tangents using MikkTSpace. Requires normals and texcoords to already be populated.
// Falls back to a simple (1,0,0,1) default if UVs are all zero (no meaningful UV space to work with).
void generateTangentsMikkTSpace(std::vector<float>& tangents, const std::vector<float>& positions,
                                const std::vector<float>& normals, const std::vector<float>& texCoords,
                                const std::vector<unsigned int>& indices, size_t vertexCount) {
    tangents.assign(vertexCount * 4, 0.0f);

    // If there are no real UVs, MikkTSpace will produce garbage. Fall back to a neutral tangent.
    bool hasUVs = false;
    for (float v : texCoords) {
        if (v != 0.0f) {
            hasUVs = true;
            break;
        }
    }
    if (!hasUVs) {
        for (size_t i = 0; i < vertexCount; ++i) {
            tangents[i * 4] = 1.0f;
            tangents[i * 4 + 3] = 1.0f;
        }
        return;
    }

    MikkTSpaceContext userData{&positions, &normals, &texCoords, &indices, &tangents};

    SMikkTSpaceInterface iface{};
    iface.m_getNumFaces = mtsGetNumFaces;
    iface.m_getNumVerticesOfFace = mtsGetNumVerticesOfFace;
    iface.m_getPosition = mtsGetPosition;
    iface.m_getNormal = mtsGetNormal;
    iface.m_getTexCoord = mtsGetTexCoord;
    iface.m_setTSpaceBasic = mtsSetTSpaceBasic;
    iface.m_setTSpace = nullptr;  // basic variant is sufficient

    SMikkTSpaceContext mikkCtx{};
    mikkCtx.m_pInterface = &iface;
    mikkCtx.m_pUserData = &userData;

    if (!genTangSpaceDefault(&mikkCtx)) {
        // MikkTSpace failed, fall back to neutral tangent
        std::println("Warning: MikkTSpace tangent generation failed, using fallback");
        for (size_t i = 0; i < vertexCount; ++i) {
            tangents[i * 4] = 1.0f;
            tangents[i * 4 + 3] = 1.0f;
        }
    }
}

struct PrimitiveData {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<float> tangents;
    std::vector<float> colors;
    std::vector<int32_t> joints;
    std::vector<float> weights;
    std::vector<unsigned int> indices;  // stored here so generation steps can share them
    size_t vertexCount = 0;
    se::render::AABB aabb{};
};

bool readPrimitiveData(const tinygltf::Model& gltfModel, const tinygltf::Primitive& primitive, PrimitiveData& out) {
    auto posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end()) {
        return false;
    }

    const auto& posAccessor = gltfModel.accessors[posIt->second];
    if (posAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || posAccessor.type != TINYGLTF_TYPE_VEC3 ||
        posAccessor.bufferView < 0 || posAccessor.bufferView >= static_cast<int>(gltfModel.bufferViews.size())) {
        return false;
    }

    out.vertexCount = posAccessor.count;
    readPrimitiveAttributes(gltfModel, primitive, out.positions, out.normals, out.texCoords, out.tangents, out.colors);
    readJointIndices(gltfModel, primitive, out.joints);
    readJointWeights(gltfModel, primitive, out.weights);
    out.indices = readIndices(gltfModel, primitive, out.vertexCount);
    return true;
}

// Fills in any missing vertex attributes, generating normals and tangents from geometry where needed.
// texCoords default to (0,0) and colors to (1,1,1,1). Joints and weights default to zero (no skinning influence).
void fillMissingAttributes(PrimitiveData& data) {
    const size_t n = data.vertexCount;

    // Texcoords and colors: simple neutral defaults are fine.
    if (data.texCoords.empty()) {
        data.texCoords.resize(n * 2, 0.0f);
    }
    if (data.colors.empty()) {
        data.colors.assign(n * 4, 1.0f);
    }

    // glTF UVs are authored for top-left image origin; OpenGL texture sampling uses bottom-left origin.
    for (size_t i = 1; i < data.texCoords.size(); i += 2) { data.texCoords[i] = 1.0f - data.texCoords[i]; }

    // Normals: generate smooth area-weighted normals from geometry if absent.
    if (data.normals.empty()) {
        generateNormals(data.normals, data.positions, data.indices, n);
    }

    // Tangents: generate with MikkTSpace if absent. Requires normals and texcoords to be ready.
    if (data.tangents.empty()) {
        generateTangentsMikkTSpace(data.tangents, data.positions, data.normals, data.texCoords, data.indices, n);
    }

    // Skinning defaults: zero joints and weights mean the vertex is unaffected by any bone.
    if (data.joints.empty()) {
        data.joints.assign(n * 4, 0);
    }
    if (data.weights.empty()) {
        data.weights.assign(n * 4, 0.0f);
        for (size_t i = 0; i < n; ++i) { data.weights[i * 4] = 1.0f; }
    }

    normalizeSkinningWeights(data.weights, n);
}

struct VertexSources {
    std::span<const uint8_t> position;
    std::span<const uint8_t> normal;
    std::span<const uint8_t> texCoord;
    std::span<const uint8_t> tangent;
    std::span<const uint8_t> color;
    std::span<const uint8_t> joints;
    std::span<const uint8_t> weights;
};

std::span<const uint8_t> findSourceForAttribute(std::string_view attributeName, const VertexSources& sources) {
    if (attributeName == "a_Position") {
        return sources.position;
    }
    if (attributeName == "a_Normal") {
        return sources.normal;
    }
    if (attributeName == "a_TexCoord") {
        return sources.texCoord;
    }
    if (attributeName == "a_Tangent") {
        return sources.tangent;
    }
    if (attributeName == "a_Color") {
        return sources.color;
    }
    if (attributeName == "a_Joints") {
        return sources.joints;
    }
    if (attributeName == "a_Weights") {
        return sources.weights;
    }
    return {};
}

std::vector<std::span<const uint8_t>> buildOrderedSources(const VertexSources& sources,
                                                          const se::render::BufferLayout& layout) {
    std::vector<std::span<const uint8_t>> orderedSources;
    orderedSources.reserve(layout.getElements().size());
    for (const auto& elem : layout.getElements()) {
        orderedSources.push_back(findSourceForAttribute(elem.name, sources));
    }
    return orderedSources;
}

// Writes vertex data into a raw byte buffer driven entirely by the layout.
// Each element is written at its layout offset; missing attributes are left zeroed.
void packVertexData(const std::vector<std::span<const uint8_t>>& orderedSources, size_t vertexCount,
                    const se::render::BufferLayout& layout, std::vector<uint8_t>& vertices) {
    vertices.resize(vertexCount * layout.getStride(), 0);

    for (size_t i = 0; i < vertexCount; ++i) {
        uint8_t* vptr = vertices.data() + i * layout.getStride();
        for (size_t e = 0; e < layout.getElements().size(); ++e) {
            const auto& elem = layout.getElements()[e];
            const auto& source = orderedSources[e];
            if (source.empty()) {
                continue;
            }
            const size_t elemBytes = static_cast<size_t>(elem.size) * elem.count;
            const size_t srcOffset = i * elemBytes;
            if (srcOffset + elemBytes <= source.size()) {
                std::memcpy(vptr + elem.offset, source.data() + srcOffset, elemBytes);
            }
        }
    }
}

VertexSources buildVertexSources(const PrimitiveData& data) {
    auto asBytes = [](const std::vector<float>& v) -> std::span<const uint8_t> {
        return {reinterpret_cast<const uint8_t*>(v.data()), v.size() * sizeof(float)};
    };
    auto asBytesInt = [](const std::vector<int32_t>& v) -> std::span<const uint8_t> {
        return {reinterpret_cast<const uint8_t*>(v.data()), v.size() * sizeof(int32_t)};
    };

    return VertexSources{
        .position = asBytes(data.positions),
        .normal = asBytes(data.normals),
        .texCoord = asBytes(data.texCoords),
        .tangent = asBytes(data.tangents),
        .color = asBytes(data.colors),
        .joints = data.joints.empty() ? std::span<const uint8_t>{} : asBytesInt(data.joints),
        .weights = data.weights.empty() ? std::span<const uint8_t>{} : asBytes(data.weights),
    };
}

std::unique_ptr<se::render::Mesh> buildMeshFromPrimitive(const tinygltf::Model& gltfModel,
                                                         const tinygltf::Primitive& primitive,
                                                         const se::render::BufferLayout& layout, bool instanced) {
    PrimitiveData data;
    if (!readPrimitiveData(gltfModel, primitive, data)) {
        return nullptr;
    }

    fillMissingAttributes(data);
    data.aabb = computeAABB(data.positions, data.vertexCount);

    VertexSources sources = buildVertexSources(data);
    auto orderedSources = buildOrderedSources(sources, layout);
    std::vector<uint8_t> vertices;
    packVertexData(orderedSources, data.vertexCount, layout, vertices);

    return std::make_unique<se::render::Mesh>(vertices, data.indices, data.aabb, layout, instanced);
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
        } catch (const std::exception& e) {
            std::println("Warning: failed to load texture (source={}) from '{}': {}", texture.source, gltfPath,
                         e.what());
        }
        gltfTextures.push_back(handle);
    }
    return gltfTextures;
}

TextureHandle createCheckerboardTexture(AssetManager& assetManager) {
    static constexpr int SIZE = 64;
    static constexpr int TILE_SIZE = 8;
    static constexpr auto PIXELS = [] {
        std::array<uint8_t, static_cast<size_t>(SIZE * SIZE * 4)> p{};
        for (int y = 0; y < SIZE; ++y) {
            for (int x = 0; x < SIZE; ++x) {
                uint8_t* px = p.data() + static_cast<ptrdiff_t>((static_cast<size_t>(y) * SIZE + x) * 4);
                bool on = (((x / TILE_SIZE) + (y / TILE_SIZE)) % 2 == 0);
                px[0] = on ? 255 : 0;
                px[1] = 0;
                px[2] = on ? 255 : 0;
                px[3] = 255;
            }
        }
        return p;
    }();
    return assetManager.getOrLoadGeneratedTexture(
        "<checkerboard>", std::span<const uint8_t>(PIXELS.data(), PIXELS.size()), SIZE, SIZE, 4);
}

MaterialHandle createDefaultMaterial(std::string_view name, AssetManager& assetManager, const ShaderHandle& handle) {
    return assetManager.getOrLoadMaterial(name, handle, MaterialTextures{}, MaterialParams{}, RenderState{});
}

std::string resolveTexturePath(const tinygltf::Model& gltfModel, int texIndex) {
    if (texIndex < 0 || texIndex >= static_cast<int>(gltfModel.textures.size())) {
        return "unknown";
    }
    int src = gltfModel.textures[texIndex].source;
    if (src < 0 || src >= static_cast<int>(gltfModel.images.size())) {
        return "unknown";
    }
    const auto& img = gltfModel.images[src];
    return img.uri.empty() ? img.name : img.uri;
}

void warnMissingTexture(const tinygltf::Model& gltfModel, const std::string& matName, int texIndex,
                        const TextureHandle& handle, const char* slot) {
    if (texIndex >= 0 && !handle.isValid()) {
        std::println("Warning: material '{}' {} texture failed to load: '{}'", matName, slot,
                     resolveTexturePath(gltfModel, texIndex));
    }
}

MaterialParams extractMaterialParams(const tinygltf::Material& mat) {
    MaterialParams params;
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
    params.occlusionStrength = static_cast<float>(mat.occlusionTexture.strength);
    params.alphaCutoff = (mat.alphaMode == "MASK") ? static_cast<float>(mat.alphaCutoff) : 0.0f;
    return params;
}

TransparencyMode extractTransparencyMode(const tinygltf::Material& mat) {
    if (!mat.extras.IsObject()) {
        return TransparencyMode::Sorted;
    }

    auto parseTag = [](std::string_view tag) {
        if (tag == "oit") {
            return TransparencyMode::OIT;
        }
        if (tag == "sorted") {
            return TransparencyMode::Sorted;
        }
        return TransparencyMode::Sorted;
    };

    if (mat.extras.Has("renderQueue")) {
        const auto& rq = mat.extras.Get("renderQueue");
        if (rq.IsString()) {
            return parseTag(rq.Get<std::string>());
        }
    }

    if (mat.extras.Has("transparencyMode")) {
        const auto& tm = mat.extras.Get("transparencyMode");
        if (tm.IsString()) {
            return parseTag(tm.Get<std::string>());
        }
    }

    return TransparencyMode::Sorted;
}

std::vector<MaterialHandle> buildMaterials(const tinygltf::Model& gltfModel, AssetManager& assetManager,
                                           const ShaderHandle& handle, const std::vector<TextureHandle>& textures,
                                           const TextureHandle& fallbackBaseColor, std::string_view sourceModelPath) {
    std::vector<MaterialHandle> materials;
    materials.reserve(gltfModel.materials.size());

    for (size_t i = 0; i < gltfModel.materials.size(); ++i) {
        const auto& mat = gltfModel.materials[i];
        MaterialTextures matTextures;

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

        // Some assets pack AO in ORM but omit an explicit occlusion slot; use ORM as AO fallback.
        if (!matTextures.occlusion.isValid() && matTextures.metallicRoughness.isValid()) {
            matTextures.occlusion = matTextures.metallicRoughness;
        }

        std::string matName = mat.name.empty() ? "material_" + std::to_string(i) : mat.name;

        warnMissingTexture(gltfModel, matName, mat.pbrMetallicRoughness.baseColorTexture.index, matTextures.baseColor,
                           "base color");
        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0 && !matTextures.baseColor.isValid()) {
            matTextures.baseColor = fallbackBaseColor;
        }
        warnMissingTexture(gltfModel, matName, mat.pbrMetallicRoughness.metallicRoughnessTexture.index,
                           matTextures.metallicRoughness, "metallic-roughness");
        warnMissingTexture(gltfModel, matName, mat.normalTexture.index, matTextures.normal, "normal");
        warnMissingTexture(gltfModel, matName, mat.emissiveTexture.index, matTextures.emissive, "emissive");
        warnMissingTexture(gltfModel, matName, mat.occlusionTexture.index, matTextures.occlusion, "occlusion");

        MaterialParams params = extractMaterialParams(mat);

        RenderState state;
        state.blend = (mat.alphaMode == "BLEND");
        state.depthWrite = !state.blend;
        state.cull = !mat.doubleSided;
        state.transparency = state.blend ? extractTransparencyMode(mat) : TransparencyMode::Sorted;

        // Namespace by source model and material index to avoid cache collisions across files.
        std::string cacheName = std::format("{}#{}#{}", sourceModelPath, i, matName);
        materials.push_back(assetManager.getOrLoadMaterial(cacheName, handle, matTextures, params, state));
    }
    return materials;
}

MaterialHandle resolveMaterial(const tinygltf::Primitive& primitive, const std::vector<MaterialHandle>& materials,
                               const MaterialHandle& fallback) {
    if (primitive.material >= 0 && primitive.material < static_cast<int>(materials.size())) {
        return materials[primitive.material];
    }
    return fallback;
}

struct NodeTRS {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

NodeTRS extractNodeTRS(const tinygltf::Node& node) {
    NodeTRS trs;
    if (node.matrix.size() == 16) {
        glm::mat4 m = glm::make_mat4(node.matrix.data());
        trs.translation = glm::vec3(m[3]);
        trs.scale = glm::vec3(glm::length(m[0]), glm::length(m[1]), glm::length(m[2]));
        glm::mat3 rotMat(glm::vec3(m[0]) / trs.scale.x, glm::vec3(m[1]) / trs.scale.y, glm::vec3(m[2]) / trs.scale.z);
        trs.rotation = glm::quat_cast(rotMat);
        return trs;
    }
    if (node.translation.size() == 3) {
        trs.translation = glm::vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]),
                                    static_cast<float>(node.translation[2]));
    }
    if (node.rotation.size() == 4) {
        // glTF quaternion order: [x, y, z, w]
        trs.rotation = glm::quat(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]),
                                 static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));
    }
    if (node.scale.size() == 3) {
        trs.scale = glm::vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]),
                              static_cast<float>(node.scale[2]));
    }
    return trs;
}

Skeleton loadSkeleton(const tinygltf::Model& gltfModel, const tinygltf::Skin& skin) {
    Skeleton skeleton;

    // Build a parent index list for all nodes to help find bone hierarchies. Nodes that aren't joints will have -1.
    std::vector<int> nodeParent(gltfModel.nodes.size(), -1);
    for (size_t i = 0; i < gltfModel.nodes.size(); ++i) {
        for (int child : gltfModel.nodes[i].children) {
            if (child >= 0 && child < static_cast<int>(gltfModel.nodes.size())) {
                nodeParent[child] = static_cast<int>(i);
            }
        }
    }

    // Build node to joint mapping for quick lookup during animation loading. Nodes that aren't joints will have -1.
    skeleton.nodeToJoint.resize(gltfModel.nodes.size(), -1);
    for (size_t j = 0; j < skin.joints.size(); ++j) {
        int nodeIdx = skin.joints[j];
        if (nodeIdx >= 0 && nodeIdx < static_cast<int>(gltfModel.nodes.size())) {
            skeleton.nodeToJoint[nodeIdx] = static_cast<int>(j);
        }
    }

    std::vector<glm::mat4> inverseBindMatrices(skin.joints.size(), glm::mat4{1.0f});
    if (skin.inverseBindMatrices >= 0) {
        const auto& acc = gltfModel.accessors[skin.inverseBindMatrices];
        const auto& bv = gltfModel.bufferViews[acc.bufferView];
        const auto& buf = gltfModel.buffers[bv.buffer];
        const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
        size_t stride = bv.byteStride > 0 ? bv.byteStride : sizeof(glm::mat4);
        for (size_t i = 0; i < acc.count && i < skin.joints.size(); ++i) {
            std::memcpy(&inverseBindMatrices[i], base + i * stride, sizeof(glm::mat4));
        }
    }

    // Build bone list (ordered so parents come before children)
    skeleton.bones.resize(skin.joints.size());
    for (size_t j = 0; j < skin.joints.size(); ++j) {
        int nodeIdx = skin.joints[j];
        const auto& node = gltfModel.nodes[nodeIdx];
        auto& bone = skeleton.bones[j];

        bone.name = node.name;
        bone.inverseBindMatrix = inverseBindMatrices[j];

        auto trs = extractNodeTRS(node);
        bone.restPosition = trs.translation;
        bone.restRotation = trs.rotation;
        bone.restScale = trs.scale;

        int parentNodeIdx = nodeParent[nodeIdx];
        bone.parent = (parentNodeIdx >= 0) ? skeleton.nodeToJoint[parentNodeIdx] : -1;
    }

    return skeleton;
}

void readTranslationKeys(AnimationChannel& chan, const std::vector<float>& timestamps, const tinygltf::Model& gltfModel,
                         const tinygltf::AnimationSampler& sampler, float& duration) {
    std::vector<float> values;
    readStridedVec(gltfModel, gltfModel.accessors[sampler.output], 3, values);
    chan.translations.reserve(timestamps.size());
    for (size_t i = 0; i < timestamps.size(); ++i) {
        chan.translations.push_back({timestamps[i], glm::vec3(values[i * 3], values[i * 3 + 1], values[i * 3 + 2])});
        duration = std::max(duration, timestamps[i]);
    }
}

void readRotationKeys(AnimationChannel& chan, const std::vector<float>& timestamps, const tinygltf::Model& gltfModel,
                      const tinygltf::AnimationSampler& sampler, float& duration) {
    std::vector<float> values;
    readStridedVec(gltfModel, gltfModel.accessors[sampler.output], 4, values);
    chan.rotations.reserve(timestamps.size());
    for (size_t i = 0; i < timestamps.size(); ++i) {
        // glTF quaternion: [x, y, z, w] → glm::quat(w, x, y, z)
        chan.rotations.push_back(
            {timestamps[i], glm::quat(values[i * 4 + 3], values[i * 4], values[i * 4 + 1], values[i * 4 + 2])});
        duration = std::max(duration, timestamps[i]);
    }
}

void readScaleKeys(AnimationChannel& chan, const std::vector<float>& timestamps, const tinygltf::Model& gltfModel,
                   const tinygltf::AnimationSampler& sampler, float& duration) {
    std::vector<float> values;
    readStridedVec(gltfModel, gltfModel.accessors[sampler.output], 3, values);
    chan.scales.reserve(timestamps.size());
    for (size_t i = 0; i < timestamps.size(); ++i) {
        chan.scales.push_back({timestamps[i], glm::vec3(values[i * 3], values[i * 3 + 1], values[i * 3 + 2])});
        duration = std::max(duration, timestamps[i]);
    }
}

// Returns the AnimationChannel for the given bone, creating it if it doesn't exist yet.
AnimationChannel& getOrCreateChannel(AnimationClip& clip, std::vector<int>& channelByBone, int boneIdx) {
    int& channelIdx = channelByBone[static_cast<size_t>(boneIdx)];
    if (channelIdx >= 0) {
        return clip.channels[static_cast<size_t>(channelIdx)];
    }
    clip.channels.emplace_back();
    clip.channels.back().boneIndex = boneIdx;
    channelIdx = static_cast<int>(clip.channels.size()) - 1;
    return clip.channels.back();
}

// Ensures the timestamp vector for a sampler is populated, reading it lazily on first use.
const std::vector<float>& ensureSamplerTimestamps(const tinygltf::Model& gltfModel, const tinygltf::Animation& gltfAnim,
                                                  int samplerIdx, std::vector<std::vector<float>>& samplerTimestamps) {
    std::vector<float>& timestamps = samplerTimestamps[static_cast<size_t>(samplerIdx)];
    if (timestamps.empty()) {
        readStridedVec(gltfModel, gltfModel.accessors[gltfAnim.samplers[samplerIdx].input], 1, timestamps);
    }
    return timestamps;
}

void processAnimationChannel(const tinygltf::Model& gltfModel, const tinygltf::Animation& gltfAnim,
                             const tinygltf::AnimationChannel& gltfChannel, const Skeleton& skeleton,
                             std::vector<std::vector<float>>& samplerTimestamps, AnimationClip& clip,
                             std::vector<int>& channelByBone) {
    const int nodeIdx = gltfChannel.target_node;
    if (nodeIdx < 0 || nodeIdx >= static_cast<int>(skeleton.nodeToJoint.size())) {
        return;
    }
    const int boneIdx = skeleton.nodeToJoint[nodeIdx];
    if (boneIdx < 0) {
        return;
    }

    const int samplerIdx = gltfChannel.sampler;
    if (samplerIdx < 0 || samplerIdx >= static_cast<int>(gltfAnim.samplers.size())) {
        return;
    }

    const std::vector<float>& timestamps = ensureSamplerTimestamps(gltfModel, gltfAnim, samplerIdx, samplerTimestamps);

    AnimationChannel& chan = getOrCreateChannel(clip, channelByBone, boneIdx);

    const auto& sampler = gltfAnim.samplers[samplerIdx];
    if (sampler.interpolation == "STEP") {
        chan.interpolation = Interpolation::Step;
    }

    const std::string_view path = gltfChannel.target_path;
    if (path == "translation") {
        readTranslationKeys(chan, timestamps, gltfModel, sampler, clip.duration);
    } else if (path == "rotation") {
        readRotationKeys(chan, timestamps, gltfModel, sampler, clip.duration);
    } else if (path == "scale") {
        readScaleKeys(chan, timestamps, gltfModel, sampler, clip.duration);
    }
}

std::vector<AnimationClip> loadAnimations(const tinygltf::Model& gltfModel, const Skeleton& skeleton) {
    std::vector<AnimationClip> clips;
    clips.reserve(gltfModel.animations.size());

    for (size_t a = 0; a < gltfModel.animations.size(); ++a) {
        const auto& gltfAnim = gltfModel.animations[a];
        AnimationClip clip;
        clip.name = gltfAnim.name.empty() ? std::format("animation_{}", a) : gltfAnim.name;
        clip.duration = 0.0f;

        std::vector<int> channelByBone(skeleton.bones.size(), -1);

        // Pre-allocate timestamp vectors for each sampler to avoid redundant reads if multiple
        // channels share the same sampler.
        std::vector<std::vector<float>> samplerTimestamps(gltfAnim.samplers.size());

        for (const auto& gltfChannel : gltfAnim.channels) {
            processAnimationChannel(gltfModel, gltfAnim, gltfChannel, skeleton, samplerTimestamps, clip, channelByBone);
        }

        clips.push_back(std::move(clip));
    }
    return clips;
}

}  // namespace

Model::Model(std::string gltfPath, ShaderHandle handle, AssetManager& assetManager) : Asset(std::move(gltfPath)) {
    try {
        tinygltf::Model gltfModel = loadGltfModel(m_Name);
        std::string gltfDir = getDirectory(m_Name);

        bool isAnimated = !gltfModel.skins.empty();
        if (isAnimated) {
            m_Skeleton = loadSkeleton(gltfModel, gltfModel.skins[0]);
            m_Animations = loadAnimations(gltfModel, *m_Skeleton);
            m_AnimationIndexByName.reserve(m_Animations.size());
            for (int i = 0; i < static_cast<int>(m_Animations.size()); ++i) {
                m_AnimationIndexByName.try_emplace(m_Animations[i].name, i);
            }
        }

        auto gltfTextures = loadGltfTextures(gltfModel, gltfDir, assetManager, m_Name);
        auto checkerboard = createCheckerboardTexture(assetManager);
        auto defaultMaterial = createDefaultMaterial(std::format("{}#default", m_Name), assetManager, handle);
        auto gltfMaterials = buildMaterials(gltfModel, assetManager, handle, gltfTextures, checkerboard, m_Name);

        auto* shader = handle.get();
        if (!shader) {
            throw std::runtime_error("Shader handle is invalid");
        }
        auto meshLayout = isAnimated ? buildSkinnedMeshLayout() : buildStaticMeshLayout();
        shader->validateLayout(meshLayout);

        size_t totalPrimitives = 0;
        for (const auto& mesh : gltfModel.meshes) { totalPrimitives += mesh.primitives.size(); }
        m_SubMeshes.reserve(totalPrimitives);

        for (const auto& mesh : gltfModel.meshes) {
            for (const auto& primitive : mesh.primitives) {
                auto meshPtr = buildMeshFromPrimitive(gltfModel, primitive, meshLayout, !isAnimated);
                if (!meshPtr) {
                    continue;
                }
                m_SubMeshes.push_back({std::move(meshPtr), resolveMaterial(primitive, gltfMaterials, defaultMaterial)});
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format("Failed to load model '{}': {}", m_Name, e.what()));
    }
}

const Skeleton& Model::getSkeleton() const {
    if (!m_Skeleton) {
        throw std::runtime_error("Model does not contain a skeleton");
    }
    return *m_Skeleton;
}

std::optional<int> Model::findAnimationClipIndex(std::string_view clipName) const {
    if (const auto it = m_AnimationIndexByName.find(clipName); it != m_AnimationIndexByName.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string Model::getDirectory(std::string_view filepath) {
    return std::filesystem::path(filepath).parent_path().string();
}

}  // namespace se::assets