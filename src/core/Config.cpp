#include "Config.h"

#include <format>
#include <stdexcept>

using namespace se::core::config;

namespace se::core {

namespace {

[[noreturn]] void throwConfigError(std::string_view msg) {
    throw std::runtime_error(std::format("Config error: {}", msg));
}

template <typename T>
T require(const toml::table& t, std::string_view key) {
    auto node = t[key];
    if (!node) {
        throwConfigError(std::format("missing key '{}'", key));
    }

    auto value = node.value<T>();
    if (!value) {
        throwConfigError(std::format("invalid type for key '{}'", key));
    }

    return *value;
}

template <typename T>
concept GlmVec = requires {
    typename T::value_type;
    { T::length() } -> std::convertible_to<glm::length_t>;
}
&&std::is_same_v<typename T::value_type, float>;

template <GlmVec T>
T require(const toml::table& t, std::string_view key) {
    constexpr glm::length_t N = T::length();
    auto node = t[key];
    if (!node)
        throwConfigError(std::format("missing key '{}'", key));

    const auto* arr = node.as_array();
    if (!arr || static_cast<glm::length_t>(arr->size()) != N)
        throwConfigError(std::format("'{}' must be an array with {} elements", key, N));

    T result;
    for (glm::length_t i = 0; i < N; ++i) {
        if (auto v = arr->get_as<double>(i))
            result[i] = static_cast<float>(v->get());
        else if (auto v = arr->get_as<int64_t>(i))
            result[i] = static_cast<float>(v->get());
        else
            throwConfigError(std::format("'{}[{}]' must be a number", key, i));
    }
    return result;
}

template <typename T>
T optional(const toml::table& t, std::string_view key, T fallback) {
    auto node = t[key];
    if (!node) {
        return fallback;
    }

    auto value = node.value<T>();
    return value ? *value : fallback;
}

template <typename T>
requires std::is_arithmetic_v<T>
void requireRange(std::string_view key, T v, T min, T max) {
    if (v < min || v > max) {
        throwConfigError(std::format("'{}' out of range", key));
    }
}

template <typename T>
requires std::is_arithmetic_v<T>
void requireGreater(std::string_view key, T v, T min) {
    if (v <= min) {
        throwConfigError(std::format("'{}' must be > {}", key, min));
    }
}

const toml::table& requireTable(const toml::table& t, std::string_view key) {
    auto* table = t[key].as_table();

    if (!table) {
        throwConfigError(std::format("missing table '[{}]'", key));
    }

    return *table;
}

}  // namespace

WindowMode Config::parseWindowMode(std::string_view s) {
    if (s == "windowed") {
        return WindowMode::Windowed;
    }
    if (s == "borderless") {
        return WindowMode::Borderless;
    }
    if (s == "fullscreen") {
        return WindowMode::Fullscreen;
    }

    throwConfigError("invalid window.mode (windowed | borderless | fullscreen)");
}

Config Config::load(std::string_view path) {
    Config cfg;

    toml::table t;
    try {
        t = toml::parse_file(std::string(path));
    } catch (const toml::parse_error& e) { throwConfigError(e.description()); }

    readWindow(t, cfg.m_Window);
    readPlayer(t, cfg.m_Player);
    readCamera(t, cfg.m_Camera);
    readCharacterController(t, cfg.m_CharacterController);
    readThirdPersonCameraController(t, cfg.m_ThirdPersonCameraController);
    readStats(t, cfg.m_Stats);
    readWorld(t, cfg.m_World);
    readRender(t, cfg.m_Render);

    return cfg;
}

void Config::readWindow(const toml::table& t, Window& w) {
    const auto& tw = requireTable(t, "window");

    w.title = require<std::string>(tw, "title");
    w.width = require<int>(tw, "width");
    w.height = require<int>(tw, "height");
    w.posX = require<int>(tw, "posX");
    w.posY = require<int>(tw, "posY");
    w.vsync = require<bool>(tw, "vsync");

    w.mode = parseWindowMode(require<std::string>(tw, "mode"));

    requireGreater("window.width", w.width, 0);
    requireGreater("window.height", w.height, 0);
}

void Config::readPlayer(const toml::table& t, Player& p) {
    const auto& tp = requireTable(t, "player");

    p.spawn = require<glm::vec3>(tp, "spawn");
}

void Config::readCharacterController(const toml::table& t, CharacterController& cc) {
    const auto& tm = requireTable(t, "characterController");

    cc.walkSpeed = require<float>(tm, "walkSpeed");
    cc.runSpeed = require<float>(tm, "runSpeed");
    cc.mouseSensitivity = require<float>(tm, "mouseSensitivity");
    cc.mouseSmoothAlpha = optional<float>(tm, "mouseSmoothAlpha", 0.5f);
    cc.useFixedStep = optional<bool>(tm, "useFixedStep", true);
    cc.fixedHz = cc.useFixedStep ? require<float>(tm, "fixedHz") : optional<float>(tm, "fixedHz", 60.f);

    requireGreater("characterController.walkSpeed", cc.walkSpeed, 0.f);
    requireGreater("characterController.runSpeed", cc.runSpeed, 0.f);
    requireGreater("characterController.mouseSensitivity", cc.mouseSensitivity, 0.f);
    requireRange("characterController.fixedHz", cc.fixedHz, 1.f, 240.f);

    if (cc.walkSpeed >= cc.runSpeed) {
        throwConfigError("characterController.walkSpeed must be < characterController.runSpeed");
    }
}

void Config::readCamera(const toml::table& t, Camera& c) {
    const auto& tc = requireTable(t, "camera");

    c.fov = require<float>(tc, "fov");
    c.nearPlane = require<float>(tc, "nearPlane");
    c.farPlane = require<float>(tc, "farPlane");
    c.aspectRatio = optional<float>(tc, "aspectRatio", 16.f / 9.f);

    requireRange("camera.fov", c.fov, 1.f, 179.f);
    requireGreater("camera.nearPlane", c.nearPlane, 0.f);
    requireGreater("camera.farPlane", c.farPlane, 0.f);
    requireGreater("camera.aspectRatio", c.aspectRatio, 0.f);

    if (c.farPlane <= c.nearPlane) {
        throwConfigError("camera.farPlane must be > nearPlane");
    }
}

void Config::readThirdPersonCameraController(const toml::table& t, ThirdPersonCameraController& cc) {
    const auto& tcc = requireTable(t, "thirdPersonCameraController");

    cc.followDistance = require<float>(tcc, "followDistance");
    cc.followHeight = require<float>(tcc, "followHeight");

    requireGreater("thirdPersonCameraController.followDistance", cc.followDistance, 0.f);
    requireGreater("thirdPersonCameraController.followHeight", cc.followHeight, 0.f);
}

void Config::readStats(const toml::table& t, Stats& s) {
    const auto& ts = requireTable(t, "stats");

    s.enabled = require<bool>(ts, "enabled");
    s.refreshInterval = require<float>(ts, "refreshInterval");

    requireGreater("stats.refreshInterval", s.refreshInterval, 0.f);
}

void Config::readWorld(const toml::table& t, World& w) {
    const auto& tw = *t["world"].as_table();

    w.renderDistance = require<int>(tw, "renderDistance");

    requireGreater("world.renderDistance", w.renderDistance, 0);
}

void Config::readRender(const toml::table& t, Render& r) {
    const auto& tr = requireTable(t, "render");

    r.msaaSamples = require<int>(tr, "msaaSamples");
    r.anisotropy = require<float>(tr, "anisotropy");

    requireRange("render.msaaSamples", r.msaaSamples, 1, 16);
    requireRange("render.anisotropy", r.anisotropy, 1.0f, 16.0f);
}

}  // namespace se::core