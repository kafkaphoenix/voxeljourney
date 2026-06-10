#include "TerrainRenderer.h"

#include <glad/glad.h>

#include "UboDefinitions.h"
#include "assets/Shader.h"

namespace se::render {

TerrainRenderer::TerrainRenderer() { m_TerrainUbo.emplace(sizeof(TerrainUbo), UboBinding::Terrain); }

void TerrainRenderer::submit(const TerrainSubmission& submission, const Frustum& frustum) {
    if (submission.mesh == nullptr) {
        return;
    }

    if (!frustumIntersectsAABB(frustum, submission.mesh->getAABB())) {
        return;
    }
    m_Meshes.push_back(submission.mesh);
}

void TerrainRenderer::flush(const FrameLightData& lights, const FrameCameraData& camera, RenderStats& stats) {
    if (m_Meshes.empty()) {
        return;
    }

    m_Shader.get()->bind();
    updateUbo(lights, camera);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, m_Wireframe ? GL_LINE : GL_FILL);

    for (const auto* mesh : m_Meshes) {
        mesh->draw();
        stats.chunksDrawCalls++;
        stats.chunksTriangles += static_cast<unsigned int>(mesh->getIndexCount() / 3);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);

    m_Meshes.clear();
}

void TerrainRenderer::updateUbo(const FrameLightData& lights, const FrameCameraData& camera) {
    TerrainUbo data{};

    data.viewProj = camera.projectionMatrix * camera.viewMatrix;

    if (!lights.directionalLights().empty()) {
        const auto& sun = lights.directionalLights().front();
        data.sunDir = glm::vec4(glm::normalize(sun.direction), 0.0f);
        data.sunColor = glm::vec4(sun.color, sun.intensity);
    }

    data.ambient = glm::vec4(lights.ambientColor, lights.ambientIntensity);

    m_TerrainUbo->updateSubData(0, data);
}

}  // namespace se::render