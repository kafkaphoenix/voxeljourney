#include "AnimationController.h"

#include <utility>

#include "Animator.h"

namespace se::scene {

void AnimationController::setLocomotionClips(LocomotionClips clips) {
    m_Clips = std::move(clips);
    m_Initialized = false;
}

void AnimationController::setLocomotionIntent(bool moving, bool running) {
    if (!moving) {
        m_DesiredState = LocomotionState::Idle;
        return;
    }
    m_DesiredState = running ? LocomotionState::Running : LocomotionState::Walking;
}

void AnimationController::apply(Animator& animator) {
    if (!m_Initialized) {
        animator.play(clipForState(m_CurrentState));
        m_Initialized = true;
        return;
    }

    if (m_DesiredState == m_CurrentState) {
        return;
    }

    m_CurrentState = m_DesiredState;
    animator.blendTo(clipForState(m_CurrentState), m_BlendDuration);
}

const std::string& AnimationController::clipForState(LocomotionState state) const {
    switch (state) {
    case LocomotionState::Idle: return m_Clips.idle;
    case LocomotionState::Walking: return m_Clips.walk;
    case LocomotionState::Running: return m_Clips.run;
    }

    return m_Clips.idle;
}

}  // namespace se::scene
