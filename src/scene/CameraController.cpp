#include "CameraController.h"

#include <cmath>

namespace se::scene {

CameraController::CameraController(const se::core::config::CameraController& config)
    : m_FollowDistance(config.followDistance),
      m_FollowHeight(config.followHeight),
      m_EyeHeight(config.eyeHeight),
      m_EyeForwardOffset(config.eyeForwardOffset) {}

void CameraController::sync(const Transform& target, float yaw, float pitch, Camera& camera) const {
    const glm::vec3 cameraPos =
        (m_Mode == CameraMode::ThirdPerson) ? syncThirdPerson(target, yaw) : syncFirstPerson(target, yaw);

    camera.setPosition(cameraPos);
    camera.setYawPitch(yaw, pitch);
    camera.setVisibilityMask(m_Mode == CameraMode::FirstPerson ? se::render::visibility::FIRST_PERSON_CAMERA
                                                               : se::render::visibility::THIRD_PERSON_CAMERA);
}

glm::vec3 CameraController::syncThirdPerson(const Transform& target, float yaw) const {
    const float yawRad = glm::radians(yaw);
    const glm::vec3 back{-std::cos(yawRad), 0.0f, -std::sin(yawRad)};
    return target.position + back * m_FollowDistance + glm::vec3(0.0f, m_FollowHeight, 0.0f);
}

glm::vec3 CameraController::syncFirstPerson(const Transform& target, float yaw) const {
    const float yawRad = glm::radians(yaw);
    const glm::vec3 forward{std::cos(yawRad), 0.0f, std::sin(yawRad)};
    return target.position + glm::vec3(0.0f, m_EyeHeight, 0.0f) + forward * m_EyeForwardOffset;
}

}  // namespace se::scene