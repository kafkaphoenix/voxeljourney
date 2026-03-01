#include "Config.h"

#include <SimpleIni.h>

#include <charconv>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>

namespace se::core {

namespace {

std::string formatKey(const char* section, const char* key) {
    return std::string("[") + section + "] " + key;
}

[[noreturn]]
void throwConfigError(const std::string& msg) {
    throw std::runtime_error("Config error: " + msg);
}

bool isAllWhitespace(const char* value) {
    for (; *value; ++value)
        if (!std::isspace(static_cast<unsigned char>(*value)))
            return false;
    return true;
}

template<typename T>
bool parseNumber(const char* str, T& out) {
    const char* end = str + std::strlen(str);
    auto [ptr, ec] = std::from_chars(str, end, out);
    return ec == std::errc() && ptr == end;
}

bool parseBoolToken(const char* value, bool& out) {
    auto eq = [&](const char* s) {
        const char* v = value;
        for (; *s && *v; ++s, ++v)
            if (std::tolower(*s) != std::tolower(*v))
                return false;
        return *s == 0 && *v == 0;
    };

    if (eq("1") || eq("true") || eq("yes") || eq("on")) {
        out = true;
        return true;
    }
    if (eq("0") || eq("false") || eq("no") || eq("off")) {
        out = false;
        return true;
    }
    return false;
}

std::string requireValue(const CSimpleIniA& ini, const char* section, const char* key) {
    const char* v = ini.GetValue(section, key, nullptr);
    if (!v || isAllWhitespace(v))
        throwConfigError("missing key " + formatKey(section, key));
    return v;
}

template<typename T>
T readNumber(const CSimpleIniA& ini, const char* section, const char* key) {
    std::string value = requireValue(ini, section, key);
    T parsed{};
    if (!parseNumber(value.c_str(), parsed))
        throwConfigError(formatKey(section,key) + " value '" + value + "' invalid number");
    return parsed;
}

bool readBool(const CSimpleIniA& ini, const char* section, const char* key) {
    std::string value = requireValue(ini, section, key);
    bool parsed{};
    if (!parseBoolToken(value.c_str(), parsed))
        throwConfigError(formatKey(section,key) + " value '" + value + "' invalid bool");
    return parsed;
}

std::string readString(const CSimpleIniA& ini, const char* section, const char* key) {
    return requireValue(ini, section, key);
}

template<typename T>
void requireRange(const char* section,const char* key,T v,T min,T max){
    if(v < min || v > max)
        throwConfigError(formatKey(section,key) + " out of range");
}

template<typename T>
void requireGreater(const char* section,const char* key,T v,T min){
    if(v <= min)
        throwConfigError(formatKey(section,key) + " must be > " + std::to_string(min));
}

} // namespace

void Config::readWindow(const CSimpleIniA& ini, Window& w) {
    w.title = readString(ini,"window","title");
    w.width = readNumber<int>(ini,"window","width");
    w.height = readNumber<int>(ini,"window","height");
    w.vsync = readBool(ini,"window","vsync");
    w.startFullscreen = readBool(ini,"window","startFullscreen");
    w.glDebugNotifications = readBool(ini,"window","glDebugNotifications");

    requireGreater("window","width",w.width,0);
    requireGreater("window","height",w.height,0);
}

void Config::readInput(const CSimpleIniA& ini, Input& i) {
    i.mouseSmoothAlpha = readNumber<float>(ini,"input","mouseSmoothAlpha");
    i.mouseSensitivity = readNumber<float>(ini,"input","mouseSensitivity");
    i.fixedStep = readNumber<float>(ini,"input","fixedStep");

    requireRange("input","mouseSmoothAlpha",i.mouseSmoothAlpha,0.f,1.f);
    requireGreater("input","mouseSensitivity",i.mouseSensitivity,0.f);
    requireGreater("input","fixedStep",i.fixedStep,0.f);
}

void Config::readCamera(const CSimpleIniA& ini, Camera& c) {
    c.moveSpeed = readNumber<float>(ini,"camera","moveSpeed");
    c.fov = readNumber<float>(ini,"camera","fov");
    c.nearPlane = readNumber<float>(ini,"camera","nearPlane");
    c.farPlane = readNumber<float>(ini,"camera","farPlane");
    c.startPosX = readNumber<float>(ini,"camera","startPosX");
    c.startPosY = readNumber<float>(ini,"camera","startPosY");
    c.startPosZ = readNumber<float>(ini,"camera","startPosZ");

    requireGreater("camera","moveSpeed",c.moveSpeed,0.f);
    requireRange("camera","fov",c.fov,1.f,179.f);
    requireGreater("camera","nearPlane",c.nearPlane,0.f);

    if (c.farPlane <= c.nearPlane)
        throwConfigError("[camera] farPlane must be > nearPlane");
}

void Config::readStats(const CSimpleIniA& ini, Stats& s) {
    s.showStats = readBool(ini,"stats","showStats");
    s.interval = readNumber<float>(ini,"stats","interval");

    requireGreater("stats","interval",s.interval,0.f);
}

Config Config::load(const std::string& path) {
    Config cfg;

    CSimpleIniA ini;
    ini.SetUnicode();

    if (ini.LoadFile(path.c_str()) < 0)
        throwConfigError("failed to load file '" + path + "'");

    readWindow(ini, cfg.m_Window);
    readInput(ini, cfg.m_Input);
    readCamera(ini, cfg.m_Camera);
    readStats(ini, cfg.m_Stats);

    return cfg;
}

} // namespace se::core