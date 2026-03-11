#include "Renderer.h"

#include <glad/glad.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <stdexcept>

#include "Frustum.h"
#include "assets/Texture.h"
#include "scene/Scene.h"

namespace se::render {

namespace {
struct PointLightUbo {
    glm::vec4 positionRange;
    glm::vec4 colorIntensity;
};

struct FrameUbo {
    glm::mat4 viewProj;
    glm::vec4 sunDir;
    glm::vec4 sunColor;
    glm::vec4 ambient;
    glm::vec4 lightCounts;
    PointLightUbo pointLights[4];
};
}

Renderer::Renderer() {
    setupGlState();
    setupFrameUbo();
    Mesh::setDefaultInstanceCapacityBytes(m_MaxBatchSize * sizeof(InstanceData));
}

void Renderer::render(const se::scene::Scene& scene) {
    setLights(scene.buildLightSet());
    clear();
    for (const auto& renderable : scene.getRenderables()) {
        submit(renderable);
    }
    flush();
}

void Renderer::setBatchSize(size_t maxInstances) {
    m_MaxBatchSize = maxInstances;
    Mesh::setDefaultInstanceCapacityBytes(m_MaxBatchSize * sizeof(InstanceData));
}

void Renderer::setupGlState() {
    glEnable(GL_DEPTH_TEST);                            // For 3D rendering allows that closer objects occlude farther ones
    glDepthFunc(GL_LESS);                               // Accept fragment if it is closer to the camera than the former one
    glEnable(GL_CULL_FACE);                             // Enable back-face culling to improve performance by not rendering faces that are facing away from the camera
    glCullFace(GL_BACK);                                // Cull back faces. Disable culling for double-sided materials like water or foliage
    glFrontFace(GL_CCW);                                // Define front faces as counter-clockwise winding order
    glEnable(GL_BLEND);                                 // Enable blending for transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Says how to blend source and destination colors based on alpha
}

void Renderer::resetGlState() {
    glEnable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    applyWireframeState();
}

void Renderer::applyWireframeState() {
    glPolygonMode(GL_FRONT_AND_BACK, m_Wireframe ? GL_LINE : GL_FILL);
    if (m_Wireframe) {
        glEnable(GL_LINE_SMOOTH);                // Enable line smoothing for better visual quality in wireframe mode
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);  // Set line smoothing hint to nicest for best quality
        glEnable(GL_POLYGON_OFFSET_FILL);        // Enable polygon offset to reduce z-fighting in wireframe mode
        glPolygonOffset(0.5f, 1.0f);             // Set polygon offset factors to push wireframe slightly back to prevent z-fighting with filled polygons
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_LINE_SMOOTH);
    }
}

void Renderer::setupFrameUbo() {
    m_FrameUbo = UniformBuffer(sizeof(FrameUbo), 0);
}

void Renderer::clear() {
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::submit(const se::scene::Renderable& renderable) {
    if (!renderable.mesh) {
        throw std::runtime_error("Renderable missing mesh");
    }

    auto materialPtr = renderable.material.get();
    if (!materialPtr) {
        throw std::runtime_error("Renderable missing material");
    }

    if (!m_Camera) {
        throw std::runtime_error("Renderer error: No camera set for rendering!");
    }

    glm::mat4 modelMatrix = renderable.transform.getMatrix();
    Frustum frustum = extractFrustum(m_Camera->getViewProjection());
    const AABB& aabb = renderable.mesh->getAABB();
    if (!frustumIntersectsAABB(frustum, aabb, modelMatrix)) {
        return;  // Culled
    }

    BatchKey key{
        renderable.mesh,
        materialPtr.get()};

    InstanceData data;
    data.modelMatrix = modelMatrix;
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(data.modelMatrix)));
    data.normalMatrix = normalMatrix;

    auto& batch = m_Batches[key];
    batch.instances.push_back(data);

    if (batch.instances.size() >= m_MaxBatchSize) {
        flushBatch(key, batch);
        batch.instances.clear();
    }
}

void Renderer::flushBatch(const BatchKey& key, BatchData& batch) {
    if (batch.instances.empty()) return;

    const auto& state = key.material->getState();
    if (state.blend) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);
    if (state.cull) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    auto shader = key.material->getShaderHandle().get();
    if (!shader) {
        throw std::runtime_error("Material missing shader");
    }
    shader->bind();

    shader->bindUniformBlock("FrameData", 0);

    auto texture = key.material->getBaseColorHandle().get();
    if (texture) {
        texture->bind(0);
        shader->setInt("u_Texture", 0);
        shader->setBool("u_HasTexture", true);
    } else {
        shader->setBool("u_HasTexture", false);
    }

    const auto& params = key.material->getParams();
    shader->setVec4("u_BaseColorFactor", &params.baseColorFactor[0]);
    shader->setFloat("u_AlphaCutoff", params.alphaCutoff);

    // at this point we know how many instances we need to draw for this batch, so we can upload the
    // instance data to the GPU
    key.mesh->updateInstanceBuffer(
        batch.instances.data(),
        batch.instances.size() * sizeof(InstanceData));

    key.mesh->drawInstanced(batch.instances.size());

    m_Stats.drawCalls++;
    m_Stats.triangles += (key.mesh->getIndexCount() / 3) * batch.instances.size();
}

void Renderer::flush() {
    if (!m_Camera) {
        throw std::runtime_error("Renderer error: No camera set for rendering!");
    }

    m_Stats.reset();

    updateFrameUbo();

    // Flush all remaining batches
    for (auto& [key, batch] : m_Batches) {
        if (!batch.instances.empty()) {
            flushBatch(key, batch);
        }
    }

    m_Batches.clear();

    resetGlState();
}

void Renderer::updateFrameUbo() {
    FrameUbo data{};
    data.viewProj = m_Camera->getViewProjection();

    glm::vec3 sunDir = glm::normalize(m_Lights.sunDir);
    data.sunDir = glm::vec4(sunDir, 0.0f);
    data.sunColor = glm::vec4(m_Lights.sunColor, 0.0f);
    data.ambient = glm::vec4(m_Lights.ambientColor, m_Lights.ambientStrength);

    int pointCount = static_cast<int>(m_Lights.pointLights.size());
    pointCount = std::min(pointCount, 4);
    data.lightCounts = glm::vec4(static_cast<float>(pointCount), 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < pointCount; ++i) {
        const auto& light = m_Lights.pointLights[i];
        data.pointLights[i].positionRange = glm::vec4(light.position, light.range);
        data.pointLights[i].colorIntensity = glm::vec4(light.color, light.intensity);
    }

    m_FrameUbo.updateSubData(0, sizeof(FrameUbo), &data);
}

void Renderer::reset() {
    m_Batches.clear();
    m_Stats.reset();
}

void Renderer::toggleWireframe() {
    m_Wireframe = !m_Wireframe;
    applyWireframeState();
}

}  // namespace se::render
