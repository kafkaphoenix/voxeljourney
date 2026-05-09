#include "Animator.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>

namespace se::scene {

namespace {

template <typename T>
size_t findKeyframeIndex(const std::vector<se::assets::Keyframe<T>>& keys, float time) {
    if (keys.size() <= 1) {
        return 0;
    }

    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (time < keys[i + 1].time) {
            return i;
        }
    }

    return keys.size() - 2;
}

template <typename T>
float scaleFactor(const std::vector<se::assets::Keyframe<T>>& keys, size_t index, float time) {
    float t0 = keys[index].time;
    float t1 = keys[index + 1].time;

    float dt = t1 - t0;

    if (dt <= 0.0f) {
        return 0.0f;
    }

    return std::clamp((time - t0) / dt, 0.0f, 1.0f);
}

}  // namespace

Animator::Animator(se::assets::ModelHandle model, int clipIndex) : m_Model(model), m_ClipIndex(clipIndex) {
    m_BoneMatrices.fill(glm::mat4{1.0f});

    if (clip()) {
        calculateBoneTransforms();
    }
}

float Animator::duration() const {
    const auto* c = clip();
    return c ? c->duration : 0.0f;
}

void Animator::setClip(int clipIndex) {
    if (m_ClipIndex == clipIndex) {
        return;
    }

    m_ClipIndex = clipIndex;
    m_CurrentTime = 0.0f;
    m_Playing = true;

    calculateBoneTransforms();
}

void Animator::setClipFade(int clipIndex, float blendDuration) {
    if (blendDuration <= 0.0f) {
        setClip(clipIndex);
        return;
    }

    if (m_ClipIndex == clipIndex) {
        return;
    }

    samplePose(m_BlendFromPose, m_ClipIndex, m_CurrentTime);

    m_BlendTimer = blendDuration;
    m_BlendDuration = blendDuration;

    m_ClipIndex = clipIndex;
    m_CurrentTime = 0.0f;
    m_Playing = true;
}

void Animator::update(float deltaTime) {
    const auto* c = clip();

    if (!c || !m_Playing) {
        return;
    }

    m_CurrentTime += deltaTime;

    if (m_CurrentTime >= c->duration) {
        if (m_Loop) {
            m_CurrentTime = std::fmod(m_CurrentTime, c->duration);
        } else {
            m_CurrentTime = c->duration;
            m_Playing = false;
        }
    }

    // Sample the current pose for blending and/or building final matrices
    samplePose(m_LocalPose, m_ClipIndex, m_CurrentTime);

    if (m_BlendTimer > 0.0f) {
        m_BlendTimer = std::max(m_BlendTimer - deltaTime, 0.0f);

        const float t = 1.0f - std::clamp(m_BlendTimer / m_BlendDuration, 0.0f, 1.0f);

        // Blend between the previous pose and the current pose based on t, and build matrices from the blended pose
        std::array<BonePose, se::assets::MAX_BONES> blendedPose{};

        const int n = boneCount();

        for (int i = 0; i < n && i < se::assets::MAX_BONES; ++i) {
            const BonePose& from = m_BlendFromPose[i];
            const BonePose& to = m_LocalPose[i];

            blendedPose[i].translation = glm::mix(from.translation, to.translation, t);

            blendedPose[i].rotation = glm::normalize(glm::slerp(from.rotation, to.rotation, t));

            blendedPose[i].scale = glm::mix(from.scale, to.scale, t);
        }

        buildMatricesFromPose(blendedPose);
    } else {  // No blending, just use the current pose
        buildMatricesFromPose(m_LocalPose);
    }
}

void Animator::play() { m_Playing = true; }

void Animator::pause() { m_Playing = false; }

void Animator::stop() {
    m_Playing = false;
    m_CurrentTime = 0.0f;

    calculateBoneTransforms();
}

const se::assets::Skeleton* Animator::skeleton() const {
    auto ptr = m_Model.get();

    return (ptr && ptr->isAnimated()) ? &ptr->getSkeleton() : nullptr;
}

const se::assets::AnimationClip* Animator::clip() const {
    auto ptr = m_Model.get();

    if (!ptr) {
        return nullptr;
    }

    const auto& anims = ptr->getAnimations();

    if (m_ClipIndex < 0 || m_ClipIndex >= static_cast<int>(anims.size())) {
        return nullptr;
    }

    return &anims[m_ClipIndex];
}

int Animator::boneCount() const {
    const auto* s = skeleton();
    return s ? static_cast<int>(s->bones.size()) : 0;
}

