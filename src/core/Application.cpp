#include "Application.h"

#include <algorithm>

namespace se::core {

Application::Application()
    : m_Config(Config::load("config.ini")),
      m_StatsTracker(m_Config.stats()),
      m_Window(m_Config.window().width, m_Config.window().height, m_Config.window().title, &m_EventBus),
      m_Renderer(),
      m_Scene(static_cast<float>(m_Config.window().width) / static_cast<float>(m_Config.window().height), m_AssetManager) {
    setupWindow();
    subscribeEvents();

    if (m_Config.window().startFullscreen) {
        m_Window.toggleFullscreen();
    }

    m_EventBus.dispatchQueued();

    m_Renderer.setCamera(m_Scene.getPlayer().getCamera());
    m_Scene.initialize();
    m_Scene.applyConfig(m_Config);
    m_Input.resetMouseFromWindow(m_Window.native());
    m_Scene.getPlayer().update(0.0f, m_Input);
}

Application::~Application() {
    m_Renderer.reset();
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
    auto title = m_StatsTracker.update(deltaTime, m_Renderer.getStats(), m_Window.baseTitle());
    if (title) {
        m_Window.setTitle(*title);
    }
}

void Application::setupWindow() {
    m_Window.applyConfig(m_Config.window());
}

void Application::subscribeEvents() {
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<FramebufferResizeEvent>([this](const FramebufferResizeEvent& e) {
        if (e.width > 0 && e.height > 0) {
            m_Scene.getPlayer().getCamera().setAspect(
                static_cast<float>(e.width) / static_cast<float>(e.height));
        }
    }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<KeyEvent>([this](const KeyEvent& e) { m_Input.onKeyEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<MouseButtonEvent>([this](const MouseButtonEvent& e) { m_Input.onMouseButtonEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<MouseMoveEvent>([this](const MouseMoveEvent& e) { m_Input.onMouseMoveEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<ScrollEvent>([this](const ScrollEvent& e) { m_Input.onScrollEvent(e); }));
    m_Subscriptions.push_back(m_EventBus.subscribeScoped<WindowFocusEvent>([this](const WindowFocusEvent& e) { m_Input.onWindowFocusEvent(e); }));
}

void Application::handleShortcuts() {
    if (m_Input.isKeyDown(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(m_Window.native(), true);
    }

    if (m_Input.isKeyPressed(GLFW_KEY_F3)) {
        m_Renderer.toggleWireframe();
    }

    if (m_Input.isKeyPressed(GLFW_KEY_F12)) {
        m_Window.toggleFullscreen();
        m_Input.resetMouseFromWindow(m_Window.native());
    }
}

void Application::updateScene(float deltaTime) {
    m_Scene.update(deltaTime, m_Input);
}

void Application::renderFrame() {
    m_Renderer.render(m_Scene);
}

void Application::run() {
    float lastTime = 0.0f;
    while (!m_Window.shouldClose()) {
        float dt = updateDeltaTime(lastTime);

        beginFrame();

        handleShortcuts();

        updateScene(dt);
        renderFrame();

        updateStats(dt);

        m_Window.swapBuffers();
    }
}

}  // namespace se::core
