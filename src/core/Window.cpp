#include "Window.h"

#include <glad/glad.h>

#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Config.h"
#include "EventBus.h"

namespace se::core {

void windowGlDebugCallback(unsigned int source, unsigned int type,
                           unsigned int id, unsigned int severity,
                           int length, const char* message, const void* userParam);

namespace {
std::string glSeverityName(unsigned int severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            return "HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM:
            return "MEDIUM";
        case GL_DEBUG_SEVERITY_LOW:
            return "LOW";
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return "NOTIFY";
        default:
            return "UNKNOWN";
    }
}

std::string glSourceName(unsigned int source) {
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "WINDOW_SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "SHADER_COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "THIRD_PARTY";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "APPLICATION";
        case GL_DEBUG_SOURCE_OTHER:
            return "OTHER";
        default:
            return "UNKNOWN";
    }
}

std::string glTypeName(unsigned int type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "DEPRECATED";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "UNDEFINED";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER:
            return "MARKER";
        case GL_DEBUG_TYPE_PUSH_GROUP:
            return "PUSH_GROUP";
        case GL_DEBUG_TYPE_POP_GROUP:
            return "POP_GROUP";
        case GL_DEBUG_TYPE_OTHER:
            return "OTHER";
        default:
            return "UNKNOWN";
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
    self->onWindowFocus(focused != 0);
}

}

Window::Window(int width, int height, std::string title, EventBus* eventBus)
    : m_EventBus(eventBus),
      m_IsFullscreen(false),
      m_WindowedWidth(width),
      m_WindowedHeight(height),
      m_WindowedPosX(100),
      m_WindowedPosY(100),
      m_Title(std::move(title)),
      m_BaseTitle(m_Title) {
    initGlfw();
    setupGlfwHints();
    createWindow(width, height, m_Title);
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
    setupInitialFramebuffer(width, height);
    setupInputMode();
}

void Window::initGlfw() {
    if (!glfwInit()) {
        const char* description;
        glfwGetError(&description);
        throw std::runtime_error(std::format(
            "GLFW init failed: {}", description ? description : "Unknown error"));
    }
}

void Window::setupGlfwHints() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
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
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
}

void Window::setupGlDebug() {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(windowGlDebugCallback, this);
    setGlDebugNotifications(false);
}

void Window::setupCallbacks() {
    glfwSetFramebufferSizeCallback(m_Window, framebufferSizeCallback);
    glfwSetKeyCallback(m_Window, keyCallback);
    glfwSetMouseButtonCallback(m_Window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_Window, cursorPosCallback);
    glfwSetScrollCallback(m_Window, scrollCallback);
    glfwSetWindowFocusCallback(m_Window, windowFocusCallback);
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

void Window::setupInputMode() {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
Window::~Window() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Window::pollEvents() const { glfwPollEvents(); }
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

void Window::onWindowFocus(bool focused) {
    if (!m_EventBus) {
        return;
    }
    WindowFocusEvent event(focused);
    m_EventBus->queue(event);
}
void Window::toggleFullscreen() {
    if (!m_IsFullscreen) {
        glfwGetWindowPos(m_Window, &m_WindowedPosX, &m_WindowedPosY);
        glfwGetWindowSize(m_Window, &m_WindowedWidth, &m_WindowedHeight);
    }

    GLFWmonitor* monitor = m_IsFullscreen ? nullptr : glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
    int targetX = m_IsFullscreen ? m_WindowedPosX : 0;
    int targetY = m_IsFullscreen ? m_WindowedPosY : 0;
    int targetW = m_IsFullscreen ? m_WindowedWidth : mode->width;
    int targetH = m_IsFullscreen ? m_WindowedHeight : mode->height;
    int targetRate = m_IsFullscreen ? 0 : mode->refreshRate;

    glfwSetWindowMonitor(m_Window, monitor, targetX, targetY, targetW, targetH, targetRate);
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    m_IsFullscreen = !m_IsFullscreen;
}

void Window::setStatsTitle(std::string title) {
    m_Title = std::move(title);
    glfwSetWindowTitle(m_Window, m_Title.c_str());
}

void Window::setVsync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
}

void Window::setGlDebugNotifications(bool enabled) {
    m_GlDebugNotifications = enabled;
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                          GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
                          enabled ? GL_TRUE : GL_FALSE);
}

void Window::applyConfig(const Config::Window& config) {
    setVsync(config.vsync);
    setGlDebugNotifications(config.glDebugNotifications);
}

void windowGlDebugCallback(unsigned int source, unsigned int type,
                           unsigned int id, unsigned int severity,
                           int length, const char* message, const void* userParam) {
    (void)source;
    (void)type;
    (void)length;

    auto* window = static_cast<Window*>(const_cast<void*>(userParam));
    if (!window || !message) {
        return;
    }

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION && !window->m_GlDebugNotifications) {
        return;
    }

    std::string text = std::string("OpenGL ") + glSeverityName(severity) +
                       " " + glTypeName(type) +
                       " [" + glSourceName(source) + "]" +
                       " (" + std::to_string(id) + "): " + message;

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        throw std::runtime_error(text);
    }

    std::cerr << text << std::endl;
}

}  // namespace se::core
