#pragma once

#include <glm/glm.hpp>

#include "Camera.h"
#include "PlayerIntent.h"
#include "Transform.h"

namespace se::scene {

enum class CameraMode : uint8_t { ThirdPerson, FirstPerson };

class CameraController {
public:
    explicit CameraController(const se::core::config::CameraController& config);

    void updateThirdPerson(const PlayerIntent& intent, float initialYaw, float initialPitch);
    void sync(const Transform& target, float yaw, float pitch, Camera& camera) const;

    void setMode(CameraMode mode) { m_Mode = mode; }
    [[nodiscard]] CameraMode getMode() const { return m_Mode; }
    [[nodiscard]] float thirdPersonYaw() const { return m_ThirdPersonYaw; }
    [[nodiscard]] float thirdPersonPitch() const { return m_ThirdPersonPitch; }

    void setFollowDistance(float distance) { m_FollowDistance = distance; }
    void setFollowHeight(float height) { m_FollowHeight = height; }
    void setEyeHeight(float height) { m_EyeHeight = height; }
    void setEyeForwardOffset(float offset) { m_EyeForwardOffset = offset; }

private:
    [[nodiscard]] glm::vec3 syncThirdPerson(const Transform& target, float yaw, float pitch) const;
    [[nodiscard]] glm::vec3 syncFirstPerson(const Transform& target, float yaw) const;

    CameraMode m_Mode = CameraMode::FirstPerson;
    // Third-person camera parameters
    float m_FollowDistance = 5.0f;  // distance behind the target
    float m_FollowHeight = 2.0f;    // height of the camera above the target
    // First-person camera parameters
    float m_EyeHeight = 1.7f;          // height of the camera from the ground
    float m_EyeForwardOffset = 0.15f;  // forward offset of the camera from the character's position
    float m_ThirdPersonYaw = 0.0f;
    float m_ThirdPersonPitch = -15.0f;
    bool m_ThirdPersonInitialized = false;
};

}  // namespace se::scene