#pragma once
#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>

#include "Event.h"

namespace se::core {

class Input {
public:
    void beginFrame();

    void onKeyEvent(const KeyEvent& event);
    void onMouseButtonEvent(const MouseButtonEvent& event);
    void onMouseMoveEvent(const MouseMoveEvent& event);
    void onScrollEvent(const ScrollEvent& event);
    void onWindowFocusEvent(const WindowFocusEvent& event);

    [[nodiscard]] bool isKeyDown(int key) const;
    [[nodiscard]] bool isKeyPressed(int key) const;
    [[nodiscard]] bool isKeyReleased(int key) const;

    [[nodiscard]] bool isMouseButtonDown(int button) const;
    [[nodiscard]] bool isMouseButtonPressed(int button) const;
    [[nodiscard]] bool isMouseButtonReleased(int button) const;

    [[nodiscard]] float getMouseX() const { return static_cast<float>(m_MouseX); }
    [[nodiscard]] float getMouseY() const { return static_cast<float>(m_MouseY); }
    [[nodiscard]] float getMouseDeltaX() const { return static_cast<float>(m_MouseDeltaX); }
    [[nodiscard]] float getMouseDeltaY() const { return static_cast<float>(m_MouseDeltaY); }
    [[nodiscard]] float getScrollX() const { return static_cast<float>(m_ScrollX); }
    [[nodiscard]] float getScrollY() const { return static_cast<float>(m_ScrollY); }

    void resetMouse(double x, double y);
    void resetMouseFromWindow(GLFWwindow* window);

private:
    enum class ButtonState : uint8_t { Up, Pressed, Down, Released };
    static void advanceState(ButtonState& state);
    [[nodiscard]] static bool isValidKey(int key);
    [[nodiscard]] static bool isValidMouseButton(int button);

    static constexpr int KEY_COUNT = GLFW_KEY_LAST + 1;
    static constexpr int MOUSE_BUTTON_COUNT = GLFW_MOUSE_BUTTON_LAST + 1;

    std::array<ButtonState, KEY_COUNT> m_Keys{};
    std::array<ButtonState, MOUSE_BUTTON_COUNT> m_MouseButtons{};

    double m_MouseX = 0.0;
    double m_MouseY = 0.0;
    double m_MouseDeltaX = 0.0;
    double m_MouseDeltaY = 0.0;
    double m_ScrollX = 0.0;
    double m_ScrollY = 0.0;

    bool m_InitializedMouse = false;
};

}  // namespace se::core
