#include "Player.h"

#include <GLFW/glfw3.h>

#include <glm/gtc/quaternion.hpp>

#include "AnimatedInstance.h"
#include "core/Config.h"
#include "core/Input.h"

namespace se::scene {

Player::Player(const se::core::Config& config)
    : m_Transform{.position = config.player().spawn},
      m_Camera(config.camera()),
      m_CharacterController(config.characterController()),
      m_CameraController(config.cameraController()),
      m_RootMotion({
          .enabled = config.characterController().useRootMotion,
          .playbackSpeed = config.characterController().rootMotionPlaybackSpeed,
          .translationMask = config.characterController().rootMotionTranslationMask,
      }) {}

void Player::configureBodyAnimator() const {
    if (!m_BodyInstance) {
        return;
    }

    m_BodyInstance->animator.setRootMotionEnabled(m_RootMotion.enabled);
    m_BodyInstance->animator.setPlaybackSpeed(m_RootMotion.playbackSpeed);
    m_BodyInstance->animator.setRootMotionTranslationMask(m_RootMotion.translationMask);
}

void Player::syncBodyToPlayer() const {
    if (!m_BodyInstance) {
        return;
    }

    m_BodyInstance->transform.position = m_Transform.position;
    m_BodyInstance->transform.rotation = m_Transform.rotation;
}

void Player::setBodyInstance(AnimatedInstance* bodyInstance) {
    m_BodyInstance = bodyInstance;

    if (m_BodyInstance) {
        configureBodyAnimator();
        syncBodyToPlayer();
    }
}

void Player::updateThirdPerson(float deltaTime, const se::core::Input& input) {
    m_CameraController.updateThirdPersonOrbit(input, m_CharacterController.facingYaw(), m_CharacterController.pitch());
    m_CharacterController.updateThirdPerson(deltaTime, input, m_Transform, m_CameraController.thirdPersonYaw());
}

void Player::updateFirstPerson(float deltaTime, const se::core::Input& input) {
    m_CharacterController.updateFirstPerson(deltaTime, input, m_Transform);
}

void Player::update(float deltaTime, const se::core::Input& input) {
    if (input.isKeyPressed(GLFW_KEY_V)) {
        CameraMode newMode = (m_CameraController.getMode() == CameraMode::FirstPerson) ? CameraMode::ThirdPerson
                                                                                       : CameraMode::FirstPerson;
        m_CameraController.setMode(newMode);
    }

    if (m_CameraController.getMode() == CameraMode::ThirdPerson) {
        updateThirdPerson(deltaTime, input);
    } else {
        updateFirstPerson(deltaTime, input);
    }

    if (m_BodyInstance) {
        const auto& locomotion = m_CharacterController.locomotionIntent();
        m_BodyInstance->controller.setLocomotionIntent(locomotion.moving, locomotion.running);
    }
}

void Player::applyRootMotion(float deltaTime) {
    if (!m_RootMotion.enabled || !m_BodyInstance) {
        return;
    }

    const glm::vec3 localDelta = m_BodyInstance->animator.rootMotionDelta() * m_RootMotion.translationMask;
    const glm::vec3 scaledLocalDelta = localDelta * m_BodyInstance->transform.scale;
    glm::vec3 worldDelta = glm::mat3_cast(m_Transform.rotation) * scaledLocalDelta;

    const auto& locomotion = m_CharacterController.locomotionIntent();
    const glm::vec3& movementDirection = m_CharacterController.movementDirection();
    if (locomotion.moving && glm::dot(movementDirection, movementDirection) > 0.0f) {
        const float fallbackSpeed = m_CharacterController.movementSpeed(locomotion.running);
        const glm::vec3 fallbackDelta = movementDirection * (fallbackSpeed * deltaTime);
        const float rootDistanceSq = glm::dot(worldDelta, worldDelta);
        const float fallbackDistanceSq = glm::dot(fallbackDelta, fallbackDelta);

        if (rootDistanceSq < fallbackDistanceSq * 0.25f) {
            worldDelta = fallbackDelta;
        }
    }

    m_Transform.position += worldDelta;
}

void Player::finalizeFrame(float deltaTime) {
    applyRootMotion(deltaTime);

    m_CameraController.sync(m_Transform, m_CharacterController.yaw(), m_CharacterController.pitch(), m_Camera);
    syncBodyToPlayer();
}

}  // namespace se::scene