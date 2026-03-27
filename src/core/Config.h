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
        // We set it to 1000 by default to give some extra headroom, but it can be adjusted in the config if needed.
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