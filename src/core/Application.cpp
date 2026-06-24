#include "Application.h"

#include <algorithm>
#include <print>

#include "Timer.h"

namespace se::core {

Application::Application()
    : m_Config(Config::load("config.toml")),
      m_StatsTracker(m_Config.stats()),
      m_Window(m_Config.window(), m_Config.render(), &m_EventBus),
      m_RenderManager(m_Config),
      m_Level(m_Config, m_Input, m_RenderManager, m_AssetManager) {
    subscribeEvents();
    m_EventBus.dispatchQueued();
}

Application::~Application() {
    m_RenderManager.reset();
    m_AssetManager.clear();
}

float Application::updateDeltaTime(float& lastTime) {
    auto currentTime = static_cast<float>(glfwGetTime());
    float dt = currentTime - lastTime;
    dt = std::min(dt, 0.1f);
    lastTime = currentTime;
    return dt;
}

void Application::beginFrame() {
    m_Input.beginFrame();
    Window::pollEvents();
    m_EventBus.dispatchQueued();
}

void Application::updateStats(float deltaTime, const FrameDebugStats& frameDebugStats) {
    auto stats = m_StatsTracker.update(deltaTime, m_RenderManager.getStats(), frameDebugStats, m_Window.getBaseTitle());
    if (stats) {
        m_Window.setStatsTitle(*stats);
    }
}

void Application::subscribeEvents() {
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<FramebufferResizeEvent>([this](
                                                                                     const FramebufferResizeEvent& e) {
        if (e.width > 0 && e.height > 0) {
            m_Level.getPlayer().getCamera().setAspectRatio(static_cast<float>(e.width) / static_cast<float>(e.height));
            m_RenderManager.resizeFramebuffer(e.width, e.height);
        }
    }));
    m_Subscriptions.push_back(
        m_EventBus.subscribeScoped<KeyEvent>([this](const KeyEvent& e) { m_Input.onKeyEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<MouseButtonEvent>(
        [this](const MouseButtonEvent& e) { m_Input.onMouseButtonEvent(e); }));
    m_Subscriptions.push_back(
        m_EventBus.subscribeScoped<MouseMoveEvent>([this](const MouseMoveEvent& e) { m_Input.onMouseMoveEvent(e); }));
    m_Subscriptions.push_back(
        m_EventBus.subscribeScoped<ScrollEvent>([this](const ScrollEvent& e) { m_Input.onScrollEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<WindowFocusEvent>([this](const WindowFocusEvent& e) {
        m_Input.onWindowFocusEvent(e);
        if (e.focused) {
            // When the window regains focus, reset the mouse position to prevent sudden jumps if the mouse moved while
            // unfocused
            m_Input.resetMouseFromWindow(m_Window.native());
        }
    }));
}

void Application::handleShortcuts() {
    if (m_Input.isKeyDown(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(m_Window.native(), static_cast<int>(true));
    }

    if (m_Input.isKeyPressed(GLFW_KEY_F3)) {
        m_RenderManager.toggleWireframe();
    }

    if (m_Input.isKeyPressed(GLFW_KEY_F4)) {
        m_RenderManager.cyclePostEffect();
    }

    if (m_Input.isKeyPressed(GLFW_KEY_F5)) {
        m_StatsTracker.cycleDisplayMode();
    }

    if (m_Input.isKeyPressed(GLFW_KEY_F6)) {
        m_RenderManager.cycleRenderDebugView();
    }

    if (m_Input.isKeyPressed(GLFW_KEY_F12)) {
        switch (m_Window.mode()) {
        case config::WindowMode::Windowed: m_Window.setMode(config::WindowMode::Fullscreen); break;
        case config::WindowMode::Fullscreen: m_Window.setMode(config::WindowMode::Windowed); break;
        }
    }
}

void Application::update(float deltaTime) { m_Level.update(deltaTime); }

void Application::render() { m_Level.render(m_RenderManager); }

void Application::run() {
    float lastTime = 0.0f;
    while (!m_Window.shouldClose()) {
        Timer frameTimer;
        float dt = updateDeltaTime(lastTime);

        beginFrame();

        if (m_Window.isMinimized() || !m_Window.isFocused()) {
            Window::waitEvents(0.1);
            continue;
        }

        handleShortcuts();

        Timer updateTimer;
        update(dt);
        auto updateMs = static_cast<float>(updateTimer.millis());

        Timer renderTimer;
        render();
        auto renderMs = static_cast<float>(renderTimer.millis());

        FrameDebugStats frameDebugStats;
        frameDebugStats.updateMs = updateMs;
        frameDebugStats.renderMs = renderMs;
        frameDebugStats.frameMs = static_cast<float>(frameTimer.millis());
        updateStats(dt, frameDebugStats);

        m_Window.swapBuffers();
    }
}

}  // namespace se::core
