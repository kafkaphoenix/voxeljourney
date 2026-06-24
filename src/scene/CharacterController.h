#pragma once

#include <glm/vec3.hpp>

#include "PlayerIntent.h"
#include "Transform.h"
#include "core/Config.h"

namespace se::scene {

class CharacterController {
public:
    struct LocomotionIntent {
        bool moving = false;
        bool running = false;
    };

    explicit CharacterController(const se::core::config::CharacterController& config);

    void updateFirstPerson(float deltaTime, const PlayerIntent& intent, Transform& transform);
    void updateThirdPerson(float deltaTime, const PlayerIntent& intent, Transform& transform, float cameraYaw);

    [[nodiscard]] float yaw() const { return m_Yaw; }
    [[nodiscard]] float pitch() const { return m_Pitch; }
    [[nodiscard]] float facingYaw() const { return m_FacingYaw; }
    [[nodiscard]] const LocomotionIntent& locomotionIntent() const { return m_LocomotionIntent; }
    [[nodiscard]] bool usesRootMotion() const { return m_UseRootMotion; }
    [[nodiscard]] const glm::vec3& movementDirection() const { return m_MoveDirection; }
    [[nodiscard]] float movementSpeed(bool running) const { return running ? m_RunSpeed : m_WalkSpeed; }

private:
    void updateMouseLook(const PlayerIntent& intent);
    void updateMovement(float deltaTime, const PlayerIntent& intent, Transform& transform, float movementYaw);
    void updateFacing(float deltaTime, bool alignFacingToViewWhenIdle);
    void applyMovementStep(float stepSeconds, const glm::vec3& moveDirection, bool running, Transform& transform) const;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_FacingYaw = 0.0f;
    float m_ThirdPersonMovementYaw = 0.0f;
    float m_TurnResponsiveness = 5.0f;
    float m_WalkSpeed = 20.0f;
    float m_RunSpeed = 40.0f;

    float m_MouseSensitivity = 0.1f;
    float m_MouseSmoothAlpha = 0.15f;
    float m_SmoothedDx = 0.0f;
    float m_SmoothedDy = 0.0f;
    glm::vec3 m_MoveDirection{0.0f};

    bool m_UseRootMotion = false;

    LocomotionIntent m_LocomotionIntent{};
};

}  // namespace se::scene
