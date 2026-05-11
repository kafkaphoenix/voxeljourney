#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include "assets/Animation.h"
#include "assets/AssetHandle.h"
#include "assets/Model.h"
#include "assets/Skeleton.h"

namespace se::scene {

struct BonePose {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

class Animator {
public:
    Animator(se::assets::ModelHandle model, int clipIndex = 0);

    void update(float deltaTime);

    void play();
    void pause();
    void stop();

    void setClip(int clipIndex);
    void setClipFade(int clipIndex, float blendDuration = 0.15f);

    void setLooping(bool loop) { m_Loop = loop; }

    [[nodiscard]] const std::array<glm::mat4, se::assets::MAX_BONES>& boneMatrices() const { return m_BoneMatrices; }
    [[nodiscard]] int boneCount() const;
    [[nodiscard]] bool isPlaying() const { return m_Playing; }
    [[nodiscard]] int clipIndex() const { return m_ClipIndex; }
    [[nodiscard]] float currentTime() const { return m_CurrentTime; }
    [[nodiscard]] float duration() const;

private:
    void calculateBoneTransforms();

    // Sample the animation clip at the given time and write the resulting pose to outPose.
    void samplePose(std::array<BonePose, se::assets::MAX_BONES>& outPose, int clipIndex, float time) const;

    void buildMatricesFromPose(const std::array<BonePose, se::assets::MAX_BONES>& pose);

    static glm::vec3 interpolatePosition(const se::assets::AnimationChannel& channel, float time);
    static glm::quat interpolateRotation(const se::assets::AnimationChannel& channel, float time);
    static glm::vec3 interpolateScale(const se::assets::AnimationChannel& channel, float time);

    [[nodiscard]] const se::assets::Skeleton* skeleton() const;
    [[nodiscard]] const se::assets::AnimationClip* clip() const;

    se::assets::ModelHandle m_Model;
    std::array<glm::mat4, se::assets::MAX_BONES> m_LocalMatrices{};
    std::array<glm::mat4, se::assets::MAX_BONES> m_GlobalMatrices{};

    int m_ClipIndex = 0;

    float m_CurrentTime = 0.0f;

    bool m_Loop = true;
    bool m_Playing = true;

    // Previous pose for blending
    std::array<BonePose, se::assets::MAX_BONES> m_BlendFromPose{};
    float m_BlendTimer = 0.0f;
    float m_BlendDuration = 0.0f;

    // Current sampled pose
    std::array<BonePose, se::assets::MAX_BONES> m_LocalPose{};

    // Final bone matrices to be sent to the shader, calculated from the current pose and the skeleton's inverse bind
    // matrices.
    std::array<glm::mat4, se::assets::MAX_BONES> m_BoneMatrices{};
};

}  // namespace se::scene