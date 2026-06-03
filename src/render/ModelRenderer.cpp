#include "ModelRenderer.h"

#include <glad/glad.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <stdexcept>

#include "UboDefinitions.h"
#include "assets/Shader.h"
#include "assets/Texture.h"
#include "scene/Animator.h"

namespace se::render {

void ModelRenderer::resetStateCache() {
    m_CachedBlendEnabled.reset();
    m_CachedDepthMaskWritable.reset();
    m_CachedCullEnabled.reset();
    m_CachedPolygonMode.reset();
}

void ModelRenderer::setBlendEnabled(const bool enabled) const {
    if (m_CachedBlendEnabled.has_value() && *m_CachedBlendEnabled == enabled) {
        return;
    }
    enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    m_CachedBlendEnabled = enabled;
}

void ModelRenderer::setDepthMask(const bool writable) const {
    if (m_CachedDepthMaskWritable.has_value() && *m_CachedDepthMaskWritable == writable) {
        return;
    }
    glDepthMask(writable ? GL_TRUE : GL_FALSE);
    m_CachedDepthMaskWritable = writable;
}

void ModelRenderer::setCullEnabled(const bool enabled) const {
    if (m_CachedCullEnabled.has_value() && *m_CachedCullEnabled == enabled) {
        return;
    }
    enabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    m_CachedCullEnabled = enabled;
}

void ModelRenderer::setPolygonMode(const GLenum mode) const {
    if (m_CachedPolygonMode.has_value() && *m_CachedPolygonMode == mode) {
        return;
    }
    glPolygonMode(GL_FRONT_AND_BACK, mode);
    m_CachedPolygonMode = mode;
}

ModelRenderer::ModelRenderer(const float anisotropy) : m_Anisotropy((std::max)(1.0f, anisotropy)) {
    setupFrameUbo();
    setupBoneUbo();
    setupDefaultSampler();
    setupDefaultTextures();
    Mesh::setDefaultInstanceCapacityBytes(m_MaxBatchSize * sizeof(InstanceData));
}

ModelRenderer::~ModelRenderer() {
    if (m_DefaultSampler) {
        glDeleteSamplers(1, &m_DefaultSampler);
    }
    glDeleteTextures(static_cast<GLsizei>(m_DefaultTextures.size()), m_DefaultTextures.data());
}

void ModelRenderer::submit(const se::scene::Renderable& renderable, const Frustum& frustum,
                           const glm::mat4& viewMatrix) {
    const glm::mat4 modelMatrix = renderable.resolvedTransform().getMatrix();
    if (!frustumIntersectsAABB(frustum, renderable.mesh->getAABB(), modelMatrix)) {
        return;
    }
    m_Queue.submit(renderable, modelMatrix, viewMatrix);
}

void ModelRenderer::flush(const se::scene::LightData& lights, const se::scene::Camera& camera, RenderStats& stats) {
    if (m_Queue.isEmpty()) {
        return;
    }

    updateFrameUbo(lights, camera);
    resetStateCache();

    drawOpaquePass(stats);
    drawTransparentPass(stats);

    restoreRenderState();

    m_Queue.clear();
}

void ModelRenderer::restoreRenderState() const {
    setBlendEnabled(true);
    setDepthMask(true);
    setCullEnabled(true);
    setPolygonMode(GL_FILL);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_LINE_SMOOTH);
}

void ModelRenderer::drawOpaquePass(RenderStats& stats) const {
    setBlendEnabled(false);
    setDepthMask(true);
    setCullEnabled(true);
    setPolygonMode(m_Wireframe ? GL_LINE : GL_FILL);
    if (m_Wireframe) {
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.5f, 1.0f);
    }

    // static opaque pass (instanced where possible, sorted by material then mesh to minimize state changes)
    for (const auto& batchView : m_Queue.getOrderedStaticOpaqueBatches()) {
        flushBatch(batchView.key, batchView.batch, stats);
    }

    // opaque animated pass (non-instanced) rendered after static to leverage early-z for occlusion of expensive
    // animated models
    for (const auto& d : m_Queue.getOpaqueAnimatedDrawItems()) { drawAnimatedDrawItem(d, stats); }
}

