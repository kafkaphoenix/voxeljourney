#pragma once

#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>

namespace se::core {

enum class WindowMode : uint8_t { Windowed, Borderless, Fullscreen };

class Config {
public:
    struct Window {
        std::string title;
        int width;
        int height;
        int posX;
        int posY;
        bool vsync;
        WindowMode mode;
    };

    struct Input {
        float mouseSmoothAlpha;
    };

    struct Player {
        float walkSpeed;
        float runSpeed;
        float mouseSensitivity;
        bool useFixedStep;
        float fixedHz;
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
        // How often to update stats in seconds (lower interval = more frequent updates, higher performance overhead).
        float refreshInterval;
    };

    struct World {
        // How many chunks away from the player to render. A higher render distance will show more of the world
        // but may cause more performance overhead.
        int renderDistance = 8;
    };

    struct Render {
        int msaaSamples;
        float anisotropy;
    };

    static Config load(std::string_view path);

    [[nodiscard]] const Window& window() const { return m_Window; }
    [[nodiscard]] const Input& input() const { return m_Input; }
    [[nodiscard]] const Player& player() const { return m_Player; }
    [[nodiscard]] const Camera& camera() const { return m_Camera; }
    [[nodiscard]] const Stats& stats() const { return m_Stats; }
    [[nodiscard]] const World& world() const { return m_World; }
    [[nodiscard]] const Render& render() const { return m_Render; }

private:
    static void readWindow(const toml::table& t, Window& w);
    static void readInput(const toml::table& t, Input& i);
    static void readPlayer(const toml::table& t, Player& p);
    static void readCamera(const toml::table& t, Camera& c);
    static void readStats(const toml::table& t, Stats& s);
    static void readWorld(const toml::table& t, World& w);
    static void readRender(const toml::table& t, Render& r);

    static WindowMode parseWindowMode(std::string_view s);

    // Default constructor is private to prevent creating Config instances without loading from a file
    Config() = default;

    Window m_Window{};
    Input m_Input{};
    Player m_Player{};
    Camera m_Camera{};
    Stats m_Stats{};
    World m_World{};
    Render m_Render{};
};

}  // namespace se::core