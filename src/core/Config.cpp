#include "Config.h"

#include <SimpleIni.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstring>
#include <expected>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

namespace se::core {

namespace {

std::string formatKey(std::string_view section, std::string_view key) {
    return std::format("[{}] {}", section, key);
}

[[noreturn]]
void throwConfigError(std::string_view msg) {
    throw std::runtime_error(std::format("Config error: {}", msg));
}

bool isAllWhitespace(std::string_view value) {
    return std::ranges::all_of(value, [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
}

template <typename T>
std::expected<T, std::string> parseNumber(std::string_view str) {
    T out{};
    const char* begin = str.data();
    const char* end = begin + str.size();
    auto [ptr, ec] = std::from_chars(begin, end, out);
    if (ec != std::errc() || ptr != end) {
        return std::unexpected(std::string("invalid number"));
    }
    return out;
}

std::expected<bool, std::string> parseBoolToken(std::string_view value) {
    auto eq = [&](std::string_view token) {
        if (token.size() != value.size()) {
            return false;
        }
        for (size_t i = 0; i < token.size(); ++i) {
            unsigned char left = static_cast<unsigned char>(token[i]);
            unsigned char right = static_cast<unsigned char>(value[i]);
            if (std::tolower(left) != std::tolower(right)) {
                return false;
            }
        }
        return true;
    };

    if (eq("1") || eq("true") || eq("yes") || eq("on")) {
        return true;
    }
    if (eq("0") || eq("false") || eq("no") || eq("off")) {
        return false;
    }
    return std::unexpected(std::string("invalid bool"));
}

std::string requireValue(const CSimpleIniA& ini, std::string_view section, std::string_view key) {
    std::string sectionStr(section);
    std::string keyStr(key);
    const char* v = ini.GetValue(sectionStr.c_str(), keyStr.c_str(), nullptr);
    if (!v || isAllWhitespace(v))
        throwConfigError(std::format("missing key {}", formatKey(section, key)));
    return v;
}

template <typename T>
T readNumber(const CSimpleIniA& ini, std::string_view section, std::string_view key) {
    std::string value = requireValue(ini, section, key);
    auto parsed = parseNumber<T>(value);
    if (!parsed)
        throwConfigError(std::format("{} value '{}' invalid number", formatKey(section, key), value));
    return *parsed;
}

bool readBool(const CSimpleIniA& ini, std::string_view section, std::string_view key) {
    std::string value = requireValue(ini, section, key);
    auto parsed = parseBoolToken(value);
    if (!parsed)
        throwConfigError(std::format("{} value '{}' invalid bool", formatKey(section, key), value));
    return *parsed;
}

std::string readString(const CSimpleIniA& ini, std::string_view section, std::string_view key) {
    return requireValue(ini, section, key);
}

template <typename T>
void requireRange(std::string_view section, std::string_view key, T v, T min, T max) {
    if (v < min || v > max)
        throwConfigError(std::format("{} out of range", formatKey(section, key)));
}

template <typename T>
void requireGreater(std::string_view section, std::string_view key, T v, T min) {
    if (v <= min)
        throwConfigError(std::format("{} must be > {}", formatKey(section, key), min));
}

}  // namespace

void Config::readWindow(const CSimpleIniA& ini, Window& w) {
    w.title = readString(ini, "window", "title");
    w.width = readNumber<int>(ini, "window", "width");
    w.height = readNumber<int>(ini, "window", "height");
    w.vsync = readBool(ini, "window", "vsync");
    w.startFullscreen = readBool(ini, "window", "startFullscreen");

    requireGreater("window", "width", w.width, 0);
    requireGreater("window", "height", w.height, 0);
}

void Config::readInput(const CSimpleIniA& ini, Input& i) {
    i.mouseSmoothAlpha = readNumber<float>(ini, "input", "mouseSmoothAlpha");
    i.mouseSensitivity = readNumber<float>(ini, "input", "mouseSensitivity");
    i.fixedStep = readNumber<float>(ini, "input", "fixedStep");

    requireRange("input", "mouseSmoothAlpha", i.mouseSmoothAlpha, 0.f, 1.f);
    requireGreater("input", "mouseSensitivity", i.mouseSensitivity, 0.f);
    requireGreater("input", "fixedStep", i.fixedStep, 0.f);
}

void Config::readCamera(const CSimpleIniA& ini, Camera& c) {
    c.moveSpeed = readNumber<float>(ini, "camera", "moveSpeed");
    c.fov = readNumber<float>(ini, "camera", "fov");
    c.nearPlane = readNumber<float>(ini, "camera", "nearPlane");
    c.farPlane = readNumber<float>(ini, "camera", "farPlane");
    c.startPosX = readNumber<float>(ini, "camera", "startPosX");
    c.startPosY = readNumber<float>(ini, "camera", "startPosY");
    c.startPosZ = readNumber<float>(ini, "camera", "startPosZ");

    requireGreater("camera", "moveSpeed", c.moveSpeed, 0.f);
    requireRange("camera", "fov", c.fov, 1.f, 179.f);
    requireGreater("camera", "nearPlane", c.nearPlane, 0.f);

    if (c.farPlane <= c.nearPlane)
        throwConfigError("[camera] farPlane must be > nearPlane");
}

void Config::readStats(const CSimpleIniA& ini, Stats& s) {
    s.showStats = readBool(ini, "stats", "showStats");
    s.interval = readNumber<float>(ini, "stats", "interval");

    requireGreater("stats", "interval", s.interval, 0.f);
}

Config Config::load(std::string_view path) {
    Config cfg;

    CSimpleIniA ini;
    ini.SetUnicode();

    std::string pathStr(path);
    if (ini.LoadFile(pathStr.c_str()) < 0)
        throwConfigError(std::format("failed to load file '{}'", path));

    readWindow(ini, cfg.m_Window);
    readInput(ini, cfg.m_Input);
    readCamera(ini, cfg.m_Camera);
    readStats(ini, cfg.m_Stats);

    return cfg;
}

}  // namespace se::core