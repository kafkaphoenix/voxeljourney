#pragma once
#include <glm/vec3.hpp>

namespace se::scene {

class Sky {
   public:
    void setAmbientColor(glm::vec3 color) { m_AmbientColor = color; }
    void setAmbientStrength(float strength) { m_AmbientStrength = strength; }
    glm::vec3 getAmbientColor() const { return m_AmbientColor; }
    float getAmbientStrength() const { return m_AmbientStrength; }

   private:
    glm::vec3 m_AmbientColor{1.0f, 1.0f, 1.0f};
    float m_AmbientStrength = 0.2f;
};

}  // namespace se::scene