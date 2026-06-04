#pragma once
#include <vector>

#include "Config.h"
#include "EventBus.h"
#include "Input.h"
#include "Level.h"
#include "StatsTracker.h"
#include "Window.h"
#include "assets/AssetManager.h"
#include "render/RenderManager.h"

namespace se::core {

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    static float updateDeltaTime(float& lastTime);
    void beginFrame();
    void subscribeEvents();
    void handleShortcuts();
    void update(float deltaTime);
    void render();
    void updateStats(float deltaTime, const FrameDebugStats& frameDebugStats);

    Config m_Config;
    StatsTracker m_StatsTracker;
    EventBus m_EventBus;
    std::vector<EventBus::Subscription> m_Subscriptions;
    Input m_Input;
    Window m_Window;
    se::assets::AssetManager m_AssetManager;
    se::render::RenderManager m_RenderManager;
    Level m_Level;
};

}  // namespace se::core
