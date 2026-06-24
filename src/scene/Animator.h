#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <string_view>

#include "assets/Animation.h"
#include "assets/AssetHandle.h"
#include "assets/Model.h"
#include "assets/Pose.h"
#include "assets/Skeleton.h"

namespace se::scene {

class Animator {
public:
    Animator(se::assets::ModelHandle handle, int clipIndex = 0);

    void update(float deltaTime);

    void play();
    void pause();
    void stop();

    void play(int clipIndex);
    void blendTo(int clipIndex, float blendDuration = 0.15f);
    void play(std::string_view clipName);
    void blendTo(std::string_view clipName, float blendDuration = 0.15f);

    void setLooping(bool loop) { m_Loop = loop; }
    void setRootMotionEnabled(bool enabled);

    [[nodiscard]] const se::assets::BonePalette& bones() const { return m_Bones; }
    [[nodiscard]] int boneCount() const;
    [[nodiscard]] bool isPlaying() const { return m_Playing; }
    [[nodiscard]] int clipIndex() const { return m_ClipIndex; }
    [[nodiscard]] float currentTime() const { return m_CurrentTime; }
    [[nodiscard]] float duration() const;
    [[nodiscard]] bool isRootMotionEnabled() const { return m_UseRootMotion; }
    [[nodiscard]] const glm::vec3& rootMotionDelta() const { return m_RootMotionDelta; }

private:
    void buildBonePalette(const se::assets::Pose& pose);
    void evaluateCurrentPose();

    [[nodiscard]] const se::assets::Skeleton* skeleton() const;
    [[nodiscard]] const se::assets::AnimationClip* clip() const;

    se::assets::ModelHandle m_Model;
    int m_ClipIndex = 0;

    float m_CurrentTime = 0.0f;
    glm::vec3 m_RootMotionDelta{0.0f};

    bool m_Loop = true;
    bool m_Playing = true;
    bool m_UseRootMotion = false;

    // Previous pose for blending
    se::assets::Pose m_BlendFromPose{};
    float m_BlendTimer = 0.0f;
    float m_BlendDuration = 0.0f;

    // Current sampled pose
    se::assets::Pose m_LocalPose{};

    // Final bones to be sent to the shader
    se::assets::BonePalette m_Bones{};
};

}  // namespace se::scene