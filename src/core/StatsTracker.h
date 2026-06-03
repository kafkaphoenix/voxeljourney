#pragma once
#include <optional>
#include <string>
#include <string_view>

#include "Config.h"
#include "render/RenderStats.h"

namespace se::core {

class StatsTracker {
public:
    explicit StatsTracker(const config::Stats& config);

    void setEnabled(bool enabled) { m_Enabled = enabled; }
    [[nodiscard]] bool enabled() const { return m_Enabled; }
    void setRefreshInterval(float seconds) { m_RefreshInterval = seconds; }
    [[nodiscard]] float getRefreshInterval() const { return m_RefreshInterval; }
    void reset();

    std::optional<std::string> update(float deltaTime, const se::render::RenderStats& renderStats,
                                      std::string_view title);

private:
    bool m_Enabled = true;
    float m_RefreshInterval = 1.0f;
    float m_Timer = 0.0f;
    int m_Frames = 0;
};

}  // namespace se::core
