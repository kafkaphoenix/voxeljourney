#include "StatsTracker.h"

#include <format>

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
                                                std::string_view title) {
    if (!m_Enabled) {
        return std::nullopt;
    }

    m_Timer += deltaTime;
    m_Frames++;
    if (m_Timer < m_Interval) {
        return std::nullopt;
    }

    float fps = m_Frames / m_Timer;
    ProcessMemoryUsage mem = getProcessMemoryUsageKB();
    std::string stats = std::format(
        "{} | FPS: {} | Draws: {} | Triangles: {} | RAM: {}/{}MB",
        title,
        static_cast<int>(fps),
        renderStats.drawCalls,
        renderStats.triangles,
        mem.usedKB / 1024,
        mem.committedKB / 1024);

    reset();
    return stats;
}

}  // namespace se::core
