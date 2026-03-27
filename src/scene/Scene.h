#pragma once
#include <span>
#include <vector>

#include "Light.h"
#include "LightData.h"
#include "Renderable.h"
#include "Sky.h"

namespace se::scene {

class Scene {
   public:
    Scene() = default;

    void addRenderable(Renderable&& r) { m_Renderables.push_back(std::move(r)); }
    const std::vector<Renderable>& getRenderables() const { return m_Renderables; }

    std::span<const DirectionalLight> getDirectionalLights() const { return m_DirectionalLights; }
    std::span<const PointLight> getPointLights() const { return m_PointLights; }
    std::span<const SpotLight> getSpotLights() const { return m_SpotLights; }
    void addDirectionalLight(const DirectionalLight& l) { m_DirectionalLights.push_back(l); }
    void addPointLight(const PointLight& l) { m_PointLights.push_back(l); }
    void addSpotLight(const SpotLight& l) { m_SpotLights.push_back(l); }

    Sky& getSky() { return m_Sky; }
    const Sky& getSky() const { return m_Sky; }

    // LightData is a zero-copy view into Scene's light vectors.
    // Translation to GPU layout happens once in updateFrameUbo.
    LightData prepareLightData() const {
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
};

}  // namespace se::scene