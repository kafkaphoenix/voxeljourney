#pragma once
#include <glm/glm.hpp>

#include "core/Config.h"

namespace se::scene {

class Camera {
public:
    Camera(const se::core::Config::Camera& config);

    void processMouse(float xoffset, float yoffset);
    void processKeyboard(bool forward, bool backward, bool left, bool right, bool up, bool down, float deltaTime);

    [[nodiscard]] glm::mat4 getViewProjection() const;
    [[nodiscard]] glm::vec3 getPosition() const { return m_Position; }

    void setAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }
    void setPosition(const glm::vec3& position) { m_Position = position; }
    void setMoveSpeed(float speed);
    void setMouseSensitivity(float sensitivity);
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

    float m_AspectRatio = 16.0f / 9.0f;
    glm::vec3 m_Position{-5.0f, 5.0f, 5.0f};
    float m_Speed = 10.0f;
    float m_Sensitivity = 0.1f;
    float m_Fov = 60.0f;
    float m_Near = 0.1f;
    float m_Far = 1000.0f;
};

}  // namespace se::scene