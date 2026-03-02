#pragma once

#include <optional>
#include <string>

#include "Config.h"
#include "render/Renderer.h"

namespace se::core {

class StatsTracker {
   public:
    explicit StatsTracker(const Config::Stats& config);

    void setEnabled(bool enabled) { m_Enabled = enabled; }
    bool enabled() const { return m_Enabled; }
    void setInterval(float seconds) { m_Interval = seconds; }
    void reset();

    std::optional<std::string> update(float deltaTime,
                                      const se::render::Renderer::Stats& renderStats,
                                      const std::string& baseTitle);

   private:
    bool m_Enabled = true;
    float m_Interval = 0.25f;
    float m_Timer = 0.0f;
    int m_Frames = 0;
};

}  // namespace se::core
