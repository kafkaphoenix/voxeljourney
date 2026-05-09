#include "ModelRenderer.h"

#include <glad/glad.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/norm.hpp>
#include <stdexcept>

#include "UboBindings.h"
#include "assets/Shader.h"
#include "assets/Texture.h"

namespace se::render {

namespace {

struct alignas(16) PointLightGpuData {
    glm::vec4 positionRange;
    glm::vec4 colorIntensity;
};

struct alignas(16) FrameUbo {
    glm::mat4 viewProj;
    glm::vec4 sunDir;
    glm::vec4 sunColor;
    glm::vec4 ambient;
    glm::ivec4 lightCounts;
    std::array<PointLightGpuData, 4> pointLights;
};

}  // namespace

ModelRenderer::ModelRenderer() {
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

void ModelRenderer::submit(const se::scene::Renderable& renderable, const Frustum& frustum) {
    m_Queue.submit(renderable, frustum);
}

void ModelRenderer::submitAnimated(const se::scene::Renderable& renderable, const Frustum& frustum,
                                   std::span<const glm::mat4> boneMatrices) {
    if (!renderable.mesh || !renderable.material.get()) {
        return;
    }

    const glm::mat4 modelMatrix = renderable.transform.getMatrix();
    if (!frustumIntersectsAABB(frustum, renderable.mesh->getAABB(), modelMatrix)) {
        return;
    }

    m_AnimatedDraws.push_back(AnimatedDraw{
        .mesh = renderable.mesh,
        .material = renderable.material.get().get(),
        .modelMatrix = modelMatrix,
        .normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix))),
        .boneMatrices = {boneMatrices.begin(), boneMatrices.end()},
    });
}

void ModelRenderer::flush(const se::scene::LightData& lights, const se::scene::Camera& camera, RenderStats& stats) {
    if (m_Queue.getOpaqueBatches().empty() && m_Queue.getTransparentBatches().empty() && m_AnimatedDraws.empty()) {
        return;
    }

    updateFrameUbo(lights, camera);

    // opaque pass
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, m_Wireframe ? GL_LINE : GL_FILL);
    if (m_Wireframe) {
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.5f, 1.0f);
    }

    for (auto& [key, batch] : m_Queue.getOpaqueBatches()) { flushBatch(key, batch, stats); }

    // Animated draws (rendered individually, not instanced)
    if (!m_AnimatedDraws.empty()) {
        flushAnimatedDraws(stats);
    }

    // transparent pass
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    for (auto& draw : getSortedTransparentDraws(camera)) { flushBatch(draw.key, *draw.batch, stats); }

    // restore known good state
    glEnable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_LINE_SMOOTH);

    m_Queue.clear();
    m_AnimatedDraws.clear();
}

void ModelRenderer::setWireframe(bool enabled) { m_Wireframe = enabled; }

void ModelRenderer::setBatchSize(const size_t maxInstances) {
    assert(m_Queue.getOpaqueBatches().empty() && m_Queue.getTransparentBatches().empty() &&
           "setBatchSize called mid-frame with live batches");
    m_MaxBatchSize = maxInstances;
    Mesh::setDefaultInstanceCapacityBytes(m_MaxBatchSize * sizeof(InstanceData));
}

void ModelRenderer::setupFrameUbo() { m_FrameUbo.emplace(sizeof(FrameUbo), UboBinding::Frame); }

void ModelRenderer::setupBoneUbo() {
    m_BoneUbo.emplace(static_cast<GLsizeiptr>(se::assets::MAX_BONES * sizeof(glm::mat4)), UboBinding::Bones);
}

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
        glSamplerParameterf(m_DefaultSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, (std::min)(4.0f, maxAniso));
    }
#endif
    glObjectLabel(GL_SAMPLER, m_DefaultSampler, -1, "DefaultSampler");
}

