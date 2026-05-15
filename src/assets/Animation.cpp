#include "Animation.h"

#include <algorithm>

namespace se::assets {

namespace {

template <typename T>
size_t findKeyframeIndex(const std::vector<Keyframe<T>>& keys, float time) {
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
float scaleFactor(const std::vector<Keyframe<T>>& keys, size_t index, float time) {
    const float t0 = keys[index].time;
    const float t1 = keys[index + 1].time;
    const float dt = t1 - t0;

    if (dt <= 0.0f) {
        return 0.0f;
    }

    return std::clamp((time - t0) / dt, 0.0f, 1.0f);
}

glm::vec3 interpolatePosition(const AnimationChannel& channel, float time) {
    const auto& keys = channel.translations;
    if (keys.empty()) {
        return glm::vec3{0.0f};
    }
    if (keys.size() == 1) {
        return keys[0].value;
    }

    const size_t i = findKeyframeIndex(keys, time);
    if (channel.interpolation == Interpolation::Step) {
        return keys[i].value;
    }

    const float t = scaleFactor(keys, i, time);
    return glm::mix(keys[i].value, keys[i + 1].value, t);
}

glm::quat interpolateRotation(const AnimationChannel& channel, float time) {
    const auto& keys = channel.rotations;
    if (keys.empty()) {
        return glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
    }
    if (keys.size() == 1) {
        return keys[0].value;
    }

    const size_t i = findKeyframeIndex(keys, time);
    if (channel.interpolation == Interpolation::Step) {
        return keys[i].value;
    }

    const float t = scaleFactor(keys, i, time);
    return glm::normalize(glm::slerp(keys[i].value, keys[i + 1].value, t));
}

glm::vec3 interpolateScale(const AnimationChannel& channel, float time) {
    const auto& keys = channel.scales;
    if (keys.empty()) {
        return glm::vec3{1.0f};
    }
    if (keys.size() == 1) {
        return keys[0].value;
    }

    const size_t i = findKeyframeIndex(keys, time);
    if (channel.interpolation == Interpolation::Step) {
        return keys[i].value;
    }

    const float t = scaleFactor(keys, i, time);
    return glm::mix(keys[i].value, keys[i + 1].value, t);
}

}  // namespace

void AnimationClip::sample(float time, const Skeleton& skeleton, Pose& outPose) const {
    const int boneCount = (std::min)(static_cast<int>(skeleton.bones.size()), MAX_BONES);

    // Start from skeleton rest pose.
    for (int i = 0; i < boneCount; ++i) {
        outPose.at(i).translation = skeleton.bones.at(i).restPosition;
        outPose.at(i).rotation = skeleton.bones.at(i).restRotation;
        outPose.at(i).scale = skeleton.bones.at(i).restScale;
    }

    // Override with clip channels.
    for (const auto& channel : channels) {
        if (channel.boneIndex < 0 || channel.boneIndex >= boneCount) {
            continue;
        }

        if (!channel.translations.empty()) {
            outPose.at(channel.boneIndex).translation = interpolatePosition(channel, time);
        }

        if (!channel.rotations.empty()) {
            outPose.at(channel.boneIndex).rotation = interpolateRotation(channel, time);
        }

        if (!channel.scales.empty()) {
            outPose.at(channel.boneIndex).scale = interpolateScale(channel, time);
        }
    }

    for (int i = boneCount; i < MAX_BONES; ++i) { outPose.at(i) = {}; }
}

}  // namespace se::assets
