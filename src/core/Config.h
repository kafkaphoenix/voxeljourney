#pragma once
#include <SimpleIni.h>

#include <glm/glm.hpp>
#include <string>
#include <string_view>

namespace se::core {

class Config {
public:
    struct Window {
        std::string title;
        int width;
        int height;
        int posX;
        int posY;
        bool vsync;
        // "windowed", "borderless", or "fullscreen"
        std::string mode;
    };

    struct Input {
        float mouseSmoothAlpha;
    };

    struct Player {
        float moveSpeed;
        float sensitivity;
        float fixedStep;
        float cameraHeight;
        float cameraDistance;
        glm::vec3 startPosition;
    };

    struct Camera {
        float fov;
        float nearPlane;
        float farPlane;
        float aspectRatio;
    };

    struct Stats {
        bool enabled;
        // How often to update stats in seconds. A lower interval will update more frequently
        // but may cause more performance overhead.
        float refreshInterval;
    };

    struct World {
        // How many chunks away from the player to render. A higher render distance will show more of the world
        // but may cause more performance overhead.
        int renderDistance = 8;
    };

    static Config load(std::string_view path);

    [[nodiscard]] const Window& window() const { return m_Window; }
    [[nodiscard]] const Input& input() const { return m_Input; }
    [[nodiscard]] const Player& player() const { return m_Player; }
    [[nodiscard]] const Camera& camera() const { return m_Camera; }
    [[nodiscard]] const Stats& stats() const { return m_Stats; }
    [[nodiscard]] const World& world() const { return m_World; }

private:
    static void readWindow(const CSimpleIniA& ini, Window& w);
    static void readInput(const CSimpleIniA& ini, Input& i);
    static void readPlayer(const CSimpleIniA& ini, Player& p);
    static void readCamera(const CSimpleIniA& ini, Camera& c);
    static void readStats(const CSimpleIniA& ini, Stats& s);
    static void readWorld(const CSimpleIniA& ini, World& w);

    // Default constructor is private to force use of load() method, which validates config values.
    Config() = default;

    Window m_Window{};
    Input m_Input{};
    Player m_Player{};
    Camera m_Camera{};
    Stats m_Stats{};
    World m_World{};
};

}  // namespace se::core