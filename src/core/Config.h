#pragma once

#include <SimpleIni.h>
#include <string>

namespace se::core {

class Config {
public:
    struct Window {
        std::string title = "Simple Engine";
        int width = 1280;
        int height = 720;
        bool vsync = true;
        bool startFullscreen = false;
        bool glDebugNotifications = false;
    };

    struct Input {
        float mouseSmoothAlpha = 0.5f;
        float mouseSensitivity = 0.1f;
        float fixedStep = 1.0f / 120.0f;
    };

    struct Camera {
        float moveSpeed = 5.0f;
        float fov = 60.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        float startPosX = -5.0f;
        float startPosY = 5.0f;
        float startPosZ = 5.0f;
    };

    struct Stats {
        bool showStats = true;
        float interval = 0.25f;
    };

    static Config load(const std::string& path);

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

} // namespace se::core