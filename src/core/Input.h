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

    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;
    bool isKeyReleased(int key) const;

    bool isMouseButtonDown(int button) const;
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonReleased(int button) const;

    float getMouseX() const { return static_cast<float>(m_MouseX); }
    float getMouseY() const { return static_cast<float>(m_MouseY); }
    float getMouseDeltaX() const { return static_cast<float>(m_MouseDeltaX); }
    float getMouseDeltaY() const { return static_cast<float>(m_MouseDeltaY); }
    float getScrollX() const { return static_cast<float>(m_ScrollX); }
    float getScrollY() const { return static_cast<float>(m_ScrollY); }

    void resetMouse(double x, double y);

   private:
    enum class ButtonState : uint8_t {
        Up,
        Pressed,
        Down,
        Released
    };
    void advanceState(ButtonState& state);
    bool isValidKey(int key) const;
    bool isValidMouseButton(int button) const;

    static constexpr int KeyCount = GLFW_KEY_LAST + 1;
    static constexpr int MouseButtonCount = GLFW_MOUSE_BUTTON_LAST + 1;

    std::array<ButtonState, KeyCount> m_Keys{};
    std::array<ButtonState, MouseButtonCount> m_MouseButtons{};

    double m_MouseX = 0.0;
    double m_MouseY = 0.0;
    double m_MouseDeltaX = 0.0;
    double m_MouseDeltaY = 0.0;
    double m_ScrollX = 0.0;
    double m_ScrollY = 0.0;

    bool m_InitializedMouse = false;
};

}  // namespace se::core
