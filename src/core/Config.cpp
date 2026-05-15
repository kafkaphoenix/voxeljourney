#include "Config.h"

#include <format>
#include <stdexcept>

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
    readInput(t, cfg.m_Input);
    readPlayer(t, cfg.m_Player);
    readCamera(t, cfg.m_Camera);
    readStats(t, cfg.m_Stats);
    readWorld(t, cfg.m_World);
    readRender(t, cfg.m_Render);

    return cfg;
}

void Config::readWindow(const toml::table& t, Window& w) {
    const auto& tw = *t["window"].as_table();

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

void Config::readInput(const toml::table& t, Input& i) {
    const auto& ti = *t["input"].as_table();

    i.mouseSmoothAlpha = require<float>(ti, "mouseSmoothAlpha");
    requireRange("input.mouseSmoothAlpha", i.mouseSmoothAlpha, 0.f, 1.f);
}

void Config::readPlayer(const toml::table& t, Player& p) {
    const auto& tp = *t["player"].as_table();

    p.walkSpeed = require<float>(tp, "walkSpeed");
    p.runSpeed = require<float>(tp, "runSpeed");
    p.mouseSensitivity = require<float>(tp, "mouseSensitivity");
    p.useFixedStep = optional<bool>(tp, "useFixedStep", true);
    p.fixedHz = p.useFixedStep ? require<float>(tp, "fixedHz") : optional<float>(tp, "fixedHz", 60.f);

    p.cameraHeight = require<float>(tp, "cameraHeight");
    p.cameraDistance = require<float>(tp, "cameraDistance");

    p.startPosition.x = require<float>(tp, "startPosX");
    p.startPosition.y = require<float>(tp, "startPosY");
    p.startPosition.z = require<float>(tp, "startPosZ");

    requireGreater("player.walkSpeed", p.walkSpeed, 0.f);
    requireGreater("player.runSpeed", p.runSpeed, 0.f);
    requireGreater("player.mouseSensitivity", p.mouseSensitivity, 0.f);
    requireRange("player.fixedHz", p.fixedHz, 1.f, 240.f);
    requireGreater("player.cameraHeight", p.cameraHeight, 0.f);
    requireGreater("player.cameraDistance", p.cameraDistance, 0.f);

    if (p.walkSpeed >= p.runSpeed) {
        throwConfigError("player.walkSpeed must be < player.runSpeed");
    }
}

void Config::readCamera(const toml::table& t, Camera& c) {
    const auto& tc = *t["camera"].as_table();

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

void Config::readStats(const toml::table& t, Stats& s) {
    const auto& ts = *t["stats"].as_table();

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
    if (const auto* tr = t["render"].as_table()) {
        r.msaaSamples = optional<int>(*tr, "msaaSamples", 4);
        r.anisotropy = optional<float>(*tr, "anisotropy", 4.0f);
    } else {
        r.msaaSamples = 4;
        r.anisotropy = 4.0f;
    }

    requireRange("render.msaaSamples", r.msaaSamples, 1, 16);
    requireRange("render.anisotropy", r.anisotropy, 1.0f, 16.0f);
}

}  // namespace se::core