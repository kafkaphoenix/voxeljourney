#pragma once
#include <glad/glad.h>

#include <array>
#include <glm/glm.hpp>
#include <optional>

#include "Frustum.h"
#include "RenderQueue.h"
#include "RenderStats.h"
#include "UniformBuffer.h"
#include "scene/Camera.h"
#include "scene/LightData.h"

namespace se::render {

class ModelRenderer {
public:
    explicit ModelRenderer(float anisotropy);
    ~ModelRenderer();

    void submit(const se::scene::Renderable& renderable, const Frustum& frustum, const glm::mat4& viewMatrix);
    void flush(const se::scene::LightData& lights, const se::scene::Camera& camera, RenderStats& stats);
    void setWireframe(bool enabled);
    void setBatchSize(size_t maxInstances);

private:
    void resetStateCache();
    void setBlendEnabled(bool enabled) const;
    void setDepthMask(bool writable) const;
    void setCullEnabled(bool enabled) const;
    void setPolygonMode(GLenum mode) const;

    void setupFrameUbo();
    void setupBoneUbo();
    void setupDefaultSampler();
    void setupDefaultTextures();
    void restoreRenderState() const;
    void drawOpaquePass(RenderStats& stats) const;
    void drawTransparentPass(RenderStats& stats);
    void bindMaterialTextures(const se::assets::MaterialTextures& textures) const;
    void flushBatch(const BatchKey& key, std::span<const InstanceData> batch, RenderStats& stats) const;
    void drawAnimatedDrawItem(const RenderQueue::DrawItem& drawItem, RenderStats& stats) const;
    void updateFrameUbo(const se::scene::LightData& lights, const se::scene::Camera& camera);

    RenderQueue m_Queue;
    size_t m_MaxBatchSize = 1000;
    std::optional<UniformBuffer> m_FrameUbo;
    std::optional<UniformBuffer> m_BoneUbo;
    GLuint m_DefaultSampler = 0;

    // 1x1 neutral default textures for missing material slots
    // [0] = white (base color, occlusion)  [1] = flat normal  [2] = black (metallic-roughness, emissive)
    std::array<GLuint, 3> m_DefaultTextures{};
    float m_Anisotropy = 4.0f;

    mutable std::optional<bool> m_CachedBlendEnabled;
    mutable std::optional<bool> m_CachedDepthMaskWritable;
    mutable std::optional<bool> m_CachedCullEnabled;
    mutable std::optional<GLenum> m_CachedPolygonMode;

    bool m_Wireframe = false;
};

}  // namespace se::render