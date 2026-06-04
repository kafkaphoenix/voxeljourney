#include "StatsTracker.h"

#include <format>

#include "MemoryUtils.h"

namespace se::core {

StatsTracker::StatsTracker(const config::Stats& config)
    : m_Enabled(config.enabled), m_RefreshInterval(config.refreshInterval) {}

void StatsTracker::reset() {
    m_Timer = 0.0f;
    m_Frames = 0;
    m_UpdateMsSum = 0.0f;
    m_RenderMsSum = 0.0f;
    m_FrameMsSum = 0.0f;
}

void StatsTracker::cycleDisplayMode() {
    switch (m_DisplayMode) {
    case DisplayMode::Basic: m_DisplayMode = DisplayMode::TimingAverages; break;
    case DisplayMode::TimingAverages: m_DisplayMode = DisplayMode::Basic; break;
    }
}

std::optional<std::string> StatsTracker::update(float deltaTime, const se::render::RenderStats& renderStats,
                                                const FrameDebugStats& frameDebugStats, std::string_view title) {
    if (!m_Enabled) {
        return std::nullopt;
    }

    m_Timer += deltaTime;
    m_Frames++;
    m_UpdateMsSum += frameDebugStats.updateMs;
    m_RenderMsSum += frameDebugStats.renderMs;
    m_FrameMsSum += frameDebugStats.frameMs;
    if (m_Timer < m_RefreshInterval) {
        return std::nullopt;
    }

    float fps = static_cast<float>(m_Frames) / m_Timer;
    float avgUpdateMs = m_UpdateMsSum / static_cast<float>(m_Frames);
    float avgRenderMs = m_RenderMsSum / static_cast<float>(m_Frames);
    float avgFrameMs = m_FrameMsSum / static_cast<float>(m_Frames);
    ProcessMemoryUsage mem = getProcessMemoryUsageKB();
    std::string stats;
    switch (m_DisplayMode) {
    case DisplayMode::Basic:
        stats = std::format(
            "{} FPS: {} RAM: {}/{}MB | Models: Draws {} Triangles {} | Animated Models: Draws {} Triangles {} | "
            "Chunks: "
            "Draws {} Triangles {}",
            title, static_cast<int>(fps), mem.usedKB / 1024, mem.committedKB / 1024, renderStats.modelsDrawCalls,
            renderStats.modelsTriangles, renderStats.animatedModelsDrawCalls, renderStats.animatedModelsTriangles,
            renderStats.chunksDrawCalls, renderStats.chunksTriangles);
        break;
    case DisplayMode::TimingAverages:
        stats = std::format("{} FPS: {} RAM: {}/{}MB | Avg ms: Update {:.2f} Render {:.2f} Frame {:.2f}", title,
                            static_cast<int>(fps), mem.usedKB / 1024, mem.committedKB / 1024, avgUpdateMs, avgRenderMs,
                            avgFrameMs);
        break;
    }

    reset();
    return stats;
}

}  // namespace se::core
