#include "Application.h"

#include <algorithm>

namespace se::core {

Application::Application()
    : m_Config(Config::load("config.ini")),
      m_StatsTracker(m_Config.stats()),
      m_Window(m_Config.window(), &m_EventBus),
      m_RenderManager(),
      m_Level(m_Config, m_RenderManager, m_AssetManager) {
    subscribeEvents();
    m_EventBus.dispatchQueued();
}

Application::~Application() {
    m_RenderManager.reset();
    m_AssetManager.clear();
}

float Application::updateDeltaTime(float& lastTime) {
    float currentTime = static_cast<float>(glfwGetTime());
    float dt = currentTime - lastTime;
    dt = std::min(dt, 0.1f);
    lastTime = currentTime;
    return dt;
}

void Application::beginFrame() {
    m_Input.beginFrame();
    m_Window.pollEvents();
    m_EventBus.dispatchQueued();
}

void Application::updateStats(float deltaTime) {
    auto stats = m_StatsTracker.update(deltaTime, m_RenderManager.getStats(), m_Window.getBaseTitle());
    if (stats) {
        m_Window.setStatsTitle(*stats);
    }
}

void Application::subscribeEvents() {
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<FramebufferResizeEvent>([this](const FramebufferResizeEvent& e) {
        if (e.width > 0 && e.height > 0) {
            m_Level.getPlayer().getCamera().setAspectRatio(
                static_cast<float>(e.width) / static_cast<float>(e.height));
        }
    }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<KeyEvent>([this](const KeyEvent& e) { m_Input.onKeyEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<MouseButtonEvent>([this](const MouseButtonEvent& e) { m_Input.onMouseButtonEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<MouseMoveEvent>([this](const MouseMoveEvent& e) { m_Input.onMouseMoveEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<ScrollEvent>([this](const ScrollEvent& e) { m_Input.onScrollEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<WindowFocusEvent>([this](const WindowFocusEvent& e) {
        m_Input.onWindowFocusEvent(e);
        if (e.focused) {
            // When the window regains focus, reset the mouse position to prevent sudden jumps if the mouse moved while unfocused
            m_Input.resetMouseFromWindow(m_Window.native());
        }
    }));
}

void Application::handleShortcuts() {
    if (m_Input.isKeyDown(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(m_Window.native(), true);
    }

    if (m_Input.isKeyPressed(GLFW_KEY_F3)) {
        m_RenderManager.toggleWireframe();
    }

    if (m_Input.isKeyPressed(GLFW_KEY_F12)) {
        switch (m_Window.mode()) {
            case Window::Mode::Windowed:
                m_Window.setMode(Window::Mode::Fullscreen);
                break;
            case Window::Mode::Fullscreen:
                m_Window.setMode(Window::Mode::Windowed);
                break;
        }
    }
}

void Application::update(float deltaTime) {
    m_Level.update(deltaTime, m_Input);
}

void Application::render() {
    m_Level.render(m_RenderManager);
}

void Application::run() {
    float lastTime = 0.0f;
    while (!m_Window.shouldClose()) {
        float dt = updateDeltaTime(lastTime);

        beginFrame();

        if (m_Window.isMinimized() || !m_Window.isFocused()) {
            m_Window.waitEvents(0.1);
            continue;
        }

        handleShortcuts();

        update(dt);
        render();

        updateStats(dt);

        m_Window.swapBuffers();
    }
}

}  // namespace se::core