void ModelRenderer::drawTransparentPass(RenderStats& stats) {
    setBlendEnabled(true);
    setDepthMask(false);

    // transparent objects need to be depth sorted back-to-front for correct blending.
    const auto& sorted = m_Queue.getDepthSortedTransparentDrawItems();
    BatchKey lastKey = {};
    std::vector<InstanceData> batch;
    for (const auto& d : sorted) {
        if (d.animator != nullptr) {  // animated transparent
            // flush any pending transparent static batch
            if (!batch.empty()) {
                flushBatch(lastKey, batch, stats);
                batch.clear();
            }
            drawAnimatedDrawItem(d, stats);
        } else {  // static transparent
            BatchKey key{d.mesh, d.material};
            if (key != lastKey) {
                if (!batch.empty()) {
                    flushBatch(lastKey, batch, stats);
                    batch.clear();
                }
                lastKey = key;
            }
            batch.push_back({d.modelMatrix, d.normalMatrix});
        }
    }
    // flush any remaining transparent static batch
    if (!batch.empty()) {
        flushBatch(lastKey, batch, stats);
    }
}

void ModelRenderer::setWireframe(bool enabled) { m_Wireframe = enabled; }

void ModelRenderer::setBatchSize(const size_t maxInstances) {
    assert(m_Queue.isEmpty() && "setBatchSize called mid-frame with live batches");
    m_MaxBatchSize = maxInstances;
    Mesh::setDefaultInstanceCapacityBytes(m_MaxBatchSize * sizeof(InstanceData));
}

void ModelRenderer::setupFrameUbo() { m_FrameUbo.emplace(sizeof(FrameUbo), UboBinding::Frame); }

void ModelRenderer::setupBoneUbo() { m_BoneUbo.emplace(sizeof(BoneUbo), UboBinding::Bones); }

