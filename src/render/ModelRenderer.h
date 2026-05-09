#pragma once
#include <glad/glad.h>

#include <array>
#include <glm/glm.hpp>
#include <optional>
#include <span>
#include <vector>

#include "Frustum.h"
#include "Mesh.h"
#include "RenderQueue.h"
#include "RenderStats.h"
#include "UniformBuffer.h"
#include "assets/Material.h"
#include "assets/Skeleton.h"
#include "scene/Camera.h"
#include "scene/LightData.h"
#include "scene/Renderable.h"

namespace se::render {

class ModelRenderer {
public:
    ModelRenderer();
    ~ModelRenderer();

    void submit(const se::scene::Renderable& renderable, const Frustum& frustum);
    void submitAnimated(const se::scene::Renderable& renderable, const Frustum& frustum,
                        std::span<const glm::mat4> boneMatrices);
    void flush(const se::scene::LightData& lights, const se::scene::Camera& camera, RenderStats& stats);
    void setWireframe(bool enabled);
    void setBatchSize(size_t maxInstances);

private:
    struct TransparentDraw {
        float distance = 0.0f;
        BatchKey key;
        BatchData* batch = nullptr;
    };

    struct AnimatedDraw {
        Mesh* mesh = nullptr;
        se::assets::Material* material = nullptr;
        glm::mat4 modelMatrix{1.0f};
        glm::mat3 normalMatrix{1.0f};
        std::vector<glm::mat4> boneMatrices;
    };

    void setupFrameUbo();
    void setupBoneUbo();
    void setupDefaultSampler();
    void setupDefaultTextures();
    void bindMaterialTextures(const se::assets::MaterialTextures& textures) const;
    void flushBatch(const BatchKey& key, BatchData& batch, RenderStats& stats) const;
    void flushAnimatedDraws(RenderStats& stats) const;
    void updateFrameUbo(const se::scene::LightData& lights, const se::scene::Camera& camera);

    std::vector<TransparentDraw> getSortedTransparentDraws(const se::scene::Camera& camera);

    RenderQueue m_Queue;
    std::vector<AnimatedDraw> m_AnimatedDraws;
    size_t m_MaxBatchSize = 1000;
    std::optional<UniformBuffer> m_FrameUbo;
    std::optional<UniformBuffer> m_BoneUbo;
    GLuint m_DefaultSampler = 0;

    // 1x1 neutral default textures for missing material slots
    // [0] = white (base color, occlusion)  [1] = flat normal  [2] = black (metallic-roughness, emissive)
    std::array<GLuint, 3> m_DefaultTextures{};

    bool m_Wireframe = false;
};

}  // namespace se::render