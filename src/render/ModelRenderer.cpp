#include "ModelRenderer.h"

#include <glad/glad.h>

#include <algorithm>
#include <cassert>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/norm.hpp>
#include <stdexcept>

#include "assets/Shader.h"
#include "assets/Texture.h"

namespace se::render {

namespace {

struct PointLightGpuData {
    glm::vec4 positionRange;
    glm::vec4 colorIntensity;
};

struct FrameUbo {
    glm::mat4 viewProj;
    glm::vec4 sunDir;
    glm::vec4 sunColor;
    glm::vec4 ambient;
    glm::vec4 lightCounts;
    PointLightGpuData pointLights[4];
};

}  // namespace

ModelRenderer::ModelRenderer() {
    setupFrameUbo();
    Mesh::setDefaultInstanceCapacityBytes(m_MaxBatchSize * sizeof(InstanceData));
}

void ModelRenderer::submit(const se::scene::Renderable& renderable,
                           const Frustum& frustum) {
    m_Queue.submit(renderable, frustum);
}

void ModelRenderer::flush(const se::scene::LightData& lights,
                          const se::scene::Camera& camera,
                          RenderStats& stats) {
    if (m_Queue.getOpaqueBatches().empty() &&
        m_Queue.getTransparentBatches().empty()) return;

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

    for (auto& [key, batch] : m_Queue.getOpaqueBatches())
        flushBatch(key, batch, stats);

    // transparent pass
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    for (auto& draw : getSortedTransparentDraws(camera))
        flushBatch(draw.key, *draw.batch, stats);

    // restore known good state
    glEnable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_LINE_SMOOTH);

    m_Queue.clear();
}

void ModelRenderer::setWireframe(bool enabled) {
    m_Wireframe = enabled;
}

void ModelRenderer::setBatchSize(const size_t maxInstances) {
    assert(m_Queue.getOpaqueBatches().empty() &&
           m_Queue.getTransparentBatches().empty() &&
           "setBatchSize called mid-frame with live batches");
    m_MaxBatchSize = maxInstances;
    Mesh::setDefaultInstanceCapacityBytes(m_MaxBatchSize * sizeof(InstanceData));
}

void ModelRenderer::setupFrameUbo() {
    m_FrameUbo.emplace(sizeof(FrameUbo), 0);
}

void ModelRenderer::flushBatch(const BatchKey& key,
                               BatchData& batch,
                               RenderStats& stats) {
    if (batch.instances.empty()) return;

    const auto& state = key.material->getState();
    state.cull ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);

    const auto shader = key.material->getShaderHandle().get();
    if (!shader) throw std::runtime_error("Material missing shader");
    shader->bind();

    key.material->getBaseColorHandle().get()->bind(0);

    const auto& params = key.material->getParams();
    shader->setVec4("u_BaseColorFactor", &params.baseColorFactor[0]);
    shader->setFloat("u_AlphaCutoff", params.alphaCutoff);

    key.mesh->updateInstanceBuffer(batch.instances);
    key.mesh->drawInstanced(batch.instances.size());

    ++stats.modelDrawCalls;
    stats.modelTriangles += static_cast<unsigned int>(
        (key.mesh->getIndexCount() / 3) * batch.instances.size());
}

std::vector<ModelRenderer::TransparentDraw>
ModelRenderer::getSortedTransparentDraws(const se::scene::Camera& camera) {
    std::vector<TransparentDraw> draws;
    draws.reserve(m_Queue.getTransparentBatches().size());

    const glm::vec3 camPos = camera.getPosition();
    for (auto& [key, batch] : m_Queue.getTransparentBatches()) {
        if (batch.instances.empty()) continue;
        const glm::vec3 center = batch.centerSum /
                                 static_cast<float>(batch.instances.size());
        draws.push_back({
            .distance = glm::length2(camPos - center),
            .key = key,
            .batch = &batch,
        });
    }

    std::ranges::sort(draws, std::greater{}, &TransparentDraw::distance);
    return draws;
}

void ModelRenderer::updateFrameUbo(const se::scene::LightData& lights,
                                   const se::scene::Camera& camera) {
    FrameUbo data{.viewProj = camera.getViewProjection()};

    if (lights.sun) {
        data.sunDir = glm::vec4(glm::normalize(lights.sun->direction), 0.0f);
        data.sunColor = glm::vec4(lights.sun->color * lights.sun->intensity, 0.0f);
    }

    data.ambient = glm::vec4(lights.ambientColor, lights.ambientStrength);

    const int pointCount = std::min(
        static_cast<int>(lights.pointLights.size()), 4);
    data.lightCounts = glm::vec4(static_cast<float>(pointCount), 0, 0, 0);

    for (int i = 0; i < pointCount; ++i) {
        const auto& l = lights.pointLights[i];
        data.pointLights[i] = {
            .positionRange = glm::vec4(l.position, l.range),
            .colorIntensity = glm::vec4(l.color * l.intensity, l.intensity),
        };
    }

    m_FrameUbo->updateSubData(0, data);
}

}  // namespace se::render