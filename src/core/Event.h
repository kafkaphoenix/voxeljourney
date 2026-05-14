#pragma once
#include <cstdint>

namespace se::core {

enum class EventType : uint8_t { FramebufferResize, WindowFocus, Key, MouseButton, MouseMove, Scroll };

struct Event {
    explicit Event(EventType type) : type(type) {}
    virtual ~Event() = default;
    EventType type;
};

struct FramebufferResizeEvent final : Event {
    static constexpr EventType EVENT_TYPE = EventType::FramebufferResize;
    FramebufferResizeEvent(int width, int height) : Event(EVENT_TYPE), width(width), height(height) {}
    int width;
    int height;
};

struct WindowFocusEvent final : Event {
    static constexpr EventType EVENT_TYPE = EventType::WindowFocus;
    explicit WindowFocusEvent(bool focused) : Event(EVENT_TYPE), focused(focused) {}
    bool focused;
};

struct KeyEvent final : Event {
    static constexpr EventType EVENT_TYPE = EventType::Key;
    KeyEvent(int key, int scancode, int action, int mods)
        : Event(EVENT_TYPE), key(key), scancode(scancode), action(action), mods(mods) {}
    int key;
    int scancode;
    int action;
    int mods;
};

struct MouseButtonEvent final : Event {
    static constexpr EventType EVENT_TYPE = EventType::MouseButton;
    MouseButtonEvent(int button, int action, int mods)
        : Event(EVENT_TYPE), button(button), action(action), mods(mods) {}
    int button;
    int action;
    int mods;
};

struct MouseMoveEvent final : Event {
    static constexpr EventType EVENT_TYPE = EventType::MouseMove;
    MouseMoveEvent(double x, double y) : Event(EVENT_TYPE), x(x), y(y) {}
    double x;
    double y;
};

struct ScrollEvent final : Event {
    static constexpr EventType EVENT_TYPE = EventType::Scroll;
    ScrollEvent(double xoffset, double yoffset) : Event(EVENT_TYPE), xoffset(xoffset), yoffset(yoffset) {}
    double xoffset;
    double yoffset;
};

}  // namespace se::core
