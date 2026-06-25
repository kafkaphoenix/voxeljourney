#pragma once

#include <glm/vec3.hpp>

namespace se::scene {

class Sky {
public:
    void setAmbientColor(glm::vec3 color) { m_AmbientColor = color; }
    void setAmbientIntensity(float strength) { m_AmbientIntensity = strength; }
    [[nodiscard]] glm::vec3 getAmbientColor() const { return m_AmbientColor; }
    [[nodiscard]] float getAmbientIntensity() const { return m_AmbientIntensity; }

private:
    glm::vec3 m_AmbientColor{1.0f, 1.0f, 1.0f};
    float m_AmbientIntensity = 0.4f;
};

}  // namespace se::scene