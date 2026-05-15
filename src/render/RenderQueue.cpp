#include "RenderQueue.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <stdexcept>

#include "scene/Animator.h"

namespace se::render {

const std::vector<RenderQueue::StaticOpaqueBatch>& RenderQueue::getOrderedStaticOpaqueBatches() const {
    m_StaticOpaqueBatchViews.clear();
    m_StaticOpaqueBatchViews.reserve(m_StaticOpaqueBatches.size());

    for (const auto& [key, batch] : m_StaticOpaqueBatches) {
        m_StaticOpaqueBatchViews.push_back({
            .key = key,
            .batch = std::span<const InstanceData>(batch),
        });
    }

    // Sort batches by material first, then mesh, to minimize state changes during rendering.
    std::ranges::sort(m_StaticOpaqueBatchViews, [](const StaticOpaqueBatch& a, const StaticOpaqueBatch& b) {
        if (a.key.material != b.key.material) {
            return a.key.material < b.key.material;
        }
        return a.key.mesh < b.key.mesh;
    });

    return m_StaticOpaqueBatchViews;
}

void RenderQueue::submit(const se::scene::Renderable& renderable, const glm::mat4& modelMatrix,
                         const glm::mat4& viewMatrix) {
    if (!renderable.mesh)
        throw std::runtime_error("Renderable missing mesh");

    auto material = renderable.material.get();
    if (!material)
        throw std::runtime_error("Renderable missing material");

    const bool isTransparent = material->getState().blend;
    const se::scene::Animator* animator = renderable.resolvedAnimator();
    const bool isAnimated = animator != nullptr;

    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

    if (isTransparent) {
        // it won't work for overlapping transparent objects, but good enough for typical alpha-tested foliage
        const auto& aabb = renderable.mesh->getAABB();
        const glm::vec3 localCenter = 0.5f * (aabb.min + aabb.max);
        const glm::vec4 worldCenter = modelMatrix * glm::vec4(localCenter, 1.0f);
        const float depth = -(viewMatrix * worldCenter).z;
        m_TransparentDrawItems.push_back({
            .viewDepth = depth,
            .mesh = renderable.mesh,
            .material = material.get(),
            .modelMatrix = modelMatrix,
            .normalMatrix = normalMatrix,
            .animator = animator,
        });
        return;
    }

    if (isAnimated) {
        m_OpaqueAnimatedDrawItems.push_back({
            .viewDepth = 0.0f,  // not used for opaque, but set to 0 for consistency
            .mesh = renderable.mesh,
            .material = material.get(),
            .modelMatrix = modelMatrix,
            .normalMatrix = normalMatrix,
            .animator = animator,
        });
        return;
    }

    const InstanceData data{
        .modelMatrix = modelMatrix,
        .normalMatrix = normalMatrix,
    };

    m_StaticOpaqueBatches[{renderable.mesh, material.get()}].push_back(data);
}

const std::vector<RenderQueue::DrawItem>& RenderQueue::getDepthSortedTransparentDrawItems() {
    std::ranges::sort(m_TransparentDrawItems, std::greater{}, &DrawItem::viewDepth);

    return m_TransparentDrawItems;
}

void RenderQueue::clear() {
    m_StaticOpaqueBatches.clear();
    m_TransparentDrawItems.clear();
    m_OpaqueAnimatedDrawItems.clear();
    m_StaticOpaqueBatchViews.clear();
}

}  // namespace se::render