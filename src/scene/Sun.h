#pragma once

#include "Light.h"
#include "Transform.h"

namespace se::scene {

class Sun {
   public:
    Sun();

    const Light& getLight() const { return m_Light; }
    Light& getLight() { return m_Light; }

    const Transform& getTransform() const { return m_Transform; }
    Transform& getTransform() { return m_Transform; }

   private:
    Transform m_Transform;
    Light m_Light;
};

}  // namespace se::scene
