#include "Player.h"

#include <GLFW/glfw3.h>

#include <glm/gtc/quaternion.hpp>

#include "AnimatedInstance.h"
#include "core/Config.h"
#include "core/Input.h"

namespace se::scene {

namespace {

void applyRootMotion(Transform& transform, const AnimatedInstance& bodyInstance,
                     const CharacterController& characterController, float deltaTime) {
    const glm::vec3 localDelta = bodyInstance.animator.rootMotionDelta();
    const glm::vec3 scaledLocalDelta = localDelta * bodyInstance.transform.scale;
    glm::vec3 worldDelta = glm::mat3_cast(transform.rotation) * scaledLocalDelta;

    const auto& locomotion = characterController.locomotionIntent();
    const glm::vec3& movementDirection = characterController.movementDirection();
    if (locomotion.moving && glm::dot(movementDirection, movementDirection) > 0.0f) {
        const float fallbackSpeed = characterController.movementSpeed(locomotion.running);
        const glm::vec3 fallbackDelta = movementDirection * (fallbackSpeed * deltaTime);
        const float rootDistanceSq = glm::dot(worldDelta, worldDelta);
        const float fallbackDistanceSq = glm::dot(fallbackDelta, fallbackDelta);

        if (rootDistanceSq < fallbackDistanceSq * 0.25f) {
            worldDelta = fallbackDelta;
        }
    }

    transform.position += worldDelta;
}

}  // namespace

Player::Player(const se::core::Config& config)
    : m_Transform{.position = config.player().spawn},
      m_Camera(config.camera()),
      m_AnimationController(),
      m_CharacterController(config.characterController()),
      m_CameraController(config.cameraController()),
      m_RootMotion({.enabled = config.characterController().useRootMotion}) {
    m_Camera.setAspectRatio(static_cast<float>(config.window().width) / static_cast<float>(config.window().height));
}

PlayerIntent Player::sampleIntent(const se::core::Input& input) {
    PlayerIntent intent{};

    if (input.isKeyDown(GLFW_KEY_W)) {
        intent.moveInput.y += 1.0f;
    }
    if (input.isKeyDown(GLFW_KEY_S)) {
        intent.moveInput.y -= 1.0f;
    }
    if (input.isKeyDown(GLFW_KEY_D)) {
        intent.moveInput.x += 1.0f;
    }
    if (input.isKeyDown(GLFW_KEY_A)) {
        intent.moveInput.x -= 1.0f;
    }

    if (glm::length(intent.moveInput) > 1.0f) {
        intent.moveInput = glm::normalize(intent.moveInput);
    }

    intent.lookDelta = glm::vec2{input.getMouseDeltaX(), input.getMouseDeltaY()};
    intent.running = input.isKeyDown(GLFW_KEY_LEFT_SHIFT) || input.isKeyDown(GLFW_KEY_RIGHT_SHIFT);
    intent.toggleCamera = input.isKeyPressed(GLFW_KEY_V);
    return intent;
}

void Player::configureBodyAnimator() const {
    if (!m_BodyInstance) {
        return;
    }

    m_BodyInstance->animator.setRootMotionEnabled(m_RootMotion.enabled);
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
        m_AnimationController.setLocomotionClips(
            AnimationController::LocomotionClips{.idle = "Survey", .walk = "Walk", .run = "Run"});
        configureBodyAnimator();
        syncBodyToPlayer();
    }
}

void Player::updateThirdPerson(float deltaTime, const PlayerIntent& intent) {
    m_CameraController.updateThirdPerson(intent, m_CharacterController.facingYaw(), m_CharacterController.pitch());
    m_CharacterController.updateThirdPerson(deltaTime, intent, m_Transform, m_CameraController.thirdPersonYaw());
}

void Player::updateFirstPerson(float deltaTime, const PlayerIntent& intent) {
    m_CharacterController.updateFirstPerson(deltaTime, intent, m_Transform);
}

void Player::update(float deltaTime, const PlayerIntent& intent) {
    if (intent.toggleCamera) {
        CameraMode newMode = (m_CameraController.getMode() == CameraMode::FirstPerson) ? CameraMode::ThirdPerson
                                                                                       : CameraMode::FirstPerson;
        m_CameraController.setMode(newMode);
    }

    if (m_CameraController.getMode() == CameraMode::ThirdPerson) {
        updateThirdPerson(deltaTime, intent);
    } else {
        updateFirstPerson(deltaTime, intent);
    }

    if (m_BodyInstance) {
        const auto& locomotion = m_CharacterController.locomotionIntent();
        m_AnimationController.setLocomotionIntent(locomotion.moving, locomotion.running);
        m_AnimationController.apply(m_BodyInstance->animator);
    }
}

void Player::finalizeFrame(float deltaTime) {
    if (m_RootMotion.enabled && m_BodyInstance) {
        applyRootMotion(m_Transform, *m_BodyInstance, m_CharacterController, deltaTime);
    }

    m_CameraController.sync(m_Transform, m_CharacterController.yaw(), m_CharacterController.pitch(), m_Camera);
    syncBodyToPlayer();
}

}  // namespace se::scene