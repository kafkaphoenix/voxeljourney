#include "CharacterController.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

#include "core/Input.h"

namespace se::scene {

CharacterController::CharacterController(const se::core::config::CharacterController& config)
    : m_WalkSpeed(config.walkSpeed),
      m_RunSpeed(config.runSpeed),
      m_MouseSensitivity(config.mouseSensitivity),
      m_MouseSmoothAlpha(config.mouseSmoothAlpha),
      m_UseFixedStep(config.useFixedStep),
      m_FixedStep(1.0f / config.fixedHz) {}

void CharacterController::update(float deltaTime, const se::core::Input& input, Transform& transform) {
    updateMouseLook(input);
    updateMovement(deltaTime, input, transform);

    // Keep facing aligned with look yaw so body and camera logic share a single orientation source.
    const float bodyYaw = glm::radians(-m_Yaw + 90.0f);
    transform.rotation = glm::angleAxis(bodyYaw, glm::vec3(0.0f, 1.0f, 0.0f));
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

void CharacterController::updateMovement(float deltaTime, const se::core::Input& input, Transform& transform) {
    const float frameDt = std::clamp(deltaTime, 0.0f, 0.05f);

    const bool moving = input.isKeyDown(GLFW_KEY_W) || input.isKeyDown(GLFW_KEY_S) || input.isKeyDown(GLFW_KEY_A) ||
                        input.isKeyDown(GLFW_KEY_D);
    const bool running = input.isKeyDown(GLFW_KEY_LEFT_SHIFT) || input.isKeyDown(GLFW_KEY_RIGHT_SHIFT);
    m_LocomotionIntent = {.moving = moving, .running = running};

    if (!m_UseFixedStep) {
        applyMovementStep(frameDt, input, transform);
        return;
    }

    m_MoveAccumulator += frameDt;
    constexpr int MAX_SUB_STEPS = 4;
    int steps = 0;
    while (m_MoveAccumulator >= m_FixedStep && steps < MAX_SUB_STEPS) {
        applyMovementStep(m_FixedStep, input, transform);
        m_MoveAccumulator -= m_FixedStep;
        ++steps;
    }

    if (m_MoveAccumulator > m_FixedStep * static_cast<float>(MAX_SUB_STEPS)) {
        m_MoveAccumulator = m_FixedStep;
    }
}

void CharacterController::applyMovementStep(float stepSeconds, const se::core::Input& input,
                                            Transform& transform) const {
    float velocity = m_WalkSpeed * stepSeconds;
    if (input.isKeyDown(GLFW_KEY_LEFT_SHIFT) || input.isKeyDown(GLFW_KEY_RIGHT_SHIFT)) {
        velocity = m_RunSpeed * stepSeconds;
    }

    const float yawRad = glm::radians(m_Yaw);
    const glm::vec3 forward{std::cos(yawRad), 0.0f, std::sin(yawRad)};
    const glm::vec3 right{-std::sin(yawRad), 0.0f, std::cos(yawRad)};

    glm::vec3 moveDir{0.0f};
    if (input.isKeyDown(GLFW_KEY_W)) {
        moveDir += forward;
    }
    if (input.isKeyDown(GLFW_KEY_S)) {
        moveDir -= forward;
    }
    if (input.isKeyDown(GLFW_KEY_D)) {
        moveDir += right;
    }
    if (input.isKeyDown(GLFW_KEY_A)) {
        moveDir -= right;
    }

    // Normalize diagonal movement to prevent faster speed.
    if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir);
    }

    transform.position += moveDir * velocity;
}

}  // namespace se::scene
