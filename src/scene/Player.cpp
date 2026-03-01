#include "Player.h"

#include <GLFW/glfw3.h>

#include <algorithm>

#include "core/Input.h"

namespace se::scene {

void Player::setMouseSmoothing(float alpha) {
    m_MouseSmoothAlpha = std::clamp(alpha, 0.0f, 1.0f);
}

void Player::setFixedStep(float stepSeconds) {
    if (stepSeconds > 0.0f) {
        m_FixedStep = stepSeconds;
    }
}

void Player::update(float deltaTime, const se::core::Input& input) {
    updateMouseLook(input);
    updateKeyboardMovement(deltaTime, input);
}

void Player::updateMouseLook(const se::core::Input& input) {
    float rawDx = input.getMouseDeltaX();
    float rawDy = -input.getMouseDeltaY();
    m_SmoothedDx = m_SmoothedDx + (rawDx - m_SmoothedDx) * m_MouseSmoothAlpha;
    m_SmoothedDy = m_SmoothedDy + (rawDy - m_SmoothedDy) * m_MouseSmoothAlpha;

    m_Camera.processMouse(m_SmoothedDx, m_SmoothedDy);
}

void Player::updateKeyboardMovement(float deltaTime, const se::core::Input& input) {
    m_MoveAccumulator += deltaTime;
    int steps = 0;
    while (m_MoveAccumulator >= m_FixedStep && steps < 4) {
        applyKeyboardStep(m_FixedStep, input);
        m_MoveAccumulator -= m_FixedStep;
        steps++;
    }

    if (steps == 0) {
        applyKeyboardStep(deltaTime, input);
        m_MoveAccumulator = 0.0f;
    }
}

void Player::applyKeyboardStep(float stepSeconds, const se::core::Input& input) {
    m_Camera.processKeyboard(
        input.isKeyDown(GLFW_KEY_W),
        input.isKeyDown(GLFW_KEY_S),
        input.isKeyDown(GLFW_KEY_A),
        input.isKeyDown(GLFW_KEY_D),
        input.isKeyDown(GLFW_KEY_SPACE),
        input.isKeyDown(GLFW_KEY_LEFT_CONTROL),
        stepSeconds);
}

}  // namespace se::scene