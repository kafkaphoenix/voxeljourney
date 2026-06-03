#pragma once

#include "Transform.h"
#include "core/Config.h"

namespace se::core {
class Input;
}

namespace se::scene {

class CharacterController {
public:
    struct LocomotionIntent {
        bool moving = false;
        bool running = false;
    };

    explicit CharacterController(const se::core::config::CharacterController& config);

    void update(float deltaTime, const se::core::Input& input, Transform& transform);

    [[nodiscard]] float yaw() const { return m_Yaw; }
    [[nodiscard]] float pitch() const { return m_Pitch; }
    [[nodiscard]] const LocomotionIntent& locomotionIntent() const { return m_LocomotionIntent; }

private:
    void updateMouseLook(const se::core::Input& input);
    void updateMovement(float deltaTime, const se::core::Input& input, Transform& transform);
    void applyMovementStep(float stepSeconds, const se::core::Input& input, Transform& transform) const;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_WalkSpeed = 20.0f;
    float m_RunSpeed = 40.0f;

    float m_MouseSensitivity = 0.1f;
    float m_MouseSmoothAlpha = 0.15f;
    float m_SmoothedDx = 0.0f;
    float m_SmoothedDy = 0.0f;

    bool m_UseFixedStep = true;
    float m_FixedStep = 1.0f / 60.0f;
    float m_MoveAccumulator = 0.0f;

    LocomotionIntent m_LocomotionIntent{};
};

}  // namespace se::scene
