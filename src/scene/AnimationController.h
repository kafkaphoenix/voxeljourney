#pragma once

#include <cstdint>
#include <string>

namespace se::scene {
class Animator;

class AnimationController {
public:
    struct LocomotionClips {
        std::string idle = "Survey";
        std::string walk = "Walk";
        std::string run = "Run";
    };

    enum class LocomotionState : uint8_t { Idle, Walking, Running };

    void setLocomotionClips(LocomotionClips clips);
    void setLocomotionIntent(bool moving, bool running);
    void apply(Animator& animator);

private:
    const std::string& clipForState(LocomotionState state) const;

    LocomotionClips m_Clips;
    LocomotionState m_CurrentState = LocomotionState::Idle;
    LocomotionState m_DesiredState = LocomotionState::Idle;
    bool m_Initialized = false;
    float m_BlendDuration = 0.35f;
};

}  // namespace se::scene
