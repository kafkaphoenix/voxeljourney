#pragma once

#include <memory>
#include <span>
#include <vector>

#include "AnimatedInstance.h"
#include "ChunkRenderable.h"
#include "Light.h"
#include "LightData.h"
#include "Renderable.h"
#include "Sky.h"
#include "voxel/IVec3Hash.h"

namespace se::scene {

class Scene {
public:
    Scene() = default;

    void addRenderable(Renderable r) { m_Renderables.push_back(r); }
    [[nodiscard]] const std::vector<Renderable>& getRenderables() const { return m_Renderables; }
    void updateChunkRenderable(ChunkRenderable r) { m_ChunkRenderables[r.position] = std::move(r); }
    void removeChunkRenderable(const glm::ivec3& pos) { m_ChunkRenderables.erase(pos); }
    [[nodiscard]] const std::unordered_map<glm::ivec3, ChunkRenderable>& getChunkRenderables() const {
        return m_ChunkRenderables;
    }

    [[nodiscard]] std::span<const DirectionalLight> getDirectionalLights() const { return m_DirectionalLights; }
    [[nodiscard]] std::span<const PointLight> getPointLights() const { return m_PointLights; }
    [[nodiscard]] std::span<const SpotLight> getSpotLights() const { return m_SpotLights; }
    void addDirectionalLight(const DirectionalLight& l) { m_DirectionalLights.push_back(l); }
    void addPointLight(const PointLight& l) { m_PointLights.push_back(l); }
    void addSpotLight(const SpotLight& l) { m_SpotLights.push_back(l); }

    [[nodiscard]] Sky& getSky() { return m_Sky; }
    [[nodiscard]] const Sky& getSky() const { return m_Sky; }

    AnimatedInstance& addAnimatedInstance(AnimatedInstance instance) {
        return *m_AnimatedInstances.emplace_back(std::make_unique<AnimatedInstance>(std::move(instance)));
    }

    [[nodiscard]] AnimatedInstance* findAnimatedInstance(std::string_view tag) {
        for (auto& a : m_AnimatedInstances) {
            if (a->tag == tag) {
                return a.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const auto& getAnimatedInstances() const { return m_AnimatedInstances; }

    void update(float deltaTime) {
        for (auto& a : m_AnimatedInstances) {
            a->controller.apply(a->animator);
            a->animator.update(deltaTime);
        }
    }

    // LightData is a zero-copy view into Scene's light vectors.
    // Translation to GPU layout happens once in updateFrameUbo.
    [[nodiscard]] LightData prepareLightData() const {
        return LightData{
            .directionalLights = m_DirectionalLights,
            .pointLights = m_PointLights,
            .spotLights = m_SpotLights,
            .ambientColor = m_Sky.getAmbientColor(),
            .ambientIntensity = m_Sky.getAmbientIntensity(),
        };
    }

private:
    Sky m_Sky;
    std::vector<DirectionalLight> m_DirectionalLights;
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight> m_SpotLights;
    std::vector<Renderable> m_Renderables;
    std::unordered_map<glm::ivec3, ChunkRenderable> m_ChunkRenderables;
    std::vector<std::unique_ptr<AnimatedInstance>> m_AnimatedInstances;
};

}  // namespace se::scene