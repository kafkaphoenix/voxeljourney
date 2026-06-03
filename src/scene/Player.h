#pragma once

#include <glm/glm.hpp>

#include "Camera.h"
#include "CameraController.h"
#include "CharacterController.h"
#include "Transform.h"

namespace se::core {
class Input;
class Config;
}

namespace se::scene {
struct AnimatedInstance;

class Player {
public:
    explicit Player(const se::core::Config& config);
    ~Player() = default;

    void update(float deltaTime, const se::core::Input& input);
    void setBodyInstance(AnimatedInstance* bodyInstance);

    [[nodiscard]] Camera& getCamera() { return m_Camera; }
    [[nodiscard]] const Camera& getCamera() const { return m_Camera; }
    [[nodiscard]] glm::vec3 getPosition() const { return m_Transform.position; }

private:
    Transform m_Transform;
    Camera m_Camera;
    AnimatedInstance* m_BodyInstance = nullptr;
    CharacterController m_CharacterController;
    CameraController m_CameraController;
};

}  // namespace se::scene