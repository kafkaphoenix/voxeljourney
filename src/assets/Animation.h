#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

#include "Pose.h"
#include "Skeleton.h"

namespace se::assets {

enum class Interpolation : uint8_t { Step, Linear };

// Keyframe data for a single animated property (translation, rotation, or scale).
template <typename T>
struct Keyframe {
    float time = 0.0f;
    T value{};
};

// Animation channel for a single bone, containing keyframes for translation, rotation, and scale.
struct AnimationChannel {
    int boneIndex = -1;  // index into Skeleton::bones
    Interpolation interpolation = Interpolation::Linear;

    std::vector<Keyframe<glm::vec3>> translations;
    std::vector<Keyframe<glm::quat>> rotations;
    std::vector<Keyframe<glm::vec3>> scales;
};

// A named animation clip containing channels for multiple bones. For example, a "walk" clip might have channels for
// the legs, arms, and torso.
struct AnimationClip {
    std::string name;
    float duration = 0.0f;  // max timestamp across all channels
    std::vector<AnimationChannel> channels;

    // Evaluates the pose for the given time by sampling each channel and applying the animated transformations to the
    // corresponding bones in the skeleton. The resulting pose is stored in outPose.
    void sample(float time, const Skeleton& skeleton, Pose& outPose) const;
};

}  // namespace se::assets
