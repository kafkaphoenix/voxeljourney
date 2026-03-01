#include "Input.h"

namespace se::core {

void Input::beginFrame() {
    m_ScrollX = 0.0;
    m_ScrollY = 0.0;
    m_MouseDeltaX = 0.0;
    m_MouseDeltaY = 0.0;

    for (auto& keyState : m_Keys) {
        advanceState(keyState);
    }
    for (auto& buttonState : m_MouseButtons) {
        advanceState(buttonState);
    }
}

void Input::onKeyEvent(const KeyEvent& event) {
    if (!isValidKey(event.key)) {
        return;
    }
    auto& state = m_Keys[event.key];
    if (event.action == GLFW_PRESS) {
        state = ButtonState::Pressed;
    } else if (event.action == GLFW_RELEASE) {
        state = ButtonState::Released;
    }
}

void Input::onMouseButtonEvent(const MouseButtonEvent& event) {
    if (!isValidMouseButton(event.button)) {
        return;
    }
    auto& state = m_MouseButtons[event.button];
    if (event.action == GLFW_PRESS) {
        state = ButtonState::Pressed;
    } else if (event.action == GLFW_RELEASE) {
        state = ButtonState::Released;
    }
}

void Input::onMouseMoveEvent(const MouseMoveEvent& event) {
    if (!m_InitializedMouse) {
        m_MouseX = event.x;
        m_MouseY = event.y;
        m_InitializedMouse = true;
        return;
    }

    m_MouseDeltaX += event.x - m_MouseX;
    m_MouseDeltaY += event.y - m_MouseY;
    m_MouseX = event.x;
    m_MouseY = event.y;
}

void Input::onScrollEvent(const ScrollEvent& event) {
    m_ScrollX += event.xoffset;
    m_ScrollY += event.yoffset;
}

void Input::onWindowFocusEvent(const WindowFocusEvent& event) {
    if (event.focused) {
        m_MouseDeltaX = 0.0;
        m_MouseDeltaY = 0.0;
        m_InitializedMouse = false;
        return;
    }

    for (auto& keyState : m_Keys) {
        keyState = ButtonState::Up;
    }
    for (auto& buttonState : m_MouseButtons) {
        buttonState = ButtonState::Up;
    }
    m_MouseDeltaX = 0.0;
    m_MouseDeltaY = 0.0;
    m_ScrollX = 0.0;
    m_ScrollY = 0.0;
}

bool Input::isKeyDown(int key) const {
    if (!isValidKey(key)) {
        return false;
    }
    const auto& state = m_Keys[key];
    return state == ButtonState::Down || state == ButtonState::Pressed;
}

bool Input::isKeyPressed(int key) const {
    if (!isValidKey(key)) {
        return false;
    }
    return m_Keys[key] == ButtonState::Pressed;
}

bool Input::isKeyReleased(int key) const {
    if (!isValidKey(key)) {
        return false;
    }
    return m_Keys[key] == ButtonState::Released;
}

bool Input::isMouseButtonDown(int button) const {
    if (!isValidMouseButton(button)) {
        return false;
    }
    const auto& state = m_MouseButtons[button];
    return state == ButtonState::Down || state == ButtonState::Pressed;
}

bool Input::isMouseButtonPressed(int button) const {
    if (!isValidMouseButton(button)) {
        return false;
    }
    return m_MouseButtons[button] == ButtonState::Pressed;
}

bool Input::isMouseButtonReleased(int button) const {
    if (!isValidMouseButton(button)) {
        return false;
    }
    return m_MouseButtons[button] == ButtonState::Released;
}

void Input::resetMouse(double x, double y) {
    m_MouseX = x;
    m_MouseY = y;
    m_MouseDeltaX = 0.0;
    m_MouseDeltaY = 0.0;
    m_InitializedMouse = true;
}

void Input::advanceState(ButtonState& state) {
    if (state == ButtonState::Pressed) {
        state = ButtonState::Down;
    } else if (state == ButtonState::Released) {
        state = ButtonState::Up;
    }
}

bool Input::isValidKey(int key) const {
    return key >= 0 && key < KeyCount;
}

bool Input::isValidMouseButton(int button) const {
    return button >= 0 && button < MouseButtonCount;
}

}  // namespace se::core
