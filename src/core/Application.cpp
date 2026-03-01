#include "Application.h"

#include <algorithm>

#include "MemoryUtils.h"

namespace se::core {

Application::Application()
    : m_Config(Config::load("config.ini")),
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
    applyConfigToCamera();
    resetMouseState();
    m_Scene.getPlayer().update(0.0f, m_Input);
    m_Scene.getPlayer().setMouseSmoothing(m_Config.input().mouseSmoothAlpha);
    m_Scene.getPlayer().setFixedStep(m_Config.input().fixedStep);
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
    if (!m_ShowStats) {
        return;
    }

    m_StatsTimer += deltaTime;
    m_StatsFrames++;
    if (m_StatsTimer >= m_Config.stats().interval) {
        float fps = m_StatsFrames / m_StatsTimer;
        const auto& stats = m_Renderer.getStats();
        size_t memKB = getProcessMemoryUsageKB();
        std::string title = m_Window.baseTitle() +
                            " | FPS: " + std::to_string(static_cast<int>(fps)) +
                            " | Draws: " + std::to_string(stats.drawCalls) +
                            " | Triangles: " + std::to_string(stats.triangles) +
                            " | RAM: " + std::to_string(memKB / 1024) + "MB";
        m_Window.setTitle(title);
        m_StatsFrames = 0;
        m_StatsTimer = 0.0f;
    }
}

void Application::setupWindow() {
    m_Window.setVsync(m_Config.window().vsync);
    m_Window.setGlDebugNotifications(m_Config.window().glDebugNotifications);
    m_ShowStats = m_Config.stats().showStats;
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

void Application::applyConfigToCamera() {
    auto& camera = m_Scene.getPlayer().getCamera();
    se::scene::Camera::Settings settings;
    settings.position = {m_Config.camera().startPosX, m_Config.camera().startPosY, m_Config.camera().startPosZ};
    settings.moveSpeed = m_Config.camera().moveSpeed;
    settings.mouseSensitivity = m_Config.input().mouseSensitivity;
    settings.fov = m_Config.camera().fov;
    settings.nearPlane = m_Config.camera().nearPlane;
    settings.farPlane = m_Config.camera().farPlane;
    camera.applySettings(settings);
}

void Application::resetMouseState() {
    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(m_Window.native(), &xpos, &ypos);
    m_Input.resetMouse(xpos, ypos);
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
        resetMouseState();
    }
}

void Application::updateScene(float deltaTime) {
    m_Scene.update(deltaTime, m_Input);
}

void Application::renderFrame() {
    m_Renderer.setLights(buildLightSet());
    renderScene();
}

se::render::Renderer::LightSet Application::buildLightSet() const {
    se::render::Renderer::LightSet lights;
    const auto& sky = m_Scene.getSky();
    const auto& sun = sky.getSun().getLight();
    lights.sunDir = sun.direction;
    lights.sunColor = sun.color * sun.intensity;
    lights.ambientColor = sky.getAmbientColor();
    lights.ambientStrength = sky.getAmbientStrength();

    const auto& pointLights = m_Scene.getPointLights();
    lights.pointLights.reserve(pointLights.size());
    for (const auto& light : pointLights) {
        if (light.type != se::scene::LightType::Point) {
            continue;
        }
        se::render::Renderer::PointLightData data;
        data.position = light.position;
        data.color = light.color;
        data.intensity = light.intensity;
        data.range = light.range;
        lights.pointLights.push_back(data);
    }

    return lights;
}

void Application::renderScene() {
    m_Renderer.clear();
    for (const auto& renderable : m_Scene.getRenderables()) {
        m_Renderer.submit(renderable);
    }
    m_Renderer.flush();
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
