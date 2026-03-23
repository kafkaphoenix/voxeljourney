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
    void addRenderable(const Renderable& r) { m_Renderables.push_back(r); }
    const std::vector<Renderable>& getRenderables() const { return m_Renderables; }

    std::span<const DirectionalLight> getDirectionalLights() const { return m_DirectionalLights; }
    std::span<const PointLight> getPointLights() const { return m_PointLights; }
    std::span<const SpotLight> getSpotLights() const { return m_SpotLights; }
    void addDirectionalLight(const DirectionalLight& l) { m_DirectionalLights.push_back(l); }
    void addPointLight(const PointLight& l) { m_PointLights.push_back(l); }
    void addSpotLight(const SpotLight& l) { m_SpotLights.push_back(l); }

    Sky& getSky() { return m_Sky; }
    const Sky& getSky() const { return m_Sky; }

    LightData getLightData() const {
        LightData d;
        d.sun = m_DirectionalLights.empty() ? nullptr : &m_DirectionalLights[0];
        d.pointLights = m_PointLights;
        d.spotLights = m_SpotLights;
        d.ambientColor = m_Sky.getAmbientColor();
        d.ambientStrength = m_Sky.getAmbientStrength();
        return d;
    }

   private:
    Sky m_Sky;
    std::vector<DirectionalLight> m_DirectionalLights;
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight> m_SpotLights;
    std::vector<Renderable> m_Renderables;
};

}  // namespace se::scene