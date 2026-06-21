#include "Player.h"

#include <GLFW/glfw3.h>

#include "AnimatedInstance.h"
#include "core/Config.h"
#include "core/Input.h"

namespace se::scene {

Player::Player(const se::core::Config& config)
    : m_Transform{.position = config.player().spawn},
      m_Camera(config.camera()),
      m_CharacterController(config.characterController()),
      m_CameraController(config.cameraController()) {}

void Player::setBodyInstance(AnimatedInstance* bodyInstance) { m_BodyInstance = bodyInstance; }

void Player::update(float deltaTime, const se::core::Input& input) {
    m_CharacterController.update(deltaTime, input, m_Transform);

    if (input.isKeyPressed(GLFW_KEY_V)) {
        CameraMode newMode = (m_CameraController.getMode() == CameraMode::FirstPerson) ? CameraMode::ThirdPerson
                                                                                       : CameraMode::FirstPerson;
        m_CameraController.setMode(newMode);
    }

    m_CameraController.sync(m_Transform, m_CharacterController.yaw(), m_CharacterController.pitch(), m_Camera);
    if (m_BodyInstance) {
        const auto& locomotion = m_CharacterController.locomotionIntent();
        m_BodyInstance->controller.setLocomotionIntent(locomotion.moving, locomotion.running);
        m_BodyInstance->transform.position = m_Transform.position;
        m_BodyInstance->transform.rotation = m_Transform.rotation;
    }
}

}  // namespace se::scene