void ModelRenderer::setupDefaultTextures() {
    // Create 1x1 neutral textures: white, flat normal, black
    glCreateTextures(GL_TEXTURE_2D, static_cast<GLsizei>(m_DefaultTextures.size()), m_DefaultTextures.data());

    auto init1x1 = [](GLuint tex, const uint8_t* pixel, const char* label) {
        glTextureStorage2D(tex, 1, GL_RGBA8, 1, 1);
        glTextureSubImage2D(tex, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        glObjectLabel(GL_TEXTURE, tex, -1, label);
    };

    constexpr uint8_t white[] = {255, 255, 255, 255};
    constexpr uint8_t flatNormal[] = {128, 128, 255, 255};
    constexpr uint8_t black[] = {0, 0, 0, 255};

    init1x1(m_DefaultTextures[0], white, "DefaultWhite1x1");
    init1x1(m_DefaultTextures[1], flatNormal, "DefaultFlatNormal1x1");
    init1x1(m_DefaultTextures[2], black, "DefaultBlack1x1");
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

void ModelRenderer::flushBatch(const BatchKey& key, BatchData& batch, RenderStats& stats) const {
    if (batch.instances.empty()) {
        return;
    }

    const auto& state = key.material->getState();
    state.cull ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);

    const auto shader = key.material->getShaderHandle().get();
    if (!shader) {
        throw std::runtime_error("Material missing shader");
    }
    shader->bind();

    bindMaterialTextures(key.material->getTextures());

    const auto& params = key.material->getParams();
    shader->setVec4("u_BaseColorFactor", &params.baseColorFactor[0]);
    shader->setFloat("u_AlphaCutoff", params.alphaCutoff);
    shader->setFloat("u_MetallicFactor", params.metallicFactor);
    shader->setFloat("u_RoughnessFactor", params.roughnessFactor);
    shader->setVec3("u_EmissiveFactor", &params.emissiveFactor[0]);

    key.mesh->updateInstanceBuffer(batch.instances);
    key.mesh->drawInstanced(batch.instances.size());

    ++stats.modelDrawCalls;
    stats.modelTriangles += static_cast<unsigned int>((key.mesh->getIndexCount() / 3) * batch.instances.size());
}

void ModelRenderer::flushAnimatedDraws(RenderStats& stats) const {
    for (const auto& draw : m_AnimatedDraws) {
        const auto& matState = draw.material->getState();
        matState.cull ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);

        const auto shader = draw.material->getShaderHandle().get();
        if (!shader)
            continue;
        shader->bind();

        bindMaterialTextures(draw.material->getTextures());

        const auto& params = draw.material->getParams();
        shader->setVec4("u_BaseColorFactor", &params.baseColorFactor[0]);
        shader->setFloat("u_AlphaCutoff", params.alphaCutoff);
        shader->setFloat("u_MetallicFactor", params.metallicFactor);
        shader->setFloat("u_RoughnessFactor", params.roughnessFactor);
        shader->setVec3("u_EmissiveFactor", &params.emissiveFactor[0]);

        shader->setMat4("u_Model", &draw.modelMatrix[0][0]);
        shader->setMat3("u_NormalMatrix", &draw.normalMatrix[0][0]);

        m_BoneUbo->updateSubData(0, std::as_bytes(std::span(draw.boneMatrices)));

        draw.mesh->draw();

        ++stats.animatedModelDrawCalls;
        stats.animatedModelTriangles += static_cast<unsigned int>(draw.mesh->getIndexCount() / 3);
    }
}

std::vector<ModelRenderer::TransparentDraw> ModelRenderer::getSortedTransparentDraws(const se::scene::Camera& camera) {
    std::vector<TransparentDraw> draws;
    draws.reserve(m_Queue.getTransparentBatches().size());

    const glm::vec3 camPos = camera.getPosition();
    for (auto& [key, batch] : m_Queue.getTransparentBatches()) {
        if (batch.instances.empty()) {
            continue;
        }

        const glm::vec3 center = batch.centerSum / static_cast<float>(batch.instances.size());
        draws.push_back(TransparentDraw{
            .distance = glm::length2(camPos - center),
            .key = key,
            .batch = &batch,
        });
    }

    std::ranges::sort(draws, std::greater{}, &TransparentDraw::distance);
    return draws;
}

void ModelRenderer::updateFrameUbo(const se::scene::LightData& lights, const se::scene::Camera& camera) {
    FrameUbo data{.viewProj = camera.getViewProjection()};

    if (!lights.directionalLights.empty()) {
        const auto& sun = lights.directionalLights[0];
        data.sunDir = glm::vec4(glm::normalize(sun.direction), 0.0f);
        data.sunColor = glm::vec4(sun.color * sun.intensity, 0.0f);
    }

    data.ambient = glm::vec4(lights.ambientColor, lights.ambientStrength);

    // (std::min) is between parentheses to avoid macro expansion, as min from windows.h causes problems with std::min
    // usage in this file We only support up to 4 point lights in the shader, so we need to clamp the count and ignore
    // any extra lights
    const int pointCount = (std::min)(static_cast<int>(lights.pointLights.size()), 4);
    data.lightCounts = glm::ivec4(pointCount, 0, 0, 0);

    for (int i = 0; i < pointCount; ++i) {
        const auto& l = lights.pointLights[i];
        data.pointLights.at(i) = {
            .positionRange = glm::vec4(l.position, l.range),
            .colorIntensity = glm::vec4(l.color * l.intensity, l.intensity),
        };
    }

    m_FrameUbo->updateSubData(0, data);
}

}  // namespace se::render