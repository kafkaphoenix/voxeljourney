#pragma once
#include <span>
#include <vector>

#include "AnimatedActor.h"
#include "ChunkRenderable.h"
#include "Light.h"
#include "LightData.h"
#include "Renderable.h"
#include "Sky.h"
#include "voxel/VoxelHash.h"

namespace se::scene {

class Scene {
public:
    Scene() = default;

    void addRenderable(Renderable r) { m_Renderables.push_back(r); }
    [[nodiscard]] const std::vector<Renderable>& getRenderables() const { return m_Renderables; }
    void updateChunkRenderable(ChunkRenderable r) { m_ChunkRenderables[r.position] = r; }
    void removeChunkRenderable(const glm::ivec3& pos) { m_ChunkRenderables.erase(pos); }
    [[nodiscard]] const std::unordered_map<glm::ivec3, ChunkRenderable, se::voxel::IVec3Hash>& getChunkRenderables()
        const {
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

    AnimatedActor& addAnimatedActor(AnimatedActor actor) {
        return *m_AnimatedActors.emplace_back(std::make_unique<AnimatedActor>(std::move(actor)));
    }

    [[nodiscard]] AnimatedActor* findActor(std::string_view tag) {
        for (auto& a : m_AnimatedActors)
            if (a->getTag() == tag)
                return a.get();
        return nullptr;
    }

    [[nodiscard]] const auto& getAnimatedActors() const { return m_AnimatedActors; }

    void update(float deltaTime) {
        for (auto& a : m_AnimatedActors) a->update(deltaTime);
    }

    // LightData is a zero-copy view into Scene's light vectors.
    // Translation to GPU layout happens once in updateFrameUbo.
    [[nodiscard]] LightData prepareLightData() const {
        return LightData{
            .directionalLights = m_DirectionalLights,
            .pointLights = m_PointLights,
            .spotLights = m_SpotLights,
            .ambientColor = m_Sky.getAmbientColor(),
            .ambientStrength = m_Sky.getAmbientStrength(),
        };
    }

private:
    Sky m_Sky;
    std::vector<DirectionalLight> m_DirectionalLights;
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight> m_SpotLights;
    std::vector<Renderable> m_Renderables;
    std::unordered_map<glm::ivec3, ChunkRenderable, se::voxel::IVec3Hash> m_ChunkRenderables;
    std::vector<std::unique_ptr<AnimatedActor>> m_AnimatedActors;
};

}  // namespace se::scene