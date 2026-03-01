#include "Sun.h"

namespace se::scene {

Sun::Sun() {
    m_Transform.position = {0.0f, 50.0f, 0.0f};
    m_Light.type = LightType::Directional;
    m_Light.direction = {-0.3f, -1.0f, -0.2f};
    m_Light.intensity = 1.2f;
}

}  // namespace se::scene
