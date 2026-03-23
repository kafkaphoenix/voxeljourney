#include "RenderQueue.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <stdexcept>

#include "Frustum.h"

namespace se::render {

void RenderQueue::submit(const se::scene::Renderable& renderable, const Frustum& frustum) {
    if (!renderable.mesh)
        throw std::runtime_error("Renderable missing mesh");

    const auto materialPtr = renderable.material.get();
    if (!materialPtr)
        throw std::runtime_error("Renderable missing material");

    const glm::mat4 modelMatrix = renderable.transform.getMatrix();

    if (!frustumIntersectsAABB(frustum, renderable.mesh->getAABB(), modelMatrix))
        return;

    const BatchKey key{renderable.mesh, materialPtr.get()};
    const InstanceData data{
        .modelMatrix = modelMatrix,
        .normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix))),
    };

    auto& map = materialPtr->getState().blend ? m_TransparentBatches : m_OpaqueBatches;
    auto& batch = map[key];
    batch.instances.push_back(data);
    batch.centerSum += glm::vec3(modelMatrix[3]);
}

void RenderQueue::clear() {
    for (auto& [key, batch] : m_OpaqueBatches) {
        batch.instances.clear();
        batch.centerSum = {};
    }
    for (auto& [key, batch] : m_TransparentBatches) {
        batch.instances.clear();
        batch.centerSum = {};
    }
}

}  // namespace se::render