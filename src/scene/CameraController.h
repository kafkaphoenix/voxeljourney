#pragma once

#include "Camera.h"
#include "Transform.h"
#include "core/Config.h"

namespace se::scene {

class CameraController {
public:
    explicit CameraController(const se::core::config::ThirdPersonCameraController& config)
        : m_FollowDistance(config.followDistance), m_FollowHeight(config.followHeight) {}

    void sync(const Transform& target, float yaw, float pitch, Camera& camera) const;

private:
    float m_FollowDistance = 5.0f;
    float m_FollowHeight = 2.0f;
};

}  // namespace se::scene
