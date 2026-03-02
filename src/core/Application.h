#pragma once
#include <vector>

#include "Config.h"
#include "EventBus.h"
#include "Input.h"
#include "StatsTracker.h"
#include "Window.h"
#include "assets/AssetManager.h"
#include "render/Renderer.h"
#include "scene/Scene.h"

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
    void handleShortcuts();
    void updateScene(float deltaTime);
    void renderFrame();
    void updateStats(float deltaTime);

    Config m_Config;
    StatsTracker m_StatsTracker;
    EventBus m_EventBus;
    std::vector<EventBus::Subscription> m_Subscriptions;
    Input m_Input;
    Window m_Window;
    se::assets::AssetManager m_AssetManager;
    se::render::Renderer m_Renderer;
    se::scene::Scene m_Scene;
};

}  // namespace se::core
