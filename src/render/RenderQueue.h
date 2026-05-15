#pragma once

#include <glm/glm.hpp>
#include <span>
#include <unordered_map>
#include <vector>

#include "Mesh.h"
#include "assets/Material.h"
#include "scene/Renderable.h"

namespace se::render {

struct InstanceData {
    glm::mat4 modelMatrix;
    glm::mat3 normalMatrix;
};

struct BatchKey {
    Mesh* mesh = nullptr;
    se::assets::Material* material = nullptr;
    bool operator==(const BatchKey&) const noexcept = default;

    struct Hash {
        size_t operator()(const BatchKey& k) const noexcept {
            const size_t h1 = std::hash<Mesh*>{}(k.mesh);
            const size_t h2 = std::hash<se::assets::Material*>{}(k.material);
            return h1 ^ (h2 << 1);
        }
    };
};

using BatchMap = std::unordered_map<BatchKey, std::vector<InstanceData>, BatchKey::Hash>;

class RenderQueue {
public:
    struct StaticOpaqueBatch {
        BatchKey key{};
        std::span<const InstanceData> batch{};
    };

    struct DrawItem {
        float viewDepth = 0.0f;
        Mesh* mesh = nullptr;
        se::assets::Material* material = nullptr;
        glm::mat4 modelMatrix{};
        glm::mat3 normalMatrix{};
        const se::scene::Animator* animator = nullptr;  // null = static, non-null = animated
    };

    void submit(const se::scene::Renderable& renderable, const glm::mat4& modelMatrix, const glm::mat4& viewMatrix);
    void clear();

    bool isEmpty() const {
        return m_StaticOpaqueBatches.empty() && m_TransparentDrawItems.empty() && m_OpaqueAnimatedDrawItems.empty();
    }

    const std::vector<StaticOpaqueBatch>& getOrderedStaticOpaqueBatches() const;
    const std::vector<DrawItem>& getDepthSortedTransparentDrawItems();
    const std::vector<DrawItem>& getOpaqueAnimatedDrawItems() const { return m_OpaqueAnimatedDrawItems; }

private:
    BatchMap m_StaticOpaqueBatches;
    std::vector<DrawItem> m_TransparentDrawItems;
    std::vector<DrawItem> m_OpaqueAnimatedDrawItems;
    mutable std::vector<StaticOpaqueBatch> m_StaticOpaqueBatchViews;
};

}  // namespace se::render