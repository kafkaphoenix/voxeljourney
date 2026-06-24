#include "Camera.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace se::scene {

Camera::Camera(const se::core::config::Camera& config)
    : m_Fov(config.fov), m_Near(config.nearPlane), m_Far(config.farPlane) {
    updateVectors();
}

void Camera::updateVectors() {
    glm::vec3 front;
    front.x = std::cos(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));
    front.y = std::sin(glm::radians(m_Pitch));
    front.z = std::sin(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void Camera::setFov(float fov) {
    if (fov > 1.0f && fov < 179.0f) {
        m_Fov = fov;
    }
}

void Camera::setClipPlanes(float nearPlane, float farPlane) {
    if (nearPlane > 0.0f && farPlane > nearPlane) {
        m_Near = nearPlane;
        m_Far = farPlane;
    }
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_Near, m_Far);
}

glm::mat4 Camera::getViewMatrix() const { return glm::lookAt(m_Position, m_Position + m_Front, m_Up); }

}  // namespace se::scene