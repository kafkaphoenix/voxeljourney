#include "StatsTracker.h"

#include "MemoryUtils.h"

namespace se::core {

StatsTracker::StatsTracker(const Config::Stats& config)
    : m_Enabled(config.showStats),
      m_Interval(config.interval) {}

void StatsTracker::reset() {
    m_Timer = 0.0f;
    m_Frames = 0;
}

std::optional<std::string> StatsTracker::update(float deltaTime,
                                                const se::render::Renderer::Stats& renderStats,
                                                const std::string& baseTitle) {
    if (!m_Enabled) {
        return std::nullopt;
    }

    m_Timer += deltaTime;
    m_Frames++;
    if (m_Timer < m_Interval) {
        return std::nullopt;
    }

    float fps = m_Frames / m_Timer;
    size_t memKB = getProcessMemoryUsageKB();
    std::string title = baseTitle +
                        " | FPS: " + std::to_string(static_cast<int>(fps)) +
                        " | Draws: " + std::to_string(renderStats.drawCalls) +
                        " | Triangles: " + std::to_string(renderStats.triangles) +
                        " | RAM: " + std::to_string(memKB / 1024) + "MB";

    reset();
    return title;
}

}  // namespace se::core
