#include "CameraController.h"

#include <cmath>
#include <glm/common.hpp>

namespace se::scene {

CameraController::CameraController(const se::core::config::CameraController& config)
    : m_FollowDistance(config.followDistance),
      m_FollowHeight(config.followHeight),
      m_EyeHeight(config.eyeHeight),
      m_EyeForwardOffset(config.eyeForwardOffset) {}

void CameraController::updateThirdPerson(const PlayerIntent& intent, float initialYaw, float initialPitch) {
    if (!m_ThirdPersonInitialized) {
        m_ThirdPersonYaw = initialYaw;
        m_ThirdPersonPitch = glm::clamp(initialPitch - 15.0f, -80.0f, 45.0f);
        m_ThirdPersonInitialized = true;
    }

    m_ThirdPersonYaw += intent.lookDelta.x * 0.1f;
    m_ThirdPersonPitch = glm::clamp(m_ThirdPersonPitch - intent.lookDelta.y * 0.1f, -80.0f, 45.0f);
}

void CameraController::sync(const Transform& target, float yaw, float pitch, Camera& camera) const {
    const float cameraYaw = (m_Mode == CameraMode::ThirdPerson) ? m_ThirdPersonYaw : yaw;
    const float cameraPitch = (m_Mode == CameraMode::ThirdPerson) ? m_ThirdPersonPitch : pitch;
    const glm::vec3 cameraPos = (m_Mode == CameraMode::ThirdPerson) ? syncThirdPerson(target, cameraYaw, cameraPitch)
                                                                    : syncFirstPerson(target, yaw);

    camera.setPosition(cameraPos);
    camera.setYawPitch(cameraYaw, cameraPitch);
    camera.setVisibilityMask(m_Mode == CameraMode::FirstPerson ? se::render::visibility::FIRST_PERSON_CAMERA
                                                               : se::render::visibility::THIRD_PERSON_CAMERA);
}

glm::vec3 CameraController::syncThirdPerson(const Transform& target, float yaw, float pitch) const {
    const float yawRad = glm::radians(yaw);
    const float pitchRad = glm::radians(pitch);
    const glm::vec3 forward{std::cos(pitchRad) * std::cos(yawRad), std::sin(pitchRad),
                            std::cos(pitchRad) * std::sin(yawRad)};
    const glm::vec3 focusPoint = target.position + glm::vec3(0.0f, m_FollowHeight, 0.0f);
    return focusPoint - forward * m_FollowDistance;
}

glm::vec3 CameraController::syncFirstPerson(const Transform& target, float yaw) const {
    const float yawRad = glm::radians(yaw);
    const glm::vec3 forward{std::cos(yawRad), 0.0f, std::sin(yawRad)};
    return target.position + glm::vec3(0.0f, m_EyeHeight, 0.0f) + forward * m_EyeForwardOffset;
}

}  // namespace se::scene