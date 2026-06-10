#pragma once

#include <glm/glm.hpp>

#include "core/Config.h"

namespace se::scene {

class Camera {
public:
    explicit Camera(const se::core::config::Camera& config);

    [[nodiscard]] glm::vec3 getPosition() const { return m_Position; }
    [[nodiscard]] glm::mat4 getProjectionMatrix() const;
    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] float getYaw() const { return m_Yaw; }

    void setAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }
    void setPosition(const glm::vec3& position) { m_Position = position; }
    void setYawPitch(float yaw, float pitch) {
        m_Yaw = yaw;
        m_Pitch = glm::clamp(pitch, -89.0f, 89.0f);
        updateVectors();
    }
    void setFov(float fov);
    void setClipPlanes(float nearPlane, float farPlane);

private:
    void updateVectors();

    glm::vec3 m_Front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_Up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_Right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_WorldUp{0.0f, 1.0f, 0.0f};
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;

    float m_AspectRatio = 16.0f / 9.0f;  // updated by resize events
    glm::vec3 m_Position{-25.0f, 15.0f, 0.0f};
    float m_Fov = 80.0f;
    float m_Near = 0.1f;
    float m_Far = 1000.0f;
};

}  // namespace se::scene