#include "Animation.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string_view>

namespace se::assets {

namespace {

const AnimationChannel* findChannel(const AnimationClip& clip, int boneIndex) {
    for (const auto& channel : clip.channels) {
        if (channel.boneIndex == boneIndex) {
            return &channel;
        }
    }

    return nullptr;
}

float wrapTime(float time, float duration) {
    if (duration <= 0.0f) {
        return 0.0f;
    }

    float wrapped = std::fmod(time, duration);
    if (wrapped < 0.0f) {
        wrapped += duration;
    }

    return wrapped;
}

int boneDepth(const Skeleton& skeleton, int boneIndex) {
    int depth = 0;
    int current = boneIndex;
    while (current >= 0 && current < static_cast<int>(skeleton.bones.size())) {
        current = skeleton.bones[static_cast<size_t>(current)].parent;
        if (current >= 0) {
            ++depth;
        }
    }

    return depth;
}

bool containsInsensitive(std::string_view text, std::string_view needle) {
    if (needle.empty() || needle.size() > text.size()) {
        return false;
    }

    for (size_t start = 0; start + needle.size() <= text.size(); ++start) {
        bool matches = true;
        for (size_t i = 0; i < needle.size(); ++i) {
            const unsigned char lhs = static_cast<unsigned char>(text[start + i]);
            const unsigned char rhs = static_cast<unsigned char>(needle[i]);
            if (std::tolower(lhs) != std::tolower(rhs)) {
                matches = false;
                break;
            }
        }
        if (matches) {
            return true;
        }
    }

    return false;
}

int rootMotionNamePriority(std::string_view boneName) {
    if (containsInsensitive(boneName, "root")) {
        return 3;
    }
    if (containsInsensitive(boneName, "hip") || containsInsensitive(boneName, "pelvis")) {
        return 2;
    }
    return 0;
}

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

glm::vec3 sampleBonePosition(const AnimationClip& clip, float time, const Skeleton& skeleton, int boneIndex,
                             bool wrapSampleTime) {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(skeleton.bones.size())) {
        return glm::vec3{0.0f};
    }

    float sampleTime = time;
    if (clip.duration > 0.0f) {
        sampleTime = wrapSampleTime ? wrapTime(time, clip.duration) : std::clamp(time, 0.0f, clip.duration);
    }

    Pose pose{};
    clip.sample(sampleTime, skeleton, pose);

    glm::mat4 worldTransform{1.0f};
    int current = boneIndex;
    while (current >= 0) {
        const auto& bonePose = pose.at(current);
        const glm::mat4 localTransform = glm::translate(glm::mat4{1.0f}, bonePose.translation) *
                                         glm::mat4_cast(bonePose.rotation) *
                                         glm::scale(glm::mat4{1.0f}, bonePose.scale);
        worldTransform = localTransform * worldTransform;
        current = skeleton.bones[static_cast<size_t>(current)].parent;
    }

    return glm::vec3(worldTransform[3]);
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

int AnimationClip::rootMotionBoneIndex(const Skeleton& skeleton) const {
    int bestBoneIndex = -1;
    int bestNamePriority = -1;
    int bestDepth = std::numeric_limits<int>::max();

    for (const auto& channel : channels) {
        if (channel.boneIndex < 0 || channel.boneIndex >= static_cast<int>(skeleton.bones.size()) ||
            channel.translations.empty()) {
            continue;
        }

        const auto& bone = skeleton.bones[static_cast<size_t>(channel.boneIndex)];
        const int namePriority = rootMotionNamePriority(bone.name);
        const int depth = boneDepth(skeleton, channel.boneIndex);

        if (namePriority > bestNamePriority || (namePriority == bestNamePriority && depth < bestDepth)) {
            bestBoneIndex = channel.boneIndex;
            bestNamePriority = namePriority;
            bestDepth = depth;
        }
    }

    return bestBoneIndex;
}

glm::vec3 AnimationClip::sampleRootDelta(float previousTime, float currentTime, const Skeleton& skeleton) const {
    if (duration <= 0.0f || currentTime <= previousTime) {
        return glm::vec3{0.0f};
    }

    const int rootBoneIndex = rootMotionBoneIndex(skeleton);
    if (rootBoneIndex < 0) {
        return glm::vec3{0.0f};
    }

    const int previousLoop = static_cast<int>(std::floor(previousTime / duration));
    const int currentLoop = static_cast<int>(std::floor(currentTime / duration));
    const float previousWrappedTime = wrapTime(previousTime, duration);
    const float currentWrappedTime = wrapTime(currentTime, duration);

    if (previousLoop == currentLoop) {
        return sampleBonePosition(*this, currentWrappedTime, skeleton, rootBoneIndex, false) -
               sampleBonePosition(*this, previousWrappedTime, skeleton, rootBoneIndex, false);
    }

    const glm::vec3 startPosition = sampleBonePosition(*this, 0.0f, skeleton, rootBoneIndex, false);
    const glm::vec3 endPosition = sampleBonePosition(*this, duration, skeleton, rootBoneIndex, false);

    glm::vec3 totalDelta = endPosition - sampleBonePosition(*this, previousWrappedTime, skeleton, rootBoneIndex, false);

    if (currentLoop - previousLoop > 1) {
        totalDelta += (endPosition - startPosition) * static_cast<float>(currentLoop - previousLoop - 1);
    }

    totalDelta += sampleBonePosition(*this, currentWrappedTime, skeleton, rootBoneIndex, false) - startPosition;
    return totalDelta;
}

}  // namespace se::assets
