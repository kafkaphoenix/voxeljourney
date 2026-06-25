#pragma once

#include "Light.h"
#include "Transform.h"

namespace se::scene {

class Sun {
public:
    Sun() = default;
    [[nodiscard]] const DirectionalLight& getLight() const { return m_Light; }
    [[nodiscard]] DirectionalLight& getLight() { return m_Light; }

private:
    DirectionalLight m_Light;
    Transform m_Transform;
};

}  // namespace se::scene