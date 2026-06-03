#include "TerrainRenderer.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

#include "assets/AssetManager.h"
#include "render/UboDefinitions.h"
#include "scene/Camera.h"
#include "scene/ChunkRenderable.h"
#include "scene/Light.h"
#include "scene/LightData.h"

namespace se::render {

TerrainRenderer::TerrainRenderer() { m_TerrainUbo.emplace(sizeof(TerrainUbo), UboBinding::Terrain); }

void TerrainRenderer::submit(const se::scene::ChunkRenderable& chunkRenderable, const Frustum& frustum) {
    if (!chunkRenderable.mesh) {
        return;
    }
    if (!frustumIntersectsAABB(frustum, chunkRenderable.mesh->getAABB())) {
        return;
    }
    m_DrawList.push_back(ChunkDraw{.mesh = chunkRenderable.mesh.get()});
}

void TerrainRenderer::flush(const se::scene::LightData& lights, const se::scene::Camera& camera, RenderStats& stats) {
    if (m_DrawList.empty()) {
        return;
    }

    m_Shader.get()->bind();
    updateUbo(lights, camera);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, m_Wireframe ? GL_LINE : GL_FILL);

    for (const auto& draw : m_DrawList) {
        draw.mesh->draw();
        stats.chunksDrawCalls++;
        stats.chunksTriangles += static_cast<unsigned int>(draw.mesh->getIndexCount() / 3);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);

    m_DrawList.clear();
}

void TerrainRenderer::updateUbo(const se::scene::LightData& lights, const se::scene::Camera& camera) {
    TerrainUbo data{};

    data.viewProj = camera.getViewProjection();

    if (!lights.directionalLights.empty()) {
        const auto& sun = lights.directionalLights[0];
        data.sunDir = glm::vec4(glm::normalize(sun.direction), 0.0f);
        data.sunColor = glm::vec4(sun.color, sun.intensity);
    }

    data.ambient = glm::vec4(lights.ambientColor, lights.ambientIntensity);

    m_TerrainUbo->updateSubData(0, data);
}

}  // namespace se::render