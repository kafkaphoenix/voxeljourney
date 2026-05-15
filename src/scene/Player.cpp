#include "Player.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <glm/gtc/quaternion.hpp>

#include "AnimatedInstance.h"
#include "core/Config.h"
#include "core/Input.h"

namespace se::scene {

Player::Player(const se::core::Config& config)
    : m_Transform{.position = config.player().startPosition},
      m_Camera(config.camera()),
      m_WalkSpeed(config.player().walkSpeed),
      m_RunSpeed(config.player().runSpeed),
      m_MouseSensitivity(config.player().mouseSensitivity),
      m_CameraHeight(config.player().cameraHeight),
      m_CameraDistance(config.player().cameraDistance),
      m_UseFixedStep(config.player().useFixedStep),
      m_MouseSmoothAlpha(config.input().mouseSmoothAlpha),
      m_FixedStep(1.0f / config.player().fixedHz) {}

void Player::setBodyInstance(AnimatedInstance* bodyInstance) {
    m_BodyInstance = bodyInstance;

    if (!m_BodyInstance) {
        return;
    }

    m_BodyInstance->animator.play(m_Clips.idle);
    m_MoveState = MoveState::Idle;
}

void Player::update(float deltaTime, const se::core::Input& input) {
    updateMouseLook(input);
    updateMovement(deltaTime, input);
    updateMoveState(input);
    syncCameraAndBody();
}

void Player::updateMouseLook(const se::core::Input& input) {
    float rawDx = input.getMouseDeltaX();
    float rawDy = -input.getMouseDeltaY();
    m_SmoothedDx += (rawDx - m_SmoothedDx) * m_MouseSmoothAlpha;
    m_SmoothedDy += (rawDy - m_SmoothedDy) * m_MouseSmoothAlpha;

    m_Yaw += m_SmoothedDx * m_MouseSensitivity;
    m_Pitch += m_SmoothedDy * m_MouseSensitivity;
    m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
}

void Player::updateMovement(float deltaTime, const se::core::Input& input) {
    const float frameDt = std::clamp(deltaTime, 0.0f, 0.05f);

    if (!m_UseFixedStep) {  // for simple movement (non-physics character controllers)
        applyMovementStep(frameDt, input);
    } else {  // deterministic stepping used by physics/gameplay simulation.
        m_MoveAccumulator += frameDt;
        constexpr int kMaxSubSteps = 4;
        int steps = 0;
        while (m_MoveAccumulator >= m_FixedStep && steps < kMaxSubSteps) {
            applyMovementStep(m_FixedStep, input);
            m_MoveAccumulator -= m_FixedStep;
            ++steps;
        }

        // drop excessive accumulated time to prevent spiral of death after long stalls (e.g. breakpoint pause or
        // tabbing out)
        if (m_MoveAccumulator > m_FixedStep * static_cast<float>(kMaxSubSteps)) {
            m_MoveAccumulator = m_FixedStep;
        }
    }
}

void Player::applyMovementStep(float stepSeconds, const se::core::Input& input) {
    float velocity = m_WalkSpeed * stepSeconds;
    if (input.isKeyDown(GLFW_KEY_LEFT_SHIFT) || input.isKeyDown(GLFW_KEY_RIGHT_SHIFT)) {
        velocity = m_RunSpeed * stepSeconds;
    }

    float yawRad = glm::radians(m_Yaw);
    glm::vec3 forward{std::cos(yawRad), 0.0f, std::sin(yawRad)};
    glm::vec3 right{-std::sin(yawRad), 0.0f, std::cos(yawRad)};

    glm::vec3 moveDir{0.0f};

    if (input.isKeyDown(GLFW_KEY_W))
        moveDir += forward;
    if (input.isKeyDown(GLFW_KEY_S))
        moveDir -= forward;
    if (input.isKeyDown(GLFW_KEY_D))
        moveDir += right;
    if (input.isKeyDown(GLFW_KEY_A))
        moveDir -= right;

    if (glm::length(moveDir) > 0.0f)
        moveDir = glm::normalize(moveDir);

    m_Transform.position += moveDir * velocity;
}

void Player::updateMoveState(const se::core::Input& input) {
    if (!m_BodyInstance) {
        return;
    }

    const bool moving = input.isKeyDown(GLFW_KEY_W) || input.isKeyDown(GLFW_KEY_S) || input.isKeyDown(GLFW_KEY_A) ||
                        input.isKeyDown(GLFW_KEY_D);
    const bool running = input.isKeyDown(GLFW_KEY_LEFT_SHIFT) || input.isKeyDown(GLFW_KEY_RIGHT_SHIFT);

    MoveState desired = MoveState::Idle;
    if (moving) {
        desired = running ? MoveState::Running : MoveState::Walking;
    }

    if (desired == m_MoveState) {
        return;
    }

    m_MoveState = desired;
    switch (m_MoveState) {
    case MoveState::Idle: transitionToClip(m_Clips.idle, 0.35f); break;
    case MoveState::Walking: transitionToClip(m_Clips.walk, 0.35f); break;
    case MoveState::Running: transitionToClip(m_Clips.run, 0.35f); break;
    }
}

void Player::syncCameraAndBody() {
    // Sync transform facing from yaw
    float bodyYaw = glm::radians(-m_Yaw + 90.0f);
    m_Transform.rotation = glm::angleAxis(bodyYaw, glm::vec3(0.0f, 1.0f, 0.0f));

    // Camera sits behind and above the player
    float yawRad = glm::radians(m_Yaw);
    glm::vec3 back{-std::cos(yawRad), 0.0f, -std::sin(yawRad)};
    glm::vec3 cameraPos = m_Transform.position + back * m_CameraDistance + glm::vec3(0.0f, m_CameraHeight, 0.0f);

    m_Camera.setPosition(cameraPos);
    m_Camera.setYawPitch(m_Yaw, m_Pitch);

    // Sync position and rotation to the animated body transform
    if (m_BodyInstance) {
        m_BodyInstance->transform.position = m_Transform.position;
        m_BodyInstance->transform.rotation = m_Transform.rotation;
    }
}

void Player::transitionToClip(std::string_view clipName, float blendDuration) {
    if (!m_BodyInstance) {
        return;
    }
    if (blendDuration <= 0.0f) {
        m_BodyInstance->animator.play(clipName);
        return;
    }
    m_BodyInstance->animator.blendTo(clipName, blendDuration);
}

}  // namespace se::scene