#include "CharacterController.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace se::scene {

namespace {

float wrapDegrees(float degrees) {
    float wrapped = std::fmod(degrees + 180.0f, 360.0f);
    if (wrapped < 0.0f) {
        wrapped += 360.0f;
    }

    return wrapped - 180.0f;
}

float shortestAngleDelta(float fromDegrees, float toDegrees) { return wrapDegrees(toDegrees - fromDegrees); }

float blendAngleDegrees(float currentDegrees, float targetDegrees, float blendAlpha) {
    const float delta = shortestAngleDelta(currentDegrees, targetDegrees);
    return wrapDegrees(currentDegrees + delta * glm::clamp(blendAlpha, 0.0f, 1.0f));
}

}  // namespace

CharacterController::CharacterController(const se::core::config::CharacterController& config)
    : m_TurnResponsiveness(config.turnResponsiveness),
      m_WalkSpeed(config.walkSpeed),
      m_RunSpeed(config.runSpeed),
      m_MouseSensitivity(config.mouseSensitivity),
      m_MouseSmoothAlpha(config.mouseSmoothAlpha),
      m_UseRootMotion(config.useRootMotion) {}

void CharacterController::updateFirstPerson(float deltaTime, const PlayerIntent& intent, Transform& transform) {
    updateMouseLook(intent);
    updateMovement(deltaTime, intent, transform, m_Yaw);
    updateFacing(deltaTime, true);

    const float bodyYaw = glm::radians(-m_FacingYaw + 90.0f);
    transform.rotation = glm::angleAxis(bodyYaw, glm::vec3(0.0f, 1.0f, 0.0f));
}

void CharacterController::updateThirdPerson(float deltaTime, const PlayerIntent& intent, Transform& transform,
                                            float cameraYaw) {
    m_SmoothedDx = 0.0f;
    m_SmoothedDy = 0.0f;

    const bool moving = glm::length(intent.moveInput) > 0.0f;
    const float movementYawAlpha = 1.0f - std::exp(-(m_TurnResponsiveness * 1.5f) * deltaTime);
    if (moving) {
        m_ThirdPersonMovementYaw = blendAngleDegrees(m_ThirdPersonMovementYaw, cameraYaw, movementYawAlpha);
    } else {
        m_ThirdPersonMovementYaw = cameraYaw;
    }

    updateMovement(deltaTime, intent, transform, m_ThirdPersonMovementYaw);
    updateFacing(deltaTime, false);

    const float bodyYaw = glm::radians(-m_FacingYaw + 90.0f);
    transform.rotation = glm::angleAxis(bodyYaw, glm::vec3(0.0f, 1.0f, 0.0f));
}

void CharacterController::updateFacing(float deltaTime, bool alignFacingToViewWhenIdle) {
    if (glm::length(m_MoveDirection) > 0.0f) {
        const float targetFacingYaw = glm::degrees(std::atan2(m_MoveDirection.z, m_MoveDirection.x));
        const float turnAlpha = 1.0f - std::exp(-m_TurnResponsiveness * deltaTime);
        m_FacingYaw = blendAngleDegrees(m_FacingYaw, targetFacingYaw, turnAlpha);
    } else if (alignFacingToViewWhenIdle) {
        m_FacingYaw = m_Yaw;
    }
}

void CharacterController::updateMouseLook(const PlayerIntent& intent) {
    const float rawDx = intent.lookDelta.x;
    const float rawDy = -intent.lookDelta.y;

    m_SmoothedDx += (rawDx - m_SmoothedDx) * m_MouseSmoothAlpha;
    m_SmoothedDy += (rawDy - m_SmoothedDy) * m_MouseSmoothAlpha;

    m_Yaw += m_SmoothedDx * m_MouseSensitivity;
    m_Pitch += m_SmoothedDy * m_MouseSensitivity;
    m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
}

void CharacterController::updateMovement(float deltaTime, const PlayerIntent& intent, Transform& transform,
                                         float movementYaw) {
    const float stepDt = std::clamp(deltaTime, 0.0f, 0.05f);

    const bool moving = glm::length(intent.moveInput) > 0.0f;
    const bool running = intent.running;
    m_LocomotionIntent = {.moving = moving, .running = running};

    const float yawRad = glm::radians(movementYaw);
    const glm::vec3 forward{std::cos(yawRad), 0.0f, std::sin(yawRad)};
    const glm::vec3 right{-std::sin(yawRad), 0.0f, std::cos(yawRad)};

    m_MoveDirection = glm::vec3{0.0f};
    m_MoveDirection += forward * intent.moveInput.y;
    m_MoveDirection += right * intent.moveInput.x;

    if (glm::length(m_MoveDirection) > 0.0f) {
        m_MoveDirection = glm::normalize(m_MoveDirection);
    }

    if (m_UseRootMotion) {
        return;
    }

    applyMovementStep(stepDt, m_MoveDirection, running, transform);
}

void CharacterController::applyMovementStep(float stepSeconds, const glm::vec3& moveDirection, bool running,
                                            Transform& transform) const {
    float velocity = m_WalkSpeed * stepSeconds;
    if (running) {
        velocity = m_RunSpeed * stepSeconds;
    }

    transform.position += moveDirection * velocity;
}

}  // namespace se::scene
