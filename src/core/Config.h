#pragma once

#include <SimpleIni.h>

#include <string>
#include <string_view>

namespace se::core {

class Config {
   public:
    struct Window {
        std::string title = "Simple Engine";
        int width = 1280;
        int height = 720;
        bool vsync = true;
        bool startFullscreen = false;
    };

    struct Input {
        float mouseSmoothAlpha = 0.5f;
        float mouseSensitivity = 0.1f;
        float fixedStep = 1.0f / 120.0f;
    };

    struct Camera {
        float moveSpeed = 15.0f;
        float fov = 60.0f;
        float nearPlane = 0.1f;
        // For big models like Sponza, we need a far plane of at least 500 to avoid clipping geometry.
        // We set it to 1000 by default to give some extra headroom, but it can be adjusted in the config if needed.
        float farPlane = 1000.0f;
        float startPosX = -5.0f;
        float startPosY = 5.0f;
        float startPosZ = 5.0f;
    };

    struct Stats {
        bool showStats = true;
        // How often to update stats in seconds. A lower interval will update more frequently
        // but may cause more performance overhead.
        float interval = 1.0f;
    };

    static Config load(std::string_view path);

    const Window& window() const { return m_Window; }
    const Input& input() const { return m_Input; }
    const Camera& camera() const { return m_Camera; }
    const Stats& stats() const { return m_Stats; }

   private:
    static void readWindow(const CSimpleIniA&, Window&);
    static void readInput(const CSimpleIniA&, Input&);
    static void readCamera(const CSimpleIniA&, Camera&);
    static void readStats(const CSimpleIniA&, Stats&);

    Window m_Window;
    Input m_Input;
    Camera m_Camera;
    Stats m_Stats;
};

}  // namespace se::core