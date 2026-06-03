#include "CameraController.h"

#include <cmath>

namespace se::scene {

void CameraController::sync(const Transform& target, float yaw, float pitch, Camera& camera) const {
    const float yawRad = glm::radians(yaw);
    const glm::vec3 back{-std::cos(yawRad), 0.0f, -std::sin(yawRad)};
    const glm::vec3 cameraPos = target.position + back * m_FollowDistance + glm::vec3(0.0f, m_FollowHeight, 0.0f);

    camera.setPosition(cameraPos);
    camera.setYawPitch(yaw, pitch);
}

}  // namespace se::scene
