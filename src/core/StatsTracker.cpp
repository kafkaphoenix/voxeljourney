#include "StatsTracker.h"

#include <format>

#include "MemoryUtils.h"

namespace se::core {

StatsTracker::StatsTracker(const Config::Stats& config)
    : m_Enabled(config.enabled), m_RefreshInterval(config.refreshInterval) {}

void StatsTracker::reset() {
    m_Timer = 0.0f;
    m_Frames = 0;
}

std::optional<std::string> StatsTracker::update(float deltaTime, const se::render::RenderStats& renderStats,
                                                std::string_view title) {
    if (!m_Enabled) {
        return std::nullopt;
    }

    m_Timer += deltaTime;
    m_Frames++;
    if (m_Timer < m_RefreshInterval) {
        return std::nullopt;
    }

    float fps = static_cast<float>(m_Frames) / m_Timer;
    ProcessMemoryUsage mem = getProcessMemoryUsageKB();
    std::string stats = std::format(
        "{} FPS: {} RAM: {}/{}MB | Models: Draws {} Triangles {} | Animated Models: Draws {} Triangles {} | Chunks: "
        "Draws {} Triangles {}",
        title, static_cast<int>(fps), mem.usedKB / 1024, mem.committedKB / 1024, renderStats.modelsDrawCalls,
        renderStats.modelsTriangles, renderStats.animatedModelsDrawCalls, renderStats.animatedModelsTriangles,
        renderStats.chunksDrawCalls, renderStats.chunksTriangles);

    reset();
    return stats;
}

}  // namespace se::core
