#pragma once
#include <vector>

#include "assets/AssetManager.h"
#include "render/Renderer.h"
#include "scene/Scene.h"
#include "Config.h"
#include "EventBus.h"
#include "Input.h"
#include "Window.h"

namespace se::core {

class Application {
   public:
    Application();
    ~Application();

    void run();

   private:
    float updateDeltaTime(float& lastTime);
    void beginFrame();
    void setupWindow();
    void subscribeEvents();
    void applyConfigToCamera();
    void resetMouseState();
    void handleShortcuts();
    void updateScene(float deltaTime);
    void renderFrame();
    se::render::Renderer::LightSet buildLightSet() const;
    void renderScene();
    void updateStats(float deltaTime);

    Config m_Config;
    EventBus m_EventBus;
    std::vector<EventBus::Subscription> m_Subscriptions;
    Input m_Input;
    Window m_Window;
    se::assets::AssetManager m_AssetManager;
    se::render::Renderer m_Renderer;
    se::scene::Scene m_Scene;
    bool m_ShowStats = true;
    float m_StatsTimer = 0.0f;
    int m_StatsFrames = 0;
};

}  // namespace se::core
