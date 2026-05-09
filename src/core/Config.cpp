#include "Config.h"

#include <SimpleIni.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstring>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace se::core {

namespace {

std::string formatKey(std::string_view section, std::string_view key) { return std::format("[{}] {}", section, key); }

void throwConfigError(std::string_view msg) { throw std::runtime_error(std::format("Config error: {}", msg)); }

bool isAllWhitespace(std::string_view value) {
    return std::ranges::all_of(value, [](unsigned char ch) { return std::isspace(ch) != 0; });
}

template <typename T>
std::optional<T> parseNumber(std::string_view str) {
    T out{};
    const char* begin = str.data();
    const char* end = begin + str.size();
    auto [ptr, ec] = std::from_chars(begin, end, out);
    if (ec != std::errc() || ptr != end) {
        return std::nullopt;
    }
    return out;
}

std::optional<bool> parseBoolToken(std::string_view value) {
    auto eq = [&](std::string_view token) {
        if (token.size() != value.size()) {
            return false;
        }
        auto valueIt = value.begin();
        for (auto tokenCh : token) {
            auto left = static_cast<unsigned char>(tokenCh);
            auto right = static_cast<unsigned char>(*valueIt);
            if (std::tolower(left) != std::tolower(right)) {
                return false;
            }
            ++valueIt;
        }
        return true;
    };

    if (eq("1") || eq("true") || eq("yes") || eq("on")) {
        return true;
    }
    if (eq("0") || eq("false") || eq("no") || eq("off")) {
        return false;
    }
    return std::nullopt;
}

std::string requireValue(const CSimpleIniA& ini, std::string_view section, std::string_view key) {
    std::string sectionStr(section);
    std::string keyStr(key);
    const char* v = ini.GetValue(sectionStr.c_str(), keyStr.c_str(), nullptr);
    if (v == nullptr || isAllWhitespace(v)) {
        throwConfigError(std::format("missing key {}", formatKey(section, key)));
    }
    return v;
}

template <typename T>
T readNumber(const CSimpleIniA& ini, std::string_view section, std::string_view key) {
    std::string value = requireValue(ini, section, key);
    auto parsed = parseNumber<T>(value);
    if (!parsed) {
        throwConfigError(std::format("{} value '{}' invalid number", formatKey(section, key), value));
    }
    return parsed.value();
}

bool readBool(const CSimpleIniA& ini, std::string_view section, std::string_view key) {
    std::string value = requireValue(ini, section, key);
    auto parsed = parseBoolToken(value);
    if (!parsed) {
        throwConfigError(std::format("{} value '{}' invalid bool", formatKey(section, key), value));
    }
    return parsed.value();
}

std::string readString(const CSimpleIniA& ini, std::string_view section, std::string_view key) {
    return requireValue(ini, section, key);
}

template <typename T>
void requireRange(std::string_view section, std::string_view key, T v, T min, T max) {
    if (v < min || v > max) {
        throwConfigError(std::format("{} out of range", formatKey(section, key)));
    }
}

template <typename T>
void requireGreater(std::string_view section, std::string_view key, T v, T min) {
    if (v <= min) {
        throwConfigError(std::format("{} must be > {}", formatKey(section, key), min));
    }
}

}  // namespace

void Config::readWindow(const CSimpleIniA& ini, Window& w) {
    w.title = readString(ini, "window", "title");
    w.width = readNumber<int>(ini, "window", "width");
    w.height = readNumber<int>(ini, "window", "height");
    w.posX = readNumber<int>(ini, "window", "posX");
    w.posY = readNumber<int>(ini, "window", "posY");
    w.vsync = readBool(ini, "window", "vsync");
    w.mode = readString(ini, "window", "mode");

    requireGreater("window", "width", w.width, 0);
    requireGreater("window", "height", w.height, 0);
    if (w.mode != "windowed" && w.mode != "borderless" && w.mode != "fullscreen") {
        throwConfigError(formatKey("window", "mode") + " must be 'windowed', 'borderless', or 'fullscreen'");
    }
}

void Config::readInput(const CSimpleIniA& ini, Input& i) {
    i.mouseSmoothAlpha = readNumber<float>(ini, "input", "mouseSmoothAlpha");

    requireRange("input", "mouseSmoothAlpha", i.mouseSmoothAlpha, 0.f, 1.f);
}

void Config::readPlayer(const CSimpleIniA& ini, Player& p) {
    p.moveSpeed = readNumber<float>(ini, "player", "moveSpeed");
    p.sensitivity = readNumber<float>(ini, "player", "sensitivity");
    p.fixedStep = readNumber<float>(ini, "player", "fixedStep");
    p.cameraHeight = readNumber<float>(ini, "player", "cameraHeight");
    p.cameraDistance = readNumber<float>(ini, "player", "cameraDistance");
    p.startPosition.x = readNumber<float>(ini, "player", "startPosX");
    p.startPosition.y = readNumber<float>(ini, "player", "startPosY");
    p.startPosition.z = readNumber<float>(ini, "player", "startPosZ");

    requireGreater("player", "moveSpeed", p.moveSpeed, 0.f);
    requireGreater("player", "sensitivity", p.sensitivity, 0.f);
    requireGreater("player", "fixedStep", p.fixedStep, 0.f);
}

void Config::readCamera(const CSimpleIniA& ini, Camera& c) {
    c.fov = readNumber<float>(ini, "camera", "fov");
    c.nearPlane = readNumber<float>(ini, "camera", "nearPlane");
    c.farPlane = readNumber<float>(ini, "camera", "farPlane");
    c.aspectRatio = readNumber<float>(ini, "camera", "aspectRatio");

    requireRange("camera", "fov", c.fov, 1.f, 179.f);
    requireGreater("camera", "nearPlane", c.nearPlane, 0.f);
    requireGreater("camera", "aspectRatio", c.aspectRatio, 0.f);

    if (c.farPlane <= c.nearPlane) {
        throwConfigError("[camera] farPlane must be > nearPlane");
    }
}

void Config::readStats(const CSimpleIniA& ini, Stats& s) {
    s.enabled = readBool(ini, "stats", "enabled");
    s.refreshInterval = readNumber<float>(ini, "stats", "refreshInterval");

    requireGreater("stats", "refreshInterval", s.refreshInterval, 0.f);
}

void Config::readWorld(const CSimpleIniA& ini, World& w) {
    w.renderDistance = readNumber<int>(ini, "world", "renderDistance");

    requireGreater("world", "renderDistance", w.renderDistance, 0);
}

Config Config::load(std::string_view path) {
    Config cfg;

    CSimpleIniA ini;
    ini.SetUnicode();

    std::string pathStr(path);
    if (ini.LoadFile(pathStr.c_str()) < 0) {
        throwConfigError(std::format("failed to load file '{}'", path));
    }

    readWindow(ini, cfg.m_Window);
    readInput(ini, cfg.m_Input);
    readPlayer(ini, cfg.m_Player);
    readCamera(ini, cfg.m_Camera);
    readStats(ini, cfg.m_Stats);
    readWorld(ini, cfg.m_World);

    return cfg;
}

}  // namespace se::core