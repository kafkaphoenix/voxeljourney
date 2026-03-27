#pragma once
#include <optional>
#include <string>
#include <string_view>

#include "Config.h"
#include "render/RenderStats.h"

namespace se::core {

class StatsTracker {
   public:
    explicit StatsTracker(const Config::Stats& config);

    void setEnabled(bool enabled) { m_Enabled = enabled; }
    bool enabled() const { return m_Enabled; }
    void setRefreshInterval(float seconds) { m_RefreshInterval = seconds; }
    void reset();

    std::optional<std::string> update(float deltaTime,
                                      const se::render::RenderStats& renderStats,
                                      std::string_view title);

   private:
    bool m_Enabled = true;
    float m_RefreshInterval = 1.0f;
    float m_Timer = 0.0f;
    int m_Frames = 0;
};

}  // namespace se::core
