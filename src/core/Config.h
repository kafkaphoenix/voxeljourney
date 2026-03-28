#pragma once
#include <SimpleIni.h>

#include <glm/glm.hpp>
#include <string>
#include <string_view>

namespace se::core {

class Config {
public:
    struct Window {
        std::string title = "Simple Engine";
        int width = 1280;
        int height = 720;
        int posX = 100;
        int posY = 100;
        bool vsync = true;
        // "windowed", "borderless", or "fullscreen"
        std::string mode = "windowed";
    };

    struct Input {
        float mouseSmoothAlpha = 0.5f;
        float fixedStep = 1.0f / 120.0f;
    };

    struct Camera {
        float moveSpeed = 15.0f;
        float fov = 60.0f;
        float nearPlane = 0.1f;
        // For big models like Sponza, we need a far plane of at least 500 to avoid clipping geometry.
        // We set it to 1000 by default to give some extra headroom, but it can be adjusted in the config if
        // needed.
        float farPlane = 1000.0f;
        glm::vec3 position = glm::vec3(-5.0f, 5.0f, 5.0f);
        float aspectRatio = 16.0f / 9.0f;
        float sensitivity = 0.1f;
    };

    struct Stats {
        bool enabled = true;
        // How often to update stats in seconds. A lower interval will update more frequently
        // but may cause more performance overhead.
        float refreshInterval = 1.0f;
    };

    static Config load(std::string_view path);

    [[nodiscard]] const Window& window() const { return m_Window; }
    [[nodiscard]] const Input& input() const { return m_Input; }
    [[nodiscard]] const Camera& camera() const { return m_Camera; }
    [[nodiscard]] const Stats& stats() const { return m_Stats; }

private:
    static void readWindow(const CSimpleIniA& ini, Window& w);
    static void readInput(const CSimpleIniA& ini, Input& i);
    static void readCamera(const CSimpleIniA& ini, Camera& c);
    static void readStats(const CSimpleIniA& ini, Stats& s);

    Window m_Window;
    Input m_Input;
    Camera m_Camera;
    Stats m_Stats;
};

}  // namespace se::core