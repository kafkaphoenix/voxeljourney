#pragma once

#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>

namespace se::core::config {

enum class WindowMode : uint8_t { Windowed, Borderless, Fullscreen };

struct Window {
    std::string title;
    int width, height, posX, posY;
    bool vsync;
    WindowMode mode;
};

struct PostProcess {
    float exposure;  // Only used for effects that require tone mapping (e.g. bloom)
};

struct Player {
    glm::vec3 spawn;
};

struct CharacterController {
    float walkSpeed, runSpeed, mouseSensitivity, mouseSmoothAlpha, turnResponsiveness;
    bool useRootMotion;
};

struct Camera {
    float fov, nearPlane, farPlane;
};

struct CameraController {
    float followDistance, followHeight, eyeHeight, eyeForwardOffset;
};

struct Stats {
    bool enabled;
    // How often to update stats in seconds (lower = more frequent, higher overhead)
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

}  // namespace se::core::config

namespace se::core {

class Config {
public:
    static Config load(std::string_view path);

    [[nodiscard]] const config::Window& window() const { return m_Window; }
    [[nodiscard]] const config::Player& player() const { return m_Player; }
    [[nodiscard]] const config::CharacterController& characterController() const { return m_CharacterController; }
    [[nodiscard]] const config::Camera& camera() const { return m_Camera; }
    [[nodiscard]] const config::CameraController& cameraController() const { return m_CameraController; }
    [[nodiscard]] const config::Stats& stats() const { return m_Stats; }
    [[nodiscard]] const config::World& world() const { return m_World; }
    [[nodiscard]] const config::Render& render() const { return m_Render; }
    [[nodiscard]] const config::PostProcess& postProcess() const { return m_PostProcess; }

private:
    static void readWindow(const toml::table& t, config::Window& w);
    static void readPlayer(const toml::table& t, config::Player& p);
    static void readCharacterController(const toml::table& t, config::CharacterController& cc);
    static void readCamera(const toml::table& t, config::Camera& c);
    static void readCameraController(const toml::table& t, config::CameraController& cc);
    static void readStats(const toml::table& t, config::Stats& s);
    static void readWorld(const toml::table& t, config::World& w);
    static void readRender(const toml::table& t, config::Render& r);
    static void readPostProcess(const toml::table& t, config::PostProcess& pp);
    static config::WindowMode parseWindowMode(std::string_view s);

    Config() = default;

    config::Window m_Window{};
    config::Player m_Player{};
    config::CharacterController m_CharacterController{};
    config::Camera m_Camera{};
    config::CameraController m_CameraController{};
    config::Stats m_Stats{};
    config::World m_World{};
    config::Render m_Render{};
    config::PostProcess m_PostProcess{};
};

}  // namespace se::core