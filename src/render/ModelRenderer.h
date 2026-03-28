#pragma once
#include <glm/glm.hpp>
#include <optional>
#include <vector>

#include "Frustum.h"
#include "Mesh.h"
#include "RenderQueue.h"
#include "RenderStats.h"
#include "UniformBuffer.h"
#include "scene/Camera.h"
#include "scene/LightData.h"
#include "scene/Renderable.h"

namespace se::render {

class ModelRenderer {
public:
    ModelRenderer();

    void submit(const se::scene::Renderable& renderable, const Frustum& frustum);
    void flush(const se::scene::LightData& lights, const se::scene::Camera& camera, RenderStats& stats);
    void setWireframe(bool enabled);
    void setBatchSize(size_t maxInstances);

private:
    struct TransparentDraw {
        float distance = 0.0f;
        BatchKey key;
        BatchData* batch = nullptr;
    };

    void setupFrameUbo();
    static void flushBatch(const BatchKey& key, BatchData& batch, RenderStats& stats);
    void updateFrameUbo(const se::scene::LightData& lights, const se::scene::Camera& camera);

    std::vector<TransparentDraw> getSortedTransparentDraws(const se::scene::Camera& camera);

    RenderQueue m_Queue;
    size_t m_MaxBatchSize = 1000;
    std::optional<UniformBuffer> m_FrameUbo;
    bool m_Wireframe = false;
};

}  // namespace se::render