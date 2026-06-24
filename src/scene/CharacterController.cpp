#include "CharacterController.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

#include "core/Input.h"

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
      m_UseFixedStep(config.useFixedStep),
      m_UseRootMotion(config.useRootMotion),
      m_FixedStep(1.0f / config.fixedHz) {}

void CharacterController::updateFirstPerson(float deltaTime, const se::core::Input& input, Transform& transform) {
    updateMouseLook(input);
    updateMovement(deltaTime, input, transform, m_Yaw);
    updateFacing(deltaTime, true);

    const float bodyYaw = glm::radians(-m_FacingYaw + 90.0f);
    transform.rotation = glm::angleAxis(bodyYaw, glm::vec3(0.0f, 1.0f, 0.0f));
}

void CharacterController::updateThirdPerson(float deltaTime, const se::core::Input& input, Transform& transform,
                                            float cameraYaw) {
    m_SmoothedDx = 0.0f;
    m_SmoothedDy = 0.0f;

    const bool moving = input.isKeyDown(GLFW_KEY_W) || input.isKeyDown(GLFW_KEY_S) || input.isKeyDown(GLFW_KEY_A) ||
                        input.isKeyDown(GLFW_KEY_D);
    const float movementYawAlpha = 1.0f - std::exp(-(m_TurnResponsiveness * 1.5f) * deltaTime);
    if (moving) {
        m_ThirdPersonMovementYaw = blendAngleDegrees(m_ThirdPersonMovementYaw, cameraYaw, movementYawAlpha);
    } else {
        m_ThirdPersonMovementYaw = cameraYaw;
    }

    updateMovement(deltaTime, input, transform, m_ThirdPersonMovementYaw);
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

void CharacterController::updateMouseLook(const se::core::Input& input) {
    const float rawDx = input.getMouseDeltaX();
    const float rawDy = -input.getMouseDeltaY();

    m_SmoothedDx += (rawDx - m_SmoothedDx) * m_MouseSmoothAlpha;
    m_SmoothedDy += (rawDy - m_SmoothedDy) * m_MouseSmoothAlpha;

    m_Yaw += m_SmoothedDx * m_MouseSensitivity;
    m_Pitch += m_SmoothedDy * m_MouseSensitivity;
    m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
}

void CharacterController::updateMovement(float deltaTime, const se::core::Input& input, Transform& transform,
                                         float movementYaw) {
    const float frameDt = std::clamp(deltaTime, 0.0f, 0.05f);

    const bool moving = input.isKeyDown(GLFW_KEY_W) || input.isKeyDown(GLFW_KEY_S) || input.isKeyDown(GLFW_KEY_A) ||
                        input.isKeyDown(GLFW_KEY_D);
    const bool running = input.isKeyDown(GLFW_KEY_LEFT_SHIFT) || input.isKeyDown(GLFW_KEY_RIGHT_SHIFT);
    m_LocomotionIntent = {.moving = moving, .running = running};

    const float yawRad = glm::radians(movementYaw);
    const glm::vec3 forward{std::cos(yawRad), 0.0f, std::sin(yawRad)};
    const glm::vec3 right{-std::sin(yawRad), 0.0f, std::cos(yawRad)};

    m_MoveDirection = glm::vec3{0.0f};
    if (input.isKeyDown(GLFW_KEY_W)) {
        m_MoveDirection += forward;
    }
    if (input.isKeyDown(GLFW_KEY_S)) {
        m_MoveDirection -= forward;
    }
    if (input.isKeyDown(GLFW_KEY_D)) {
        m_MoveDirection += right;
    }
    if (input.isKeyDown(GLFW_KEY_A)) {
        m_MoveDirection -= right;
    }

    if (glm::length(m_MoveDirection) > 0.0f) {
        m_MoveDirection = glm::normalize(m_MoveDirection);
    }

    if (m_UseRootMotion) {
        m_MoveAccumulator = 0.0f;
        return;
    }

    if (!m_UseFixedStep) {
        applyMovementStep(frameDt, m_MoveDirection, running, transform);
        return;
    }

    m_MoveAccumulator += frameDt;
    constexpr int MAX_SUB_STEPS = 4;
    int steps = 0;
    while (m_MoveAccumulator >= m_FixedStep && steps < MAX_SUB_STEPS) {
        applyMovementStep(m_FixedStep, m_MoveDirection, running, transform);
        m_MoveAccumulator -= m_FixedStep;
        ++steps;
    }

    if (m_MoveAccumulator > m_FixedStep * static_cast<float>(MAX_SUB_STEPS)) {
        m_MoveAccumulator = m_FixedStep;
    }
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
