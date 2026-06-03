#include "Window.h"

#include <glad/glad.h>

#include <cstdio>
#include <format>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Config.h"
#include "EventBus.h"

namespace se::core {

void windowGlDebugCallback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length,
                           const char* message, const void* userParam);

namespace {
std::string glSeverityName(unsigned int severity) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
    case GL_DEBUG_SEVERITY_LOW: return "LOW";
    case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFY";
    default: return "UNKNOWN";
    }
}

std::string glSourceName(unsigned int source) {
    switch (source) {
    case GL_DEBUG_SOURCE_API: return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW_SYSTEM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER_COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
    case GL_DEBUG_SOURCE_OTHER: return "OTHER";
    default: return "UNKNOWN";
    }
}

std::string glTypeName(unsigned int type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR: return "ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED";
    case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
    case GL_DEBUG_TYPE_MARKER: return "MARKER";
    case GL_DEBUG_TYPE_PUSH_GROUP: return "PUSH_GROUP";
    case GL_DEBUG_TYPE_POP_GROUP: return "POP_GROUP";
    case GL_DEBUG_TYPE_OTHER: return "OTHER";
    default: return "UNKNOWN";
    }
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    glViewport(0, 0, width, height);

    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }

    self->onFramebufferResize(width, height);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }
    self->onKeyEvent(key, scancode, action, mods);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }
    self->onMouseButtonEvent(button, action, mods);
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }
    self->onMouseMove(xpos, ypos);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }
    self->onScroll(xoffset, yoffset);
}

void windowFocusCallback(GLFWwindow* window, int focused) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }
    self->onFocusChange(focused != 0);
}

void windowPosCallback(GLFWwindow* window, int xpos, int ypos) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }
    self->onPositionChange(xpos, ypos);
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }
    self->onSizeChange(width, height);
}

void windowIconifyCallback(GLFWwindow* window, int iconified) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }
    self->onIconifyChange(iconified != 0);
}

}

Window::Window(const config::Window& config, const config::Render& renderConfig, EventBus* eventBus)
    : m_EventBus(eventBus),
      m_Width(config.width),
      m_Height(config.height),
      m_PosX(config.posX),
      m_PosY(config.posY),
      m_Title(config.title),
      m_BaseTitle(config.title),
      m_Vsync(config.vsync) {
    initGlfw();
    setupGlfwHints(renderConfig);
    createWindow(config.width, config.height, m_Title);
    glfwSetWindowUserPointer(m_Window, this);
    try {
        initGlad();
    } catch (...) {
        glfwDestroyWindow(m_Window);
        glfwTerminate();
        throw;
    }
    setupGlDebug();
    setupCallbacks();
    setupInitialFramebuffer(config.width, config.height);
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    setMode(config.mode);
    setVsync(m_Vsync);
}

void Window::initGlfw() {
    if (!glfwInit()) {
        const char* description = nullptr;
        glfwGetError(&description);
        throw std::runtime_error(std::format("GLFW init failed: {}", description ? description : "Unknown error"));
    }
}

void Window::setupGlfwHints(const config::Render& renderConfig) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, renderConfig.msaaSamples);
}

void Window::createWindow(int width, int height, std::string_view title) {
    std::string titleStr(title);
    m_Window = glfwCreateWindow(width, height, titleStr.c_str(), nullptr, nullptr);
    if (!m_Window) {
        throw std::runtime_error("Window creation failed");
    }
}

void Window::initGlad() {
    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
}