// Sets default sampler parameters for all materials that don't specify their own sampler.
// This is separate from the Texture class because some materials might want different sampler settings (e.g. clamp vs
// repeat).
void ModelRenderer::setupDefaultSampler() {
    glCreateSamplers(1, &m_DefaultSampler);
    glSamplerParameteri(m_DefaultSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(m_DefaultSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(m_DefaultSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(m_DefaultSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_EXT_texture_filter_anisotropic
    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
    if (maxAniso > 0.0f) {
        glSamplerParameterf(m_DefaultSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, (std::min)(m_Anisotropy, maxAniso));
    }
#endif
    glObjectLabel(GL_SAMPLER, m_DefaultSampler, -1, "DefaultSampler");
}

void init1x1(GLuint tex, std::span<const uint8_t, 4> pixel, const char* label) {
    glTextureStorage2D(tex, 1, GL_RGBA8, 1, 1);
    glTextureSubImage2D(tex, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    glObjectLabel(GL_TEXTURE, tex, -1, label);
}

void ModelRenderer::setupDefaultTextures() {
    // Create 1x1 neutral textures: white, flat normal, black
    glCreateTextures(GL_TEXTURE_2D, static_cast<GLsizei>(m_DefaultTextures.size()), m_DefaultTextures.data());

    constexpr std::array<uint8_t, 4> WHITE = {255, 255, 255, 255};
    constexpr std::array<uint8_t, 4> FLATNORMAL = {128, 128, 255, 255};  // (0,0,1) normal encoded in [0,255]
    constexpr std::array<uint8_t, 4> BLACK = {0, 0, 0, 255};

    init1x1(m_DefaultTextures[0], WHITE, "DefaultWhite1x1");
    init1x1(m_DefaultTextures[1], FLATNORMAL, "DefaultFlatNormal1x1");
    init1x1(m_DefaultTextures[2], BLACK, "DefaultBlack1x1");
}

void ModelRenderer::bindMaterialTextures(const se::assets::MaterialTextures& textures) const {
    // Slot 0: base color (default: white or checkerboard if base color failed to load)
    // Slot 1: normal map (default: flat normal)
    // Slot 2: metallic-roughness (default: black = non-metallic, smooth)
    // Slot 3: emissive (default: black = no emission)
    // Slot 4: occlusion (default: white = fully lit)
    auto bindTexture = [this](unsigned int slot, const se::assets::TextureHandle& handle, GLuint fallback) {
        if (handle.isValid()) {
            handle.get()->bind(slot);
        } else {
            glBindTextureUnit(slot, fallback);
        }
        glBindSampler(slot, m_DefaultSampler);
    };

    bindTexture(0, textures.baseColor, m_DefaultTextures[0]);          // white
    bindTexture(1, textures.normal, m_DefaultTextures[1]);             // flat normal
    bindTexture(2, textures.metallicRoughness, m_DefaultTextures[2]);  // black
    bindTexture(3, textures.emissive, m_DefaultTextures[2]);           // black
    bindTexture(4, textures.occlusion, m_DefaultTextures[0]);          // white
}

void ModelRenderer::flushBatch(const BatchKey& key, const std::span<const InstanceData> batch,
                               RenderStats& stats) const {
    if (batch.empty()) {
        return;
    }

    const auto& state = key.material->getState();
    setCullEnabled(state.cull);

    auto shader = key.material->getShaderHandle().get();
    if (!shader) {
        throw std::runtime_error("Material missing shader");
    }
    shader->bind();

    bindMaterialTextures(key.material->getTextures());

    const auto& params = key.material->getParams();
    shader->setVec4("u_BaseColorFactor", params.baseColorFactor);
    shader->setFloat("u_AlphaCutoff", params.alphaCutoff);
    shader->setFloat("u_MetallicFactor", params.metallicFactor);
    shader->setFloat("u_RoughnessFactor", params.roughnessFactor);
    shader->setVec3("u_EmissiveFactor", params.emissiveFactor);

    key.mesh->updateInstanceBuffer(std::as_bytes(batch));
    key.mesh->drawInstanced(batch.size());

    ++stats.modelsDrawCalls;
    stats.modelsTriangles += static_cast<unsigned int>((key.mesh->getIndexCount() / 3) * batch.size());
}

void ModelRenderer::drawAnimatedDrawItem(const RenderQueue::DrawItem& drawItem, RenderStats& stats) const {
    if (!drawItem.mesh || !drawItem.material || drawItem.animator == nullptr) {
        throw std::runtime_error("Invalid animated draw call");
    }

    const auto& state = drawItem.material->getState();
    setCullEnabled(state.cull);

    auto shader = drawItem.material->getShaderHandle().get();
    if (!shader) {
        throw std::runtime_error("Animated material missing shader");
    }

    shader->bind();
    bindMaterialTextures(drawItem.material->getTextures());

    const auto& params = drawItem.material->getParams();
    shader->setVec4("u_BaseColorFactor", params.baseColorFactor);
    shader->setFloat("u_AlphaCutoff", params.alphaCutoff);
    shader->setFloat("u_MetallicFactor", params.metallicFactor);
    shader->setFloat("u_RoughnessFactor", params.roughnessFactor);
    shader->setVec3("u_EmissiveFactor", params.emissiveFactor);

    shader->setMat4("u_Model", drawItem.modelMatrix);
    shader->setMat3("u_Normal", drawItem.normalMatrix);

    const auto boneMatrices = drawItem.animator->bones();
    m_BoneUbo->updateSubData(0, std::as_bytes(std::span<const glm::mat4>(boneMatrices)));

    drawItem.mesh->draw();

    ++stats.animatedModelsDrawCalls;
    stats.animatedModelsTriangles += static_cast<unsigned int>(drawItem.mesh->getIndexCount() / 3);
}

void ModelRenderer::updateFrameUbo(const se::scene::LightData& lights, const se::scene::Camera& camera) {
    FrameUbo data{};

    data.viewProj = camera.getViewProjection();
    data.cameraPos = glm::vec4(camera.getPosition(), 1.0f);
    data.ambient = glm::vec4(lights.ambientColor, lights.ambientIntensity);

    if (!lights.directionalLights.empty()) {
        const auto& sun = lights.directionalLights[0];
        data.sunDir = glm::vec4(glm::normalize(sun.direction), 0.0f);
        data.sunColor = glm::vec4(sun.color, sun.intensity);
    }

    data.ambient = glm::vec4(lights.ambientColor, lights.ambientIntensity);

    // (std::min) is between parentheses to avoid macro expansion, as min from windows.h causes problems with std::min
    // usage in this file We only support up to 4 point lights in the shader, so we need to clamp the count and ignore
    // any extra lights
    const int pointCount = (std::min)(static_cast<int>(lights.pointLights.size()), 4);
    data.lightCounts = glm::ivec4(pointCount, 0, 0, 0);

    for (int i = 0; i < pointCount; ++i) {
        const auto& l = lights.pointLights[i];
        data.pointLights.at(i) = {
            .positionRange = glm::vec4(l.position, l.range),
            .colorIntensity = glm::vec4(l.color, l.intensity),
        };
    }

    m_FrameUbo->updateSubData(0, data);
}

}  // namespace se::render