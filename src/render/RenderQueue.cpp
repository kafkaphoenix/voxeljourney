#include "RenderQueue.h"

#include <algorithm>
#include <glm/gtc/matrix_inverse.hpp>
#include <stdexcept>

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

void RenderQueue::submit(const ModelSubmission& submission, const glm::mat4& viewMatrix) {
    if (!submission.mesh) {
        throw std::runtime_error("Render submission missing mesh");
    }

    const auto* material = submission.material;
    if (!material) {
        throw std::runtime_error("Render submission missing material");
    }

    const bool isTransparent = material->getState().blend;
    const bool isAnimated = submission.isAnimated();

    const glm::mat4& modelMatrix = submission.modelMatrix;
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

    if (isTransparent) {
        const auto& aabb = submission.mesh->getAABB();
        const glm::vec3 localCenter = 0.5f * (aabb.min + aabb.max);
        const glm::vec4 worldCenter = modelMatrix * glm::vec4(localCenter, 1.0f);
        const float depth = -(viewMatrix * worldCenter).z;

        // Two transparency modes:
        // - Sorted: CPU sorts objects back-to-front; correct for non-intersecting geometry, cheap for few objects.
        // - OIT: order-independent, handles intersecting transparents in one pass, no sorting needed but approximate.
        auto& bucket = material->getState().transparency == se::assets::TransparencyMode::OIT
                           ? m_OITTransparentDrawItems
                           : m_SortedTransparentDrawItems;
        bucket.push_back(DrawItem{
            .viewDepth = depth,
            .mesh = submission.mesh,
            .material = material,
            .modelMatrix = modelMatrix,
            .normalMatrix = normalMatrix,
            .bones = submission.bones,
        });
        return;
    }

    if (isAnimated) {
        m_OpaqueAnimatedDrawItems.push_back(DrawItem{
            .viewDepth = 0.0f,  // not used for opaque, but set to 0 for consistency
            .mesh = submission.mesh,
            .material = material,
            .modelMatrix = modelMatrix,
            .normalMatrix = normalMatrix,
            .bones = submission.bones,
        });
        return;
    }

    const InstanceData data{
        .modelMatrix = modelMatrix,
        .normalMatrix = normalMatrix,
    };

    m_StaticOpaqueBatches[BatchKey{.mesh = submission.mesh, .material = material}].push_back(data);
}

const std::vector<RenderQueue::DrawItem>& RenderQueue::getDepthSortedTransparentDrawItems() {
    std::ranges::sort(m_SortedTransparentDrawItems, std::greater{}, &DrawItem::viewDepth);

    return m_SortedTransparentDrawItems;
}

void RenderQueue::clear() {
    m_StaticOpaqueBatches.clear();
    m_SortedTransparentDrawItems.clear();
    m_OITTransparentDrawItems.clear();
    m_OpaqueAnimatedDrawItems.clear();
    m_StaticOpaqueBatchViews.clear();
}

}  // namespace se::render