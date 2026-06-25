#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "Config.h"
#include "render/RenderStats.h"

namespace se::core {

struct FrameDebugStats {
    float updateMs = 0.0f;
    float renderMs = 0.0f;
    float frameMs = 0.0f;
};

class StatsTracker {
public:
    enum class DisplayMode : uint8_t { Basic, TimingAverages };

    explicit StatsTracker(const config::Stats& config);

    void setEnabled(bool enabled) { m_Enabled = enabled; }
    [[nodiscard]] bool enabled() const { return m_Enabled; }
    void setRefreshInterval(float seconds) { m_RefreshInterval = seconds; }
    [[nodiscard]] float getRefreshInterval() const { return m_RefreshInterval; }
    void reset();
    void cycleDisplayMode();
    [[nodiscard]] DisplayMode displayMode() const { return m_DisplayMode; }

    std::optional<std::string> update(float deltaTime, const se::render::RenderStats& renderStats,
                                      const FrameDebugStats& frameDebugStats, std::string_view title);

private:
    DisplayMode m_DisplayMode = DisplayMode::Basic;
    bool m_Enabled = true;
    float m_RefreshInterval = 1.0f;
    float m_Timer = 0.0f;
    int m_Frames = 0;
    float m_UpdateMsSum = 0.0f;
    float m_RenderMsSum = 0.0f;
    float m_FrameMsSum = 0.0f;
};

}  // namespace se::core
