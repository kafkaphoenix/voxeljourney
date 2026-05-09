#include "Player.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

#include "AnimatedActor.h"
#include "core/Config.h"
#include "core/Input.h"

namespace se::scene {

Player::Player(const se::core::Config& config)
    : m_Transform{.position = config.player().startPosition},
      m_Camera(config.camera()),
      m_MoveSpeed(config.player().moveSpeed),
      m_Sensitivity(config.player().sensitivity),
      m_CameraHeight(config.player().cameraHeight),
      m_CameraDistance(config.player().cameraDistance),
      m_MouseSmoothAlpha(config.input().mouseSmoothAlpha),
      m_FixedStep(config.player().fixedStep) {}

void Player::setBodyActor(AnimatedActor* actor) {
    m_BodyActor = actor;
    if (m_BodyActor) {
        m_BodyActor->playClip(m_Clips.idle);
        m_MoveState = MoveState::Idle;
    }
}

void Player::update(float deltaTime, const se::core::Input& input) {
    updateMouseLook(input);
    updateMovement(deltaTime, input);
    updateMoveState(input);
    syncComponents();
}

void Player::updateMouseLook(const se::core::Input& input) {
    float rawDx = input.getMouseDeltaX();
    float rawDy = -input.getMouseDeltaY();
    m_SmoothedDx += (rawDx - m_SmoothedDx) * m_MouseSmoothAlpha;
    m_SmoothedDy += (rawDy - m_SmoothedDy) * m_MouseSmoothAlpha;

    m_Yaw += m_SmoothedDx * m_Sensitivity;
    m_Pitch += m_SmoothedDy * m_Sensitivity;
    m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
}

void Player::updateMovement(float deltaTime, const se::core::Input& input) {
    m_MoveAccumulator += deltaTime;
    int steps = 0;
    while (m_MoveAccumulator >= m_FixedStep && steps < 4) {
        applyMovementStep(m_FixedStep, input);
        m_MoveAccumulator -= m_FixedStep;
        steps++;
    }

    if (steps == 0) {
        applyMovementStep(deltaTime, input);
        m_MoveAccumulator = 0.0f;
    }
}

void Player::applyMovementStep(float stepSeconds, const se::core::Input& input) {
    float velocity = m_MoveSpeed * stepSeconds;

    float yawRad = glm::radians(m_Yaw);
    glm::vec3 forward{std::cos(yawRad), 0.0f, std::sin(yawRad)};
    glm::vec3 right{-std::sin(yawRad), 0.0f, std::cos(yawRad)};

    if (input.isKeyDown(GLFW_KEY_W))
        m_Transform.position += forward * velocity;
    if (input.isKeyDown(GLFW_KEY_S))
        m_Transform.position -= forward * velocity;
    if (input.isKeyDown(GLFW_KEY_A))
        m_Transform.position -= right * velocity;
    if (input.isKeyDown(GLFW_KEY_D))
        m_Transform.position += right * velocity;
    if (input.isKeyDown(GLFW_KEY_SPACE))
        m_Transform.position.y += velocity;
    if (input.isKeyDown(GLFW_KEY_LEFT_CONTROL))
        m_Transform.position.y -= velocity;
}

void Player::updateMoveState(const se::core::Input& input) {
    if (!m_BodyActor)
        return;

    bool moving = input.isKeyDown(GLFW_KEY_W) || input.isKeyDown(GLFW_KEY_S) || input.isKeyDown(GLFW_KEY_A) ||
                  input.isKeyDown(GLFW_KEY_D);

    if (moving && m_MoveState == MoveState::Idle) {
        m_MoveState = MoveState::Walking;
        m_BodyActor->playClip(m_Clips.walk, 0.35f);
    } else if (!moving && m_MoveState == MoveState::Walking) {
        m_MoveState = MoveState::Idle;
        m_BodyActor->playClip(m_Clips.idle, 0.35f);
    }
}

void Player::syncComponents() {
    // Sync transform facing from yaw
    float bodyYaw = glm::radians(-m_Yaw + 90.0f);
    m_Transform.rotation = glm::angleAxis(bodyYaw, glm::vec3(0.0f, 1.0f, 0.0f));

    // Camera sits behind and above the player
    float yawRad = glm::radians(m_Yaw);
    glm::vec3 back{-std::cos(yawRad), 0.0f, -std::sin(yawRad)};
    glm::vec3 cameraPos = m_Transform.position + back * m_CameraDistance + glm::vec3(0.0f, m_CameraHeight, 0.0f);

    m_Camera.setPosition(cameraPos);
    m_Camera.setYawPitch(m_Yaw, m_Pitch);

    // Sync position and rotation to body actor
    if (m_BodyActor) {
        auto& bodyTransform = m_BodyActor->getTransform();
        bodyTransform.position = m_Transform.position;
        bodyTransform.rotation = m_Transform.rotation;
    }
}

}  // namespace se::scene