void Window::setupGlDebug() {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(windowGlDebugCallback, this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}

void Window::setupCallbacks() {
    glfwSetFramebufferSizeCallback(m_Window, framebufferSizeCallback);
    glfwSetKeyCallback(m_Window, keyCallback);
    glfwSetMouseButtonCallback(m_Window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_Window, cursorPosCallback);
    glfwSetScrollCallback(m_Window, scrollCallback);
    glfwSetWindowFocusCallback(m_Window, windowFocusCallback);
    glfwSetWindowPosCallback(m_Window, windowPosCallback);
    glfwSetWindowSizeCallback(m_Window, windowSizeCallback);
    glfwSetWindowIconifyCallback(m_Window, windowIconifyCallback);
}

void Window::setupInitialFramebuffer(int width, int height) {
    glViewport(0, 0, width, height);

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(m_Window, &fbWidth, &fbHeight);
    if (fbWidth > 0 && fbHeight > 0) {
        m_LastFramebufferWidth = fbWidth;
        m_LastFramebufferHeight = fbHeight;
        glViewport(0, 0, fbWidth, fbHeight);
        if (m_EventBus) {
            FramebufferResizeEvent event(fbWidth, fbHeight);
            m_EventBus->queue(event);
        }
    }
}

Window::~Window() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Window::swapBuffers() const { glfwSwapBuffers(m_Window); }
bool Window::shouldClose() const { return glfwWindowShouldClose(m_Window); }
void Window::onFramebufferResize(int width, int height) {
    if (!m_EventBus) {
        return;
    }

    if (width == m_LastFramebufferWidth && height == m_LastFramebufferHeight) {
        return;
    }

    m_LastFramebufferWidth = width;
    m_LastFramebufferHeight = height;

    FramebufferResizeEvent event(width, height);
    m_EventBus->queue(event);
}

void Window::onKeyEvent(int key, int scancode, int action, int mods) {
    if (!m_EventBus) {
        return;
    }
    KeyEvent event(key, scancode, action, mods);
    m_EventBus->queue(event);
}

void Window::onMouseButtonEvent(int button, int action, int mods) {
    if (!m_EventBus) {
        return;
    }
    MouseButtonEvent event(button, action, mods);
    m_EventBus->queue(event);
}

void Window::onMouseMove(double xpos, double ypos) {
    if (!m_EventBus) {
        return;
    }
    MouseMoveEvent event(xpos, ypos);
    m_EventBus->queue(event);
}

void Window::onScroll(double xoffset, double yoffset) {
    if (!m_EventBus) {
        return;
    }
    ScrollEvent event(xoffset, yoffset);
    m_EventBus->queue(event);
}

void Window::onFocusChange(bool focused) {
    if (!m_EventBus) {
        return;
    }
    m_Focused = focused;
    WindowFocusEvent event(focused);
    m_EventBus->queue(event);
}

void Window::onPositionChange(int xpos, int ypos) {
    if (m_Mode != config::WindowMode::Windowed || m_IgnoreSizeEvents || m_LastMode != config::WindowMode::Windowed) {
        return;
    }
    m_PosX = xpos;
    m_PosY = ypos;
}

void Window::onSizeChange(int width, int height) {
    if (m_Mode != config::WindowMode::Windowed || m_IgnoreSizeEvents || m_LastMode != config::WindowMode::Windowed) {
        return;
    }
    if (width <= 0 || height <= 0) {
        return;
    }
    m_Width = width;
    m_Height = height;
}

void Window::onIconifyChange(bool minimized) { m_Minimized = minimized; }

void Window::setMode(config::WindowMode newMode) {
    if (m_Mode == newMode) {
        return;
    }

    m_LastMode = m_Mode;
    m_Mode = newMode;
    GLFWmonitor* monitor = nullptr;
    int xpos = 0;
    int ypos = 0;
    int width = 0;
    int height = 0;
    int refresh = GLFW_DONT_CARE;

    m_IgnoreSizeEvents = true;
    if (newMode == config::WindowMode::Windowed) {
        xpos = m_PosX;
        ypos = m_PosY;
        width = m_Width;
        height = m_Height;
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_TRUE);
        monitor = nullptr;
        refresh = GLFW_DONT_CARE;
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        int nmonitors = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&nmonitors);
        monitor = glfwGetPrimaryMonitor();
        if (monitors && nmonitors > 0) {
            int wx = 0;
            int wy = 0;
            int ww = 0;
            int wh = 0;
            glfwGetWindowPos(m_Window, &wx, &wy);
            glfwGetWindowSize(m_Window, &ww, &wh);
            for (int i = 0; i < nmonitors; ++i) {
                int mx = 0;
                int my = 0;
                glfwGetMonitorPos(monitors[i], &mx, &my);
                const GLFWvidmode* vm = glfwGetVideoMode(monitors[i]);
                if (wx + ww / 2 >= mx && wx + ww / 2 < mx + vm->width && wy + wh / 2 >= my &&
                    wy + wh / 2 < my + vm->height) {
                    monitor = monitors[i];
                    break;
                }
            }
        }
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        xpos = 0;
        ypos = 0;
        width = mode->width;
        height = mode->height;
        refresh = mode->refreshRate;
        if (newMode == config::WindowMode::Borderless) {
            glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_FALSE);
        } else {
            glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_TRUE);
        }
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    glfwSetWindowMonitor(m_Window, monitor, xpos, ypos, width, height, refresh);
    if (newMode == config::WindowMode::Windowed) {
        glfwSetWindowPos(m_Window, xpos, ypos);
    }
    m_IgnoreSizeEvents = false;

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(m_Window, &fbWidth, &fbHeight);
    if (fbWidth > 0 && fbHeight > 0) {
        if (fbWidth != m_LastFramebufferWidth || fbHeight != m_LastFramebufferHeight) {
            m_LastFramebufferWidth = fbWidth;
            m_LastFramebufferHeight = fbHeight;
        }
        glViewport(0, 0, fbWidth, fbHeight);
        if (m_EventBus) {
            FramebufferResizeEvent event(fbWidth, fbHeight);
            m_EventBus->queue(event);
        }
    }
}

void Window::setStatsTitle(std::string title) {
    m_Title = std::move(title);
    glfwSetWindowTitle(m_Window, m_Title.c_str());
}

void Window::setVsync(bool enabled) {
    m_Vsync = enabled;
    glfwSwapInterval(enabled ? 1 : 0);
}

void windowGlDebugCallback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length,
                           const char* message, const void* userParam) {
    (void)source;
    (void)type;
    (void)length;

    const auto* window = static_cast<const Window*>(userParam);
    if (!window || !message) {
        return;
    }

    std::string text = std::string("OpenGL ") + glSeverityName(severity) + " " + glTypeName(type) + " [" +
                       glSourceName(source) + "]" + " (" + std::to_string(id) + "): " + message;

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        throw std::runtime_error(text);
    }

    std::println(stderr, "{}", text);
}

}  // namespace se::core
