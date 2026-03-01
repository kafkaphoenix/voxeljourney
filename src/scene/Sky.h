#pragma once

#include <glm/vec3.hpp>

#include "Sun.h"

namespace se::scene {

class Sky {
   public:
    Sky() = default;

    const Sun& getSun() const { return m_Sun; }
    Sun& getSun() { return m_Sun; }

    glm::vec3 getAmbientColor() const { return m_AmbientColor; }
    float getAmbientStrength() const { return m_AmbientStrength; }

    void setAmbientColor(const glm::vec3& color) { m_AmbientColor = color; }
    void setAmbientStrength(float strength) { m_AmbientStrength = strength; }

   private:
    Sun m_Sun;
    glm::vec3 m_AmbientColor{1.0f, 1.0f, 1.0f};
    float m_AmbientStrength = 0.7f;
};

}  // namespace se::scene