// Calculate final bone matrices from the current local pose and the skeleton's inverse bind matrices, and store them in
// m_BoneMatrices.
void Animator::calculateBoneTransforms() {
    samplePose(m_LocalPose, m_ClipIndex, m_CurrentTime);
    buildMatricesFromPose(m_LocalPose);
}

void Animator::samplePose(std::array<BonePose, se::assets::MAX_BONES>& outPose, int clipIndex, float time) const {
    const auto* s = skeleton();

    if (!s) {
        return;
    }

    const auto& bones = s->bones;

    const int numBones = static_cast<int>(bones.size());

    auto ptr = m_Model.get();

    if (!ptr) {
        return;
    }

    const auto& anims = ptr->getAnimations();

    if (clipIndex < 0 || clipIndex >= static_cast<int>(anims.size())) {
        return;
    }

    const auto& clip = anims[clipIndex];

    // Start from rest pose
    for (int i = 0; i < numBones && i < se::assets::MAX_BONES; ++i) {
        outPose[i].translation = bones[i].restPosition;
        outPose[i].rotation = bones[i].restRotation;
        outPose[i].scale = bones[i].restScale;
    }

    // Override animated channels
    for (const auto& channel : clip.channels) {
        if (channel.boneIndex < 0 || channel.boneIndex >= numBones) {
            continue;
        }

        if (!channel.translations.empty()) {
            outPose[channel.boneIndex].translation = interpolatePosition(channel, time);
        }

        if (!channel.rotations.empty()) {
            outPose[channel.boneIndex].rotation = interpolateRotation(channel, time);
        }

        if (!channel.scales.empty()) {
            outPose[channel.boneIndex].scale = interpolateScale(channel, time);
        }
    }
}

void Animator::buildMatricesFromPose(const std::array<BonePose, se::assets::MAX_BONES>& pose) {
    const auto* s = skeleton();

    if (!s) {
        return;
    }

    const auto& bones = s->bones;

    const int numBones = static_cast<int>(bones.size());

    for (int i = 0; i < numBones; ++i) {
        const BonePose& p = pose[i];

        glm::mat4 t = glm::translate(glm::mat4{1.0f}, p.translation);

        glm::mat4 r = glm::toMat4(p.rotation);

        glm::mat4 s = glm::scale(glm::mat4{1.0f}, p.scale);

        m_LocalMatrices[i] = t * r * s;
    }

    for (int i = 0; i < numBones; ++i) {
        if (bones[i].parent >= 0) {
            m_GlobalMatrices[i] = m_GlobalMatrices[bones[i].parent] * m_LocalMatrices[i];
        } else {
            m_GlobalMatrices[i] = m_LocalMatrices[i];
        }
    }

    for (int i = 0; i < numBones && i < se::assets::MAX_BONES; ++i) {
        m_BoneMatrices[i] = m_GlobalMatrices[i] * bones[i].inverseBindMatrix;
    }
}

glm::vec3 Animator::interpolatePosition(const se::assets::AnimationChannel& channel, float time) const {
    const auto& keys = channel.translations;

    if (keys.size() == 1) {
        return keys[0].value;
    }

    size_t i = findKeyframeIndex(keys, time);

    if (channel.interpolation == se::assets::Interpolation::Step) {
        return keys[i].value;
    }

    float f = scaleFactor(keys, i, time);

    return glm::mix(keys[i].value, keys[i + 1].value, f);
}

glm::quat Animator::interpolateRotation(const se::assets::AnimationChannel& channel, float time) const {
    const auto& keys = channel.rotations;

    if (keys.size() == 1) {
        return keys[0].value;
    }

    size_t i = findKeyframeIndex(keys, time);

    if (channel.interpolation == se::assets::Interpolation::Step) {
        return keys[i].value;
    }

    float f = scaleFactor(keys, i, time);

    return glm::slerp(keys[i].value, keys[i + 1].value, f);
}

glm::vec3 Animator::interpolateScale(const se::assets::AnimationChannel& channel, float time) const {
    const auto& keys = channel.scales;

    if (keys.size() == 1) {
        return keys[0].value;
    }

    size_t i = findKeyframeIndex(keys, time);

    if (channel.interpolation == se::assets::Interpolation::Step) {
        return keys[i].value;
    }

    float f = scaleFactor(keys, i, time);

    return glm::mix(keys[i].value, keys[i + 1].value, f);
}

}  // namespace se::